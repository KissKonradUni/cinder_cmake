#pragma once

#include "data/str.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

/**
 * This header defines the binary format for mesh files used in the engine.
 * The format is designed to be compact and efficient for loading at runtime, while also being flexible enough to accommodate future extensions.
 * The vertex data may be uploaded as is, as the format can be recalculated in the Vertex shader.
 *
 * The usual file structure is as follows:
 * [MeshFileHeader][MeshChunkHeader][ChunkData]...[MeshChunkHeader][ChunkData]
 *
 * Each chunk is linked to the next via the nextChunkOffset field in the MeshChunk
 * A tipical mesh would contain all of it's chunks in the following order:
 * [SubmeshDataChunk][VertexDataChunk][IndexDataChunk][MaterialDataChunk]
 * 
 * And each file may contain multiple meshes.
 */

namespace codex {

struct __attribute__((packed)) P_Quaternion {
    float x;
    float y;
    float z;
    float w;
};

struct __attribute__((packed)) P_Vector3 {
    float x;
    float y;
    float z;
};

struct __attribute__((packed)) P_String {
    uint32_t length; // Length of the string in bytes (including null terminator)
    //uint8_t* data;   // Raw data member for string data (UTF-8 encoded, null-terminated)
};

struct __attribute__((packed)) P_AABB {
    P_Vector3 min;
    P_Vector3 max;
};

struct __attribute__((packed)) P_Transform {
    P_Vector3    position;
    P_Quaternion rotation;
    P_Vector3    scale;
};

struct __attribute__((packed)) MeshFileHeader {
    uint32_t magic;            // "CEMF" (Cinder Engine Mesh File)
    uint16_t version;          // 1 for now
    uint16_t flags;            // Bitfield for future use

    uint32_t fileSize;         // Total byte size to read
    uint32_t meshCount;        // Number of meshes in the file

    uint32_t firstChunkOffset; // Byte offset to the first chunk (from the start of the file)

    P_AABB wholeBoundingBox;   // AABB encompassing the entire mesh
    P_AABB quantizationBounds; // AABB used for quantization of vertex positions
};

enum class MeshChunkType : uint8_t {
    SubmeshData = 0x01,
    VertexData = 0x02,
    IndexData = 0x03,
    MaterialData = 0x04,
    // Future chunk types can be added here
};

struct __attribute__((packed)) MeshChunkHeader {
    MeshChunkType type;       // Type of the chunk
    uint32_t size;            // Size of the chunk data in bytes (excluding this header)
    uint32_t meshID;          // ID of the mesh this chunk belongs to
    uint32_t nextChunkOffset; // Byte offset to the next chunk (0 if this is the last chunk) (from the start of the file)
};

static constexpr uint32_t SUBMESH_NO_PARENT = 0xFFFFFFFF;

struct __attribute__((packed)) SubmeshDataChunk {
    P_Transform transform;       // Local transform for this submesh
    P_AABB submeshBoundingBox;   // AABB for this submesh
    uint32_t parentSubmeshID;    // Parent submesh ID (SUBMESH_NO_PARENT if root)
};

struct __attribute__((packed)) P_Vertex {
    uint16_t position[4]; // Quantized position (x, y, z)
    int16_t  normal[2];   // Octahedral encoded normal (x, y)
    uint16_t uv[2];       // Quantized UV coordinates (u, v)
};

struct __attribute__((packed)) VertexDataChunk {
    uint32_t vertexCount; // Number of vertices

    //P_Vertex* vertices;  // Raw data member for vertex data
};

struct __attribute__((packed)) IndexDataChunk {
    uint32_t indexCount; // Number of indices

    /*
    uint32_t* indices32; // Raw data member for 32-bit indices
    */
};

struct __attribute__((packed)) MaterialDataChunk {
    P_String diffuseTexturePath; // File path to the diffuse texture
    P_String normalTexturePath;  // File path to the normal texture
    P_String armTexturePath;     // File path to the ambient-roughness-metallic texture
};

struct LoadedMaterialInfo
{
    cinder::str diffuse;
    cinder::str normal;
    cinder::str arm;
};

struct LoadedSubmesh {
    P_Transform transform;
    P_AABB boundingBox;

    uint32_t vertexOffset;  // Offset into the global vertex buffer
    uint32_t vertexCount;

    uint32_t indexOffset;   // Offset into the global index buffer
    uint32_t indexCount;
    
    uint32_t materialIndex; // Index into the mesh's material array
    uint32_t parentIndex;   // Index into submeshes[] (SUBMESH_NO_PARENT if root)
};

struct LoadedMesh {
    P_AABB wholeBoundingBox; // AABB encompassing the entire mesh
    P_AABB quantizationBounds;
    
    std::vector<LoadedSubmesh> submeshes;
    std::vector<P_Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<LoadedMaterialInfo> materials;

    static void convertUsingAssimp(const std::filesystem::path& filePath, const std::filesystem::path& outputPath);
    static std::unique_ptr<LoadedMesh> loadFromFile(std::filesystem::path filePath);
};

}; // namespace codex