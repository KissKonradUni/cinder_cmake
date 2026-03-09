#include "helpers/mesh_converter.hpp"
#include "logging.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>

namespace codex {

// ---- Constants ----

static constexpr uint32_t CEMF_MAGIC   = 0x464D4543; // "CEMF" little-endian
static constexpr uint16_t CEMF_VERSION = 1;

// ---- Assimp node-tree helpers ----

// Decompose an aiMatrix4x4 into our packed transform
static P_Transform decomposeAiMatrix(const aiMatrix4x4& m) {
    aiVector3D   pos, scl;
    aiQuaternion rot;
    m.Decompose(scl, rot, pos);

    P_Transform t{};
    t.position = { pos.x, pos.y, pos.z };
    t.rotation = { rot.x, rot.y, rot.z, rot.w };
    t.scale    = { scl.x, scl.y, scl.z };
    return t;
}

// One entry per (node, mesh-index) pair produced by the tree walk
struct SubmeshEntry {
    uint32_t    assimpMeshIdx;  // index into scene->mMeshes[]
    P_Transform localTransform; // node-local transform
    uint32_t    parentSubmeshID; // our submesh ID of the parent node (SUBMESH_NO_PARENT for root)
};

// Recursively collect all (node, mesh) pairs preserving hierarchy.
// `parentID` is the submesh ID of the first mesh in the parent node
// (or SUBMESH_NO_PARENT for the root).
static void collectSubmeshes(const aiNode* node,
                             uint32_t parentID,
                             std::vector<SubmeshEntry>& out) {
    // The first mesh on this node inherits the parent relationship.
    // Subsequent meshes on the same node share the same parent.
    uint32_t firstIDOnThisNode = parentID;

    for (unsigned i = 0; i < node->mNumMeshes; ++i) {
        SubmeshEntry e{};
        e.assimpMeshIdx  = node->mMeshes[i];
        e.localTransform = decomposeAiMatrix(node->mTransformation);
        e.parentSubmeshID = parentID;

        uint32_t myID = static_cast<uint32_t>(out.size());
        if (i == 0) firstIDOnThisNode = myID;

        out.push_back(e);
    }

    // If the node had no meshes, it's a pure grouping node — propagate
    // its transform through parentID for its children.
    // In that case children still reference the same parentID we got.
    uint32_t childParent = (node->mNumMeshes > 0) ? firstIDOnThisNode : parentID;

    for (unsigned c = 0; c < node->mNumChildren; ++c) {
        collectSubmeshes(node->mChildren[c], childParent, out);
    }
}

// ---- Quantization / encoding helpers ----

// Map a float from [lo, hi] -> [0, 65535]
static uint16_t quantizePosition(float value, float lo, float hi) {
    if (hi - lo < 1e-7f) return 32768;
    float t = std::clamp((value - lo) / (hi - lo), 0.0f, 1.0f);
    return static_cast<uint16_t>(t * 65535.0f + 0.5f);
}

// Map a float from [0, 1] -> [0, 65535]
static uint16_t quantizeUV(float value) {
    return static_cast<uint16_t>(std::clamp(value, 0.0f, 1.0f) * 65535.0f + 0.5f);
}

// Octahedral-encode a unit normal to two int16_t values in [-32767, 32767]
static void octahedralEncode(float nx, float ny, float nz,
                             int16_t& outX, int16_t& outY) {
    float l1 = std::abs(nx) + std::abs(ny) + std::abs(nz);
    if (l1 < 1e-7f) { outX = outY = 0; return; }

    float ox = nx / l1;
    float oy = ny / l1;

    if (nz < 0.0f) {
        float tx = (1.0f - std::abs(oy)) * (ox >= 0.0f ? 1.0f : -1.0f);
        float ty = (1.0f - std::abs(ox)) * (oy >= 0.0f ? 1.0f : -1.0f);
        ox = tx;
        oy = ty;
    }

    outX = static_cast<int16_t>(std::clamp(ox, -1.0f, 1.0f) * 32767.0f);
    outY = static_cast<int16_t>(std::clamp(oy, -1.0f, 1.0f) * 32767.0f);
}

// ---- Binary write helpers ----

static void writeBytes(std::vector<uint8_t>& buf, const void* data, size_t n) {
    auto p = reinterpret_cast<const uint8_t*>(data);
    buf.insert(buf.end(), p, p + n);
}

// Write a P_String: uint32_t length (incl. null) + string bytes + null
static void writePString(std::vector<uint8_t>& buf, const std::string& s) {
    uint32_t len = static_cast<uint32_t>(s.size() + 1); // +1 for '\0'
    writeBytes(buf, &len, sizeof(len));
    writeBytes(buf, s.c_str(), len);
}

// Read a P_String from a raw pointer, advancing it
static std::string readPString(const uint8_t*& ptr, const uint8_t* end) {
    if (ptr + sizeof(uint32_t) > end) return {};
    uint32_t len;
    std::memcpy(&len, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    if (len == 0 || ptr + len > end) return {};
    std::string result(reinterpret_cast<const char*>(ptr), len - 1);
    ptr += len;
    return result;
}

// ---- Expand / contract an AABB with a single point ----

static void expandAABB(P_AABB& box, float x, float y, float z) {
    box.min.x = std::min(box.min.x, x);
    box.min.y = std::min(box.min.y, y);
    box.min.z = std::min(box.min.z, z);
    box.max.x = std::max(box.max.x, x);
    box.max.y = std::max(box.max.y, y);
    box.max.z = std::max(box.max.z, z);
}

static P_AABB invalidAABB() {
    constexpr float big = std::numeric_limits<float>::max();
    return { { big, big, big }, { -big, -big, -big } };
}

// ========================================================================
// convertUsingAssimp – load via Assimp and write the .cemf binary format
// ========================================================================

void LoadedMesh::convertUsingAssimp(const std::filesystem::path& filePath,
                                    const std::filesystem::path& outputPath) {
    // Skip conversion when the output is already newer than the source
    if (std::filesystem::exists(outputPath) && std::filesystem::exists(filePath)) {
        auto srcTime = std::filesystem::last_write_time(filePath);
        auto dstTime = std::filesystem::last_write_time(outputPath);
        if (dstTime >= srcTime) {
            echo::logInfo("Skipping conversion - " + outputPath.filename().string()
                          + " is up to date.");
            return;
        }
    }

    echo::logInfo("Converting mesh: " + filePath.string()
                  + " -> " + outputPath.string());

    // --- Import with Assimp ---
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        filePath.string(),
        aiProcess_Triangulate
        | aiProcess_GenNormals
        | aiProcess_GenUVCoords
        | aiProcess_JoinIdenticalVertices
        | aiProcess_OptimizeMeshes
        | aiProcess_ImproveCacheLocality);

    if (!scene || !scene->mRootNode
        || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
        echo::logError("Assimp import failed: "
                       + std::string(importer.GetErrorString()));
        return;
    }

    // --- Compute global AABB (also used as quantization bounds) ---
    P_AABB globalAABB = invalidAABB();
    for (unsigned m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* am = scene->mMeshes[m];
        for (unsigned v = 0; v < am->mNumVertices; ++v) {
            const auto& p = am->mVertices[v];
            expandAABB(globalAABB, p.x, p.y, p.z);
        }
    }
    P_AABB quantBounds = globalAABB;

    // --- Walk the node tree to collect submeshes with hierarchy ---
    std::vector<SubmeshEntry> submeshEntries;
    collectSubmeshes(scene->mRootNode, SUBMESH_NO_PARENT, submeshEntries);

    // --- Build chunks for every submesh entry ---
    struct ChunkEntry {
        MeshChunkHeader header;
        std::vector<uint8_t> data;
    };
    std::vector<ChunkEntry> chunks;

    for (uint32_t m = 0; m < static_cast<uint32_t>(submeshEntries.size()); ++m) {
        const auto& entry = submeshEntries[m];
        const aiMesh* am  = scene->mMeshes[entry.assimpMeshIdx];

        // ---- SubmeshData ----
        {
            ChunkEntry ce{};
            ce.header.type   = MeshChunkType::SubmeshData;
            ce.header.meshID = m;

            SubmeshDataChunk sd{};
            sd.transform       = entry.localTransform;
            sd.parentSubmeshID = entry.parentSubmeshID;

            P_AABB meshAABB = invalidAABB();
            for (unsigned v = 0; v < am->mNumVertices; ++v) {
                const auto& p = am->mVertices[v];
                expandAABB(meshAABB, p.x, p.y, p.z);
            }
            sd.submeshBoundingBox = meshAABB;

            writeBytes(ce.data, &sd, sizeof(sd));
            ce.header.size = static_cast<uint32_t>(ce.data.size());
            chunks.push_back(std::move(ce));
        }

        // ---- VertexData ----
        {
            ChunkEntry ce{};
            ce.header.type   = MeshChunkType::VertexData;
            ce.header.meshID = m;

            uint32_t vertexCount = am->mNumVertices;
            writeBytes(ce.data, &vertexCount, sizeof(vertexCount));

            for (unsigned v = 0; v < am->mNumVertices; ++v) {
                P_Vertex vert{};

                const auto& pos = am->mVertices[v];
                vert.position[0] = quantizePosition(pos.x, quantBounds.min.x, quantBounds.max.x);
                vert.position[1] = quantizePosition(pos.y, quantBounds.min.y, quantBounds.max.y);
                vert.position[2] = quantizePosition(pos.z, quantBounds.min.z, quantBounds.max.z);
                vert.position[3] = 0; // unused padding

                if (am->mNormals) {
                    const auto& n = am->mNormals[v];
                    octahedralEncode(n.x, n.y, n.z, vert.normal[0], vert.normal[1]);
                }

                if (am->mTextureCoords[0]) {
                    const auto& uv = am->mTextureCoords[0][v];
                    vert.uv[0] = quantizeUV(uv.x);
                    vert.uv[1] = quantizeUV(uv.y);
                }

                writeBytes(ce.data, &vert, sizeof(vert));
            }

            ce.header.size = static_cast<uint32_t>(ce.data.size());
            chunks.push_back(std::move(ce));
        }

        // ---- IndexData ----
        {
            ChunkEntry ce{};
            ce.header.type   = MeshChunkType::IndexData;
            ce.header.meshID = m;

            uint32_t indexCount = 0;
            for (unsigned f = 0; f < am->mNumFaces; ++f)
                indexCount += am->mFaces[f].mNumIndices;

            writeBytes(ce.data, &indexCount, sizeof(indexCount));
            for (unsigned f = 0; f < am->mNumFaces; ++f) {
                const aiFace& face = am->mFaces[f];
                for (unsigned i = 0; i < face.mNumIndices; ++i) {
                    uint32_t idx = face.mIndices[i];
                    writeBytes(ce.data, &idx, sizeof(idx));
                }
            }

            ce.header.size = static_cast<uint32_t>(ce.data.size());
            chunks.push_back(std::move(ce));
        }

        // ---- MaterialData ----
        {
            ChunkEntry ce{};
            ce.header.type   = MeshChunkType::MaterialData;
            ce.header.meshID = m;

            std::string diffusePath, normalPath, armPath;

            if (am->mMaterialIndex < scene->mNumMaterials) {
                const aiMaterial* mat = scene->mMaterials[am->mMaterialIndex];
                aiString texPath;

                if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
                    diffusePath = texPath.C_Str();
                if (mat->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS)
                    normalPath = texPath.C_Str();
                if (mat->GetTexture(aiTextureType_METALNESS, 0, &texPath) == AI_SUCCESS)
                    armPath = texPath.C_Str();
                else if (mat->GetTexture(aiTextureType_UNKNOWN, 0, &texPath) == AI_SUCCESS)
                    armPath = texPath.C_Str();
            }

            writePString(ce.data, diffusePath);
            writePString(ce.data, normalPath);
            writePString(ce.data, armPath);

            ce.header.size = static_cast<uint32_t>(ce.data.size());
            chunks.push_back(std::move(ce));
        }
    }

    // --- Patch nextChunkOffset for every chunk ---
    uint32_t cursor = static_cast<uint32_t>(sizeof(MeshFileHeader));
    for (size_t i = 0; i < chunks.size(); ++i) {
        uint32_t thisSize = static_cast<uint32_t>(sizeof(MeshChunkHeader))
                          + chunks[i].header.size;
        uint32_t nextOff  = (i + 1 < chunks.size()) ? (cursor + thisSize) : 0;
        chunks[i].header.nextChunkOffset = nextOff;
        cursor += thisSize;
    }

    uint32_t totalFileSize = cursor; // cursor has advanced past all chunks

    // --- Assemble file header ---
    MeshFileHeader fh{};
    fh.magic             = CEMF_MAGIC;
    fh.version           = CEMF_VERSION;
    fh.flags             = 0;
    fh.fileSize          = totalFileSize;
    fh.meshCount         = static_cast<uint32_t>(submeshEntries.size());
    fh.firstChunkOffset  = static_cast<uint32_t>(sizeof(MeshFileHeader));
    fh.wholeBoundingBox  = globalAABB;
    fh.quantizationBounds = quantBounds;

    // --- Write ---
    if (outputPath.has_parent_path())
        std::filesystem::create_directories(outputPath.parent_path());

    std::ofstream out(outputPath, std::ios::binary);
    if (!out.is_open()) {
        echo::logError("Cannot open output file: " + outputPath.string());
        return;
    }

    out.write(reinterpret_cast<const char*>(&fh), sizeof(fh));
    for (auto& c : chunks) {
        out.write(reinterpret_cast<const char*>(&c.header), sizeof(MeshChunkHeader));
        out.write(reinterpret_cast<const char*>(c.data.data()),
                  static_cast<std::streamsize>(c.data.size()));
    }
    out.close();

    echo::logInfo("Conversion done: " + outputPath.string()
                  + " (" + std::to_string(totalFileSize) + " bytes, "
                  + std::to_string(submeshEntries.size()) + " sub-meshes)");
}

// ========================================================================
// loadFromFile – read a .cemf binary into a LoadedMesh
// ========================================================================

std::unique_ptr<LoadedMesh> LoadedMesh::loadFromFile(std::filesystem::path filePath) {
    // Read entire file into memory
    std::ifstream in(filePath, std::ios::binary | std::ios::ate);
    if (!in.is_open()) {
        echo::logError("Cannot open mesh file: " + filePath.string());
        return nullptr;
    }

    const auto fileSize = static_cast<size_t>(in.tellg());
    in.seekg(0, std::ios::beg);

    if (fileSize < sizeof(MeshFileHeader)) {
        echo::logError("Mesh file too small: " + filePath.string());
        return nullptr;
    }

    std::vector<uint8_t> blob(fileSize);
    in.read(reinterpret_cast<char*>(blob.data()), static_cast<std::streamsize>(fileSize));
    in.close();

    const uint8_t* data    = blob.data();
    const uint8_t* dataEnd = data + fileSize;

    // --- Parse header ---
    MeshFileHeader fh;
    std::memcpy(&fh, data, sizeof(fh));

    if (fh.magic != CEMF_MAGIC) {
        echo::logError("Bad magic in mesh file: " + filePath.string());
        return nullptr;
    }
    if (fh.version != CEMF_VERSION) {
        echo::logError("Unsupported CEMF version "
                       + std::to_string(fh.version) + " in " + filePath.string());
        return nullptr;
    }

    auto mesh = std::make_unique<LoadedMesh>();
    mesh->wholeBoundingBox  = fh.wholeBoundingBox;
    mesh->quantizationBounds = fh.quantizationBounds;

    // --- Temporary per-meshID storage ---
    struct MeshBuildData {
        SubmeshDataChunk submesh{};
        bool             hasSubmesh  = false;
        uint32_t         parentSubmeshID = SUBMESH_NO_PARENT;
        uint32_t         vertexCount = 0;
        uint32_t         indexCount  = 0;
        std::vector<P_Vertex>  vertices;
        std::vector<uint32_t>  indices;
        LoadedMaterialInfo     material{};
    };
    std::vector<MeshBuildData> perMesh(fh.meshCount);

    // --- Walk the chunk linked-list ---
    uint32_t chunkOff = fh.firstChunkOffset;

    while (chunkOff != 0 && chunkOff + sizeof(MeshChunkHeader) <= fileSize) {
        const uint8_t* cptr = data + chunkOff;

        MeshChunkHeader ch;
        std::memcpy(&ch, cptr, sizeof(ch));

        const uint8_t* cdata = cptr + sizeof(MeshChunkHeader);
        const uint8_t* cend  = cdata + ch.size;

        if (cend > dataEnd) {
            echo::logError("Chunk data exceeds file bounds");
            break;
        }
        if (ch.meshID >= fh.meshCount) {
            echo::logWarning("Chunk references invalid meshID "
                             + std::to_string(ch.meshID));
            chunkOff = ch.nextChunkOffset;
            continue;
        }

        auto& md = perMesh[ch.meshID];

        switch (ch.type) {
        case MeshChunkType::SubmeshData: {
            if (ch.size >= sizeof(SubmeshDataChunk)) {
                std::memcpy(&md.submesh, cdata, sizeof(SubmeshDataChunk));
                md.hasSubmesh = true;
                md.parentSubmeshID = md.submesh.parentSubmeshID;
            }
            break;
        }
        case MeshChunkType::VertexData: {
            if (ch.size < sizeof(uint32_t)) break;
            uint32_t vc;
            std::memcpy(&vc, cdata, sizeof(uint32_t));
            md.vertexCount = vc;

            const uint8_t* vptr = cdata + sizeof(uint32_t);
            size_t need = vc * sizeof(P_Vertex);
            if (vptr + need <= cend) {
                md.vertices.resize(vc);
                std::memcpy(md.vertices.data(), vptr, need);
            }
            break;
        }
        case MeshChunkType::IndexData: {
            if (ch.size < sizeof(uint32_t)) break;
            uint32_t ic;
            std::memcpy(&ic, cdata, sizeof(uint32_t));
            md.indexCount = ic;

            const uint8_t* iptr = cdata + sizeof(uint32_t);
            size_t need = ic * sizeof(uint32_t);
            if (iptr + need <= cend) {
                md.indices.resize(ic);
                std::memcpy(md.indices.data(), iptr, need);
            }
            break;
        }
        case MeshChunkType::MaterialData: {
            const uint8_t* mptr = cdata;
            std::string diffuse = readPString(mptr, cend);
            std::string normal  = readPString(mptr, cend);
            std::string arm     = readPString(mptr, cend);

            cinder::str_init(md.material.diffuse);
            cinder::str_copy(md.material.diffuse, diffuse.c_str());
            cinder::str_init(md.material.normal);
            cinder::str_copy(md.material.normal, normal.c_str());
            cinder::str_init(md.material.arm);
            cinder::str_copy(md.material.arm, arm.c_str());
            break;
        }
        default:
            echo::logWarning("Unknown chunk type 0x"
                             + std::to_string(static_cast<unsigned>(ch.type)));
            break;
        }

        chunkOff = ch.nextChunkOffset;
    }

    // --- Stitch per-mesh data into the unified LoadedMesh ---
    uint32_t globalVertexOff = 0;
    uint32_t globalIndexOff  = 0;

    for (uint32_t m = 0; m < fh.meshCount; ++m) {
        auto& md = perMesh[m];

        mesh->vertices.insert(mesh->vertices.end(),
                              md.vertices.begin(), md.vertices.end());
        mesh->indices.insert(mesh->indices.end(),
                             md.indices.begin(), md.indices.end());

        LoadedSubmesh sub{};
        sub.transform     = md.submesh.transform;
        sub.boundingBox   = md.submesh.submeshBoundingBox;
        sub.vertexOffset  = globalVertexOff;
        sub.vertexCount   = md.vertexCount;
        sub.indexOffset   = globalIndexOff;
        sub.indexCount    = md.indexCount;
        sub.materialIndex = static_cast<uint32_t>(mesh->materials.size());
        sub.parentIndex   = md.parentSubmeshID;

        mesh->submeshes.push_back(sub);
        mesh->materials.push_back(std::move(md.material));

        globalVertexOff += md.vertexCount;
        globalIndexOff  += md.indexCount;
    }

    echo::logInfo("Loaded mesh: " + filePath.string()
                  + " (" + std::to_string(mesh->submeshes.size()) + " sub-meshes, "
                  + std::to_string(mesh->vertices.size()) + " verts, "
                  + std::to_string(mesh->indices.size()) + " indices)");

    return mesh;
}

}; // namespace codex