#include "layers/renderer.hpp"
#include "data/math.hpp"

#include "slang.h"

#include <fstream>

namespace prism {

slang::IGlobalSession* g_slangGlobalSession = nullptr;

SDL_GPUShader* vertexGpuShader = nullptr;
SDL_GPUShader* fragmentGpuShader = nullptr;

void RendererLayer::loadShader(const std::filesystem::path& shaderPath) {
    // Init
    if (g_slangGlobalSession == nullptr) {
        slang::createGlobalSession(&g_slangGlobalSession);
    }

    slang::SessionDesc slangSessionDesc = {};
    slang::TargetDesc slangTargetDesc = {};

    slangTargetDesc.format = SLANG_SPIRV; // TODO: Make this platform dependent
    slangTargetDesc.profile = g_slangGlobalSession->findProfile("spriv_1_5");

    slangSessionDesc.targets = &slangTargetDesc;
    slangSessionDesc.targetCount = 1;
    slang::CompilerOptionEntry compilerOptions[] = {{
        slang::CompilerOptionName::EmitSpirvDirectly, { slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr }
    }};
    slangSessionDesc.compilerOptionEntries = compilerOptions;
    slangSessionDesc.compilerOptionEntryCount = 1;

    const char* slangSearchPaths[] = {
        "assets/shaders"
    };
    slangSessionDesc.searchPaths = slangSearchPaths;
    slangSessionDesc.searchPathCount = 1;

    // Create session
    slang::ISession* session = nullptr;
    g_slangGlobalSession->createSession(slangSessionDesc, &session);

    // Load modules
    slang::IModule* slangModule = nullptr;
    slang::IBlob* diagnosticBlob = nullptr;
    slangModule = session->loadModule(shaderPath.string().c_str(), &diagnosticBlob);
    if (diagnosticBlob) {
        echo::logError(std::format("Shader compilation error: {}", (const char*)diagnosticBlob->getBufferPointer()));
    }

    if (!slangModule) {
        echo::logError(std::format("Failed to load shader module from path: {}", shaderPath.string()));
        return;
    }

    // Query entry points
    slang::IEntryPoint* vertexEntryPoint = nullptr;
    slangModule->findAndCheckEntryPoint("vertexMain", SlangStage::SLANG_STAGE_VERTEX, &vertexEntryPoint, &diagnosticBlob);
    if (diagnosticBlob) {
        echo::logError(std::format("Vertex shader entry point error: {}", (const char*)diagnosticBlob->getBufferPointer()));
    }

    if (!vertexEntryPoint) {
        echo::logError("Failed to find vertex shader entry point.");
        return;
    }

    slang::IEntryPoint* fragmentEntryPoint = nullptr; 
    slangModule->findAndCheckEntryPoint("fragmentMain", SlangStage::SLANG_STAGE_FRAGMENT, &fragmentEntryPoint, &diagnosticBlob);
    if (diagnosticBlob) {
        echo::logError(std::format("Fragment shader entry point error: {}", (const char*)diagnosticBlob->getBufferPointer()));
    }

    if (!fragmentEntryPoint) {
        echo::logError("Failed to find fragment shader entry point.");
        return;
    }

    // Compose modules + entry points into a program
    slang::IComponentType* componentTypes[] = {
        slangModule,
        vertexEntryPoint,
        fragmentEntryPoint
    };

    slang::IComponentType* program = nullptr;
    auto result = session->createCompositeComponentType(componentTypes, 3, &program, &diagnosticBlob);
    if (diagnosticBlob) {
        echo::logError(std::format("Shader composition error: {}", (const char*)diagnosticBlob->getBufferPointer()));
    }

    if (!program || SLANG_FAILED(result)) {
        echo::logError("Failed to create shader program.");
        return;
    }

    // Linking
    slang::IComponentType* linkedProgram = nullptr;
    result = program->link(&linkedProgram, &diagnosticBlob);
    if (diagnosticBlob) {
        echo::logError(std::format("Shader linking error: {}", (const char*)diagnosticBlob->getBufferPointer()));
    }

    if (!linkedProgram || SLANG_FAILED(result)) {
        echo::logError("Failed to link shader program.");
        return;
    }

    // Get target kernel
    slang::IBlob* vertexKernelBlob = nullptr;
    result = linkedProgram->getEntryPointCode(0, 0, &vertexKernelBlob, &diagnosticBlob);
    if (diagnosticBlob) {
        echo::logError(std::format("Shader code retrieval error: {}", (const char*)diagnosticBlob->getBufferPointer()));
    }

    if (!vertexKernelBlob || SLANG_FAILED(result)) {
        echo::logError("Failed to retrieve shader code.");
        return;
    }

    echo::logInfo(std::format("Vertex shader loaded and compiled successfully. ({} bytes)", vertexKernelBlob->getBufferSize()));

    slang::IBlob* fragmentKernelBlob = nullptr;
    result = linkedProgram->getEntryPointCode(1, 0, &fragmentKernelBlob, &diagnosticBlob);
    if (diagnosticBlob) {
        echo::logError(std::format("Shader code retrieval error: {}", (const char*)diagnosticBlob->getBufferPointer()));
    }

    if (!fragmentKernelBlob || SLANG_FAILED(result)) {
        echo::logError("Failed to retrieve shader code.");
        return;
    }

    echo::logInfo(std::format("Fragment shader loaded and compiled successfully. ({} bytes)", fragmentKernelBlob->getBufferSize()));

    // Save SPIR-V binary to file for inspection
    std::filesystem::path vertexOutputPath = shaderPath.parent_path() / (shaderPath.stem().string() + ".vert.spv");
    std::ofstream vertexOutputFile(vertexOutputPath, std::ios::binary);
    vertexOutputFile.write((const char*)vertexKernelBlob->getBufferPointer(), vertexKernelBlob->getBufferSize());
    vertexOutputFile.close();

    std::filesystem::path fragmentOutputPath = shaderPath.parent_path() / (shaderPath.stem().string() + ".frag.spv");
    std::ofstream fragmentOutputFile(fragmentOutputPath, std::ios::binary);
    fragmentOutputFile.write((const char*)fragmentKernelBlob->getBufferPointer(), fragmentKernelBlob->getBufferSize());
    fragmentOutputFile.close();

    // Create GPU shader objects
    SDL_GPUShaderCreateInfo vertexCreateInfo = {
        .code_size = vertexKernelBlob->getBufferSize(),
        .code = (const uint8_t*)vertexKernelBlob->getBufferPointer(),
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = SDL_GPU_SHADERSTAGE_VERTEX,
        .num_samplers = 0,
        .num_storage_textures = 0,
        .num_storage_buffers = 0,
        .num_uniform_buffers = 1,
        .props = 0
    };
    vertexGpuShader = SDL_CreateGPUShader(m_gpuDevice->getInternal(), &vertexCreateInfo);

    SDL_GPUShaderCreateInfo fragmentCreateInfo = {
        .code_size = fragmentKernelBlob->getBufferSize(),
        .code = (const uint8_t*)fragmentKernelBlob->getBufferPointer(),
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
        .num_samplers = 0,
        .num_storage_textures = 0,
        .num_storage_buffers = 0,
        .num_uniform_buffers = 1,
        .props = 0
    };
    fragmentGpuShader = SDL_CreateGPUShader(m_gpuDevice->getInternal(), &fragmentCreateInfo);
}

SDL_GPUBuffer* quadVertexBuffer = nullptr;
SDL_GPUBuffer* quadIndexBuffer = nullptr;
SDL_GPUBuffer* drawIndirectBuffer = nullptr;

/*
#define VERTEX_COUNT 4
#define INDEX_COUNT  6

Vertex quadVertices[VERTEX_COUNT] = {
    { vec3(-0.5f, -0.5f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(0.0f, 0.0f) },
    { vec3( 0.5f, -0.5f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(1.0f, 0.0f) },
    { vec3( 0.5f,  0.5f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(1.0f, 1.0f) },
    { vec3(-0.5f,  0.5f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(0.0f, 1.0f) }
};

uint32_t quadIndices[INDEX_COUNT] = {
    0, 1, 2,
    2, 3, 0
};
*/

SDL_GPUIndexedIndirectDrawCommand drawCommands[1] = { 0 };

struct UniformData {
    float boundsMin[4]; // xyz = min, w = padding (std140 alignment)
    float boundsMax[4]; // xyz = max, w = padding
    float time;
    float _pad[3];      // pad to 16-byte multiple
};
UniformData frameUniformData = {};

void RendererLayer::createQuad(std::unique_ptr<codex::LoadedMesh>& mesh) {
    uint32_t vertexCount = static_cast<uint32_t>(mesh->submeshes[0].vertexCount);
    uint32_t indexCount = static_cast<uint32_t>(mesh->submeshes[0].indexCount);

    drawCommands[0] = {
        .num_indices = indexCount,
        .num_instances = 1,
        .first_index = 0,
        .vertex_offset = 0,
        .first_instance = 0
    };
    frameUniformData.boundsMin[0] = mesh->quantizationBounds.min.x;
    frameUniformData.boundsMin[1] = mesh->quantizationBounds.min.y;
    frameUniformData.boundsMin[2] = mesh->quantizationBounds.min.z;
    frameUniformData.boundsMin[3] = 0.0f;
    frameUniformData.boundsMax[0] = mesh->quantizationBounds.max.x;
    frameUniformData.boundsMax[1] = mesh->quantizationBounds.max.y;
    frameUniformData.boundsMax[2] = mesh->quantizationBounds.max.z;
    frameUniformData.boundsMax[3] = 0.0f;

    // Vertex buffer init
    SDL_GPUBufferCreateInfo createInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = static_cast<Uint32>(sizeof(codex::P_Vertex) * vertexCount),
        .props = 0
    };
    quadVertexBuffer = SDL_CreateGPUBuffer(m_gpuDevice->getInternal(), &createInfo);
    if (!quadVertexBuffer) {
        echo::logError(std::format("Failed to create quad vertex buffer. Error: {}", SDL_GetError()));
        return;
    }
    
    SDL_GPUTransferBufferCreateInfo transferCreateInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = static_cast<Uint32>(sizeof(codex::P_Vertex) * vertexCount),
        .props = 0
    };
    auto vertexTransferBuffer = SDL_CreateGPUTransferBuffer(
        m_gpuDevice->getInternal(), 
        &transferCreateInfo
    );
    
    SDL_GPUTransferBufferLocation vertexTransferLocation = {
        .transfer_buffer = vertexTransferBuffer,
        .offset = 0,
    };
    auto transferPtr = SDL_MapGPUTransferBuffer(m_gpuDevice->getInternal(), vertexTransferBuffer, false);
    SDL_memcpy(transferPtr, mesh->vertices.data(), sizeof(codex::P_Vertex) * vertexCount);
    SDL_UnmapGPUTransferBuffer(m_gpuDevice->getInternal(), vertexTransferBuffer);
    
    // Index buffer init
    createInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    createInfo.size = sizeof(uint32_t) * indexCount;
    quadIndexBuffer = SDL_CreateGPUBuffer(m_gpuDevice->getInternal(), &createInfo);
    if (!quadIndexBuffer) {
        echo::logError(std::format("Failed to create quad index buffer. Error: {}", SDL_GetError()));
        return;
    }

    transferCreateInfo.size = sizeof(uint32_t) * indexCount;
    auto indexTransferBuffer = SDL_CreateGPUTransferBuffer(
        m_gpuDevice->getInternal(), 
        &transferCreateInfo
    );

    SDL_GPUTransferBufferLocation indexTransferLocation = {
        .transfer_buffer = indexTransferBuffer,
        .offset = 0,
    };
    transferPtr = SDL_MapGPUTransferBuffer(m_gpuDevice->getInternal(), indexTransferBuffer, false);
    SDL_memcpy(transferPtr, mesh->indices.data(), sizeof(uint32_t) * indexCount);
    SDL_UnmapGPUTransferBuffer(m_gpuDevice->getInternal(), indexTransferBuffer);

    // Command buffer init
    createInfo.usage = SDL_GPU_BUFFERUSAGE_INDIRECT;
    createInfo.size = sizeof(SDL_GPUIndexedIndirectDrawCommand) * 1;
    drawIndirectBuffer = SDL_CreateGPUBuffer(m_gpuDevice->getInternal(), &createInfo);
    if (!drawIndirectBuffer) {
        echo::logError(std::format("Failed to create draw indirect buffer. Error: {}", SDL_GetError()));
        return;
    }

    transferCreateInfo.size = sizeof(SDL_GPUIndexedIndirectDrawCommand) * 1;
    auto indirectTransferBuffer = SDL_CreateGPUTransferBuffer(
        m_gpuDevice->getInternal(), 
        &transferCreateInfo
    );

    SDL_GPUTransferBufferLocation indirectTransferLocation = {
        .transfer_buffer = indirectTransferBuffer,
        .offset = 0,
    };
    transferPtr = SDL_MapGPUTransferBuffer(m_gpuDevice->getInternal(), indirectTransferBuffer, false);
    SDL_memcpy(transferPtr, drawCommands, sizeof(SDL_GPUIndexedIndirectDrawCommand) * 1);
    SDL_UnmapGPUTransferBuffer(m_gpuDevice->getInternal(), indirectTransferBuffer);

    // Upload
    auto commandBuffer = SDL_AcquireGPUCommandBuffer(m_gpuDevice->getInternal());
    auto copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    
    SDL_GPUBufferRegion copyRegion = {
        .buffer = quadVertexBuffer,
        .offset = 0,
        .size = static_cast<Uint32>(sizeof(codex::P_Vertex) * vertexCount)
    };
    SDL_UploadToGPUBuffer(
        copyPass,
        &vertexTransferLocation,
        &copyRegion,
        false
    );

    copyRegion.buffer = quadIndexBuffer;
    copyRegion.size = sizeof(uint32_t) * indexCount;
    SDL_UploadToGPUBuffer(
        copyPass,
        &indexTransferLocation,
        &copyRegion,
        false
    );

    copyRegion.buffer = drawIndirectBuffer;
    copyRegion.size = sizeof(SDL_GPUIndexedIndirectDrawCommand) * 1;
    SDL_UploadToGPUBuffer(
        copyPass,
        &indirectTransferLocation,
        &copyRegion,
        false
    );

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
}

SDL_GPUGraphicsPipeline* graphicsPipeline = nullptr;
SDL_GPUTexture* depthTexture = nullptr;
SDL_GPUTextureCreateInfo depthTextureInfo = {
    .type             = SDL_GPU_TEXTURETYPE_2D,
    .format           = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
    .usage            = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
    .width            = 1,
    .height           = 1,
    .layer_count_or_depth = 1,
    .num_levels       = 1,
    .sample_count     = SDL_GPU_SAMPLECOUNT_1,
    .props            = 0
};
int viewportWidth = 800;
int viewportHeight = 600;
bool framebufferResized = false;

void RendererLayer::createPipeline() {
    // Create depth texture sized to the window
    SDL_GetWindowSize(m_window->getInternal(), &viewportWidth, &viewportHeight);
    depthTextureInfo.width = viewportWidth;
    depthTextureInfo.height = viewportHeight;
    depthTexture = SDL_CreateGPUTexture(m_gpuDevice->getInternal(), &depthTextureInfo);
    if (!depthTexture) {
        echo::logError(std::format("Failed to create depth texture. Error: {}", SDL_GetError()));
        return;
    }

    SDL_GPUVertexBufferDescription bufferDescription[1] = {
        {
            .slot = 0,
            .pitch = sizeof(codex::P_Vertex),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            .instance_step_rate = 0
        }
    };

    SDL_GPUVertexAttribute vertexAttributes[3] = {
        {
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_USHORT4_NORM, // uint16 [0,65535] -> float [0,1]
            .offset = offsetof(codex::P_Vertex, position)
        },
        {
            .location = 1,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_SHORT2_NORM,  // int16 [-32767,32767] -> float [-1,1]
            .offset = offsetof(codex::P_Vertex, normal)
        },
        {
            .location = 2,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_USHORT2_NORM, // uint16 [0,65535] -> float [0,1]
            .offset = offsetof(codex::P_Vertex, uv)
        }
    };

    SDL_GPUColorTargetDescription colorTargetDescriptions[1] = {
        {
            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB,
            .blend_state = {
                .src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
                .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
                .color_blend_op = SDL_GPU_BLENDOP_ADD,
                .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
                .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
                .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                .color_write_mask = 0x0,
                .enable_blend = false,
                .enable_color_write_mask = false
            }
        }
    };

    SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .vertex_shader = vertexGpuShader,
        .fragment_shader = fragmentGpuShader,
        .vertex_input_state = {
            .vertex_buffer_descriptions = bufferDescription,
            .num_vertex_buffers = 1,
            .vertex_attributes = vertexAttributes,
            .num_vertex_attributes = 3,
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = {
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .cull_mode = SDL_GPU_CULLMODE_BACK,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
            .depth_bias_constant_factor = 1.0f,
            .depth_bias_clamp = 0.0f,
            .depth_bias_slope_factor = 1.0f,
            .enable_depth_bias = false,
            .enable_depth_clip = true
        },
        .multisample_state = {
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
            .sample_mask = 0x0,
            .enable_mask = false,
            .enable_alpha_to_coverage = false
        },
        .depth_stencil_state = {
            .compare_op = SDL_GPU_COMPAREOP_LESS,
            .back_stencil_state = {
                .fail_op = SDL_GPU_STENCILOP_KEEP,
                .pass_op = SDL_GPU_STENCILOP_KEEP,
                .depth_fail_op = SDL_GPU_STENCILOP_KEEP,
                .compare_op = SDL_GPU_COMPAREOP_ALWAYS,
            },
            .front_stencil_state = {
                .fail_op = SDL_GPU_STENCILOP_KEEP,
                .pass_op = SDL_GPU_STENCILOP_KEEP,
                .depth_fail_op = SDL_GPU_STENCILOP_KEEP,
                .compare_op = SDL_GPU_COMPAREOP_ALWAYS,
            },
            .compare_mask = 0x0,
            .write_mask = 0x0,
            .enable_depth_test = true,
            .enable_depth_write = true,
            .enable_stencil_test = false
        },
        .target_info = {
            .color_target_descriptions = colorTargetDescriptions,
            .num_color_targets = 1,
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
            .has_depth_stencil_target = true
        },
        .props = 0
    };

    graphicsPipeline = SDL_CreateGPUGraphicsPipeline(m_gpuDevice->getInternal(), &pipelineCreateInfo);
    if (!graphicsPipeline) {
        echo::logError(std::format("Failed to create graphics pipeline. Error: {}", SDL_GetError()));
        return;
    }
}

// TODO: Asset and shader manager
void RendererLayer::loadAssets() {
    loadShader("assets/shaders/newFormat");

    // Temporary test for mesh loading
    auto meshPath = std::filesystem::current_path() / "assets/models/plastic_monobloc_chair_01_1k.gltf";
    auto outputPath = std::filesystem::current_path() / "assets/models/plastic_monobloc_chair_01_1k.cemf";

    codex::LoadedMesh::convertUsingAssimp(meshPath, outputPath);
    auto loadedMesh = codex::LoadedMesh::loadFromFile(outputPath);
    if (loadedMesh) {
        echo::logInfo(std::format("Mesh loaded successfully: {} vertices, {} indices.", loadedMesh->vertices.size(), loadedMesh->indices.size()));
    } else {
        echo::logError("Failed to load mesh.");
        return;
    }

    createQuad(loadedMesh);
    createPipeline();
}

PhaseState RendererLayer::onAttach() {
    loadAssets();

    return PhaseState::Continue;
};

PhaseState RendererLayer::onDetach() {
    if (g_slangGlobalSession) {
        slang::shutdown();
        g_slangGlobalSession = nullptr;
    }

    return PhaseState::Continue;
};

EventState RendererLayer::onEvent(SDL_Event* event) {
    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        SDL_GetWindowSize(m_window->getInternal(), &viewportWidth, &viewportHeight);
        framebufferResized = true;
    }
    
    return EventState::Propagate;
};

PhaseState RendererLayer::onPrepareFrame() {
    return PhaseState::Continue;
};

PhaseState RendererLayer::onRenderFrame(SDL_GPUCommandBuffer** commandBuffer, SDL_GPUTexture** swapchainTexture) {
    if (framebufferResized) {
        SDL_ReleaseGPUTexture(m_gpuDevice->getInternal(), depthTexture);
        depthTextureInfo.width = viewportWidth;
        depthTextureInfo.height = viewportHeight;
        depthTexture = SDL_CreateGPUTexture(m_gpuDevice->getInternal(), &depthTextureInfo);
        if (!depthTexture) {
            echo::logError(std::format("Failed to recreate depth texture after resize. Error: {}", SDL_GetError()));
            return PhaseState::Continue;
        }

        framebufferResized = false;
    }
    
    SDL_GPUColorTargetInfo colorTargetInfo = { 
        .texture = *swapchainTexture,
        .mip_level = 0,
        .layer_or_depth_plane = 0,
        .clear_color = (SDL_FColor){ 0.5f, 0.5f, 0.5f, 1.0f },
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .cycle = false
    };
    SDL_GPUDepthStencilTargetInfo depthTargetInfo = {
        .texture          = depthTexture,
        .clear_depth      = 1.0f,
        .load_op          = SDL_GPU_LOADOP_CLEAR,
        .store_op         = SDL_GPU_STOREOP_DONT_CARE,
        .stencil_load_op  = SDL_GPU_LOADOP_DONT_CARE,
        .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
        .cycle            = false,
        .clear_stencil    = 0
    };
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(*commandBuffer, &colorTargetInfo, 1, &depthTargetInfo);

    SDL_BindGPUGraphicsPipeline(renderPass, graphicsPipeline);
    
    float time = m_host->getTime().totalTime;
    frameUniformData.time = time;
    SDL_PushGPUVertexUniformData(*commandBuffer, 0, &frameUniformData, sizeof(frameUniformData));
    SDL_PushGPUFragmentUniformData(*commandBuffer, 0, &frameUniformData, sizeof(frameUniformData));

    int viewportWidth, viewportHeight;
    SDL_GetWindowSize(m_window->getInternal(), &viewportWidth, &viewportHeight);
    SDL_GPUViewport viewport = {
        .x = 0,
        .y = 0,
        .w = viewportWidth  * 1.0f,
        .h = viewportHeight * 1.0f,
        .min_depth = 0.0f,
        .max_depth = 1.0f
    };
    SDL_SetGPUViewport(renderPass, &viewport);
    
    SDL_GPUBufferBinding vertexBufferBindings[1] = {
        {
            .buffer = quadVertexBuffer,
            .offset = 0,
        }
    };
    SDL_BindGPUVertexBuffers(renderPass, 0, vertexBufferBindings, 1);
    SDL_GPUBufferBinding indexBufferBinding = {
        .buffer = quadIndexBuffer,
        .offset = 0,
    };
    SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
    SDL_DrawGPUIndexedPrimitivesIndirect(
        renderPass,
        drawIndirectBuffer,
        0,
        1
    );

    SDL_EndGPURenderPass(renderPass);

    return PhaseState::Continue;
};


}; // namespace prism