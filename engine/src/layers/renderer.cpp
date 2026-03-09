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
        .num_uniform_buffers = 0,
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

struct Vertex {
    vec3 position;
    vec3 normal;
    vec2 uv;
};

#define USE_CUBE

#ifdef USE_CUBE
#define VERTEX_COUNT 24
#define INDEX_COUNT  36

Vertex quadVertices[VERTEX_COUNT] = {
    // Face 1
    { vec3(-0.238129f, -0.350876f, -0.087637f), vec3(-0.8145f, -0.0149f, -0.5800f), vec2(0.375000f, 0.000000f) },
    { vec3(-0.405827f,  0.063021f,  0.137232f), vec3(-0.8145f, -0.0149f, -0.5800f), vec2(0.625000f, 0.000000f) },
    { vec3(-0.169106f,  0.343438f, -0.202370f), vec3(-0.8145f, -0.0149f, -0.5800f), vec2(0.625000f, 0.250000f) },
    { vec3(-0.001408f, -0.070460f, -0.427239f), vec3(-0.8145f, -0.0149f, -0.5800f), vec2(0.375000f, 0.250000f) },
    // Face 2
    { vec3(-0.001408f, -0.070460f, -0.427239f), vec3( 0.4734f,  0.5608f, -0.6792f), vec2(0.375000f, 0.250000f) },
    { vec3(-0.169106f,  0.343438f, -0.202370f), vec3( 0.4734f,  0.5608f, -0.6792f), vec2(0.625000f, 0.250000f) },
    { vec3( 0.238129f,  0.350876f,  0.087637f), vec3( 0.4734f,  0.5608f, -0.6792f), vec2(0.625000f, 0.500000f) },
    { vec3( 0.405827f, -0.063021f, -0.137232f), vec3( 0.4734f,  0.5608f, -0.6792f), vec2(0.375000f, 0.500000f) },
    // Face 3
    { vec3( 0.405827f, -0.063021f, -0.137232f), vec3( 0.8145f,  0.0149f,  0.5800f), vec2(0.375000f, 0.500000f) },
    { vec3( 0.238129f,  0.350876f,  0.087637f), vec3( 0.8145f,  0.0149f,  0.5800f), vec2(0.625000f, 0.500000f) },
    { vec3( 0.001408f,  0.070460f,  0.427239f), vec3( 0.8145f,  0.0149f,  0.5800f), vec2(0.625000f, 0.750000f) },
    { vec3( 0.169106f, -0.343438f,  0.202370f), vec3( 0.8145f,  0.0149f,  0.5800f), vec2(0.375000f, 0.750000f) },
    // Face 4
    { vec3( 0.169106f, -0.343438f,  0.202370f), vec3(-0.4734f, -0.5608f,  0.6792f), vec2(0.375000f, 0.750000f) },
    { vec3( 0.001408f,  0.070460f,  0.427239f), vec3(-0.4734f, -0.5608f,  0.6792f), vec2(0.625000f, 0.750000f) },
    { vec3(-0.405827f,  0.063021f,  0.137232f), vec3(-0.4734f, -0.5608f,  0.6792f), vec2(0.625000f, 1.000000f) },
    { vec3(-0.238129f, -0.350876f, -0.087637f), vec3(-0.4734f, -0.5608f,  0.6792f), vec2(0.375000f, 1.000000f) },
    // Face 5
    { vec3(-0.001408f, -0.070460f, -0.427239f), vec3( 0.3354f, -0.8278f, -0.4497f), vec2(0.125000f, 0.500000f) },
    { vec3( 0.405827f, -0.063021f, -0.137232f), vec3( 0.3354f, -0.8278f, -0.4497f), vec2(0.375000f, 0.500000f) },
    { vec3( 0.169106f, -0.343438f,  0.202370f), vec3( 0.3354f, -0.8278f, -0.4497f), vec2(0.375000f, 0.750000f) },
    { vec3(-0.238129f, -0.350876f, -0.087637f), vec3( 0.3354f, -0.8278f, -0.4497f), vec2(0.125000f, 0.750000f) },
    // Face 6
    { vec3( 0.238129f,  0.350876f,  0.087637f), vec3(-0.3354f,  0.8278f,  0.4497f), vec2(0.625000f, 0.500000f) },
    { vec3(-0.169106f,  0.343438f, -0.202370f), vec3(-0.3354f,  0.8278f,  0.4497f), vec2(0.875000f, 0.500000f) },
    { vec3(-0.405827f,  0.063021f,  0.137232f), vec3(-0.3354f,  0.8278f,  0.4497f), vec2(0.875000f, 0.750000f) },
    { vec3( 0.001408f,  0.070460f,  0.427239f), vec3(-0.3354f,  0.8278f,  0.4497f), vec2(0.625000f, 0.750000f) },
};

uint32_t quadIndices[INDEX_COUNT] = {
     0,  1,  2,   0,  2,  3,
     4,  5,  6,   4,  6,  7,
     8,  9, 10,   8, 10, 11,
    12, 13, 14,  12, 14, 15,
    16, 17, 18,  16, 18, 19,
    20, 21, 22,  20, 22, 23,
};

#else
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

#endif

SDL_GPUIndexedIndirectDrawCommand drawCommands[1] = {
    {
        .num_indices = INDEX_COUNT,
        .num_instances = 1,
        .first_index = 0,
        .vertex_offset = 0,
        .first_instance = 0
    }
};

void RendererLayer::createQuad() {
    // Vertex buffer init
    SDL_GPUBufferCreateInfo createInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = sizeof(Vertex) * VERTEX_COUNT,
        .props = 0
    };
    quadVertexBuffer = SDL_CreateGPUBuffer(m_gpuDevice->getInternal(), &createInfo);
    if (!quadVertexBuffer) {
        echo::logError(std::format("Failed to create quad vertex buffer. Error: {}", SDL_GetError()));
        return;
    }
    
    SDL_GPUTransferBufferCreateInfo transferCreateInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = sizeof(Vertex) * VERTEX_COUNT,
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
    SDL_memcpy(transferPtr, quadVertices, sizeof(Vertex) * VERTEX_COUNT);
    SDL_UnmapGPUTransferBuffer(m_gpuDevice->getInternal(), vertexTransferBuffer);
    
    // Index buffer init
    createInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    createInfo.size = sizeof(uint32_t) * INDEX_COUNT;
    quadIndexBuffer = SDL_CreateGPUBuffer(m_gpuDevice->getInternal(), &createInfo);
    if (!quadIndexBuffer) {
        echo::logError(std::format("Failed to create quad index buffer. Error: {}", SDL_GetError()));
        return;
    }

    transferCreateInfo.size = sizeof(uint32_t) * INDEX_COUNT;
    auto indexTransferBuffer = SDL_CreateGPUTransferBuffer(
        m_gpuDevice->getInternal(), 
        &transferCreateInfo
    );

    SDL_GPUTransferBufferLocation indexTransferLocation = {
        .transfer_buffer = indexTransferBuffer,
        .offset = 0,
    };
    transferPtr = SDL_MapGPUTransferBuffer(m_gpuDevice->getInternal(), indexTransferBuffer, false);
    SDL_memcpy(transferPtr, quadIndices, sizeof(uint32_t) * INDEX_COUNT);
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
        .size = sizeof(Vertex) * VERTEX_COUNT
    };
    SDL_UploadToGPUBuffer(
        copyPass,
        &vertexTransferLocation,
        &copyRegion,
        false
    );

    copyRegion.buffer = quadIndexBuffer;
    copyRegion.size = sizeof(uint32_t) * INDEX_COUNT;
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

void RendererLayer::createPipeline() {
    SDL_GPUVertexBufferDescription bufferDescription[1] = {
        {
            .slot = 0,
            .pitch = sizeof(Vertex),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            .instance_step_rate = 0
        }
    };

    SDL_GPUVertexAttribute vertexAttributes[3] = {
        {
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = offsetof(Vertex, position)
        },
        {
            .location = 1,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = offsetof(Vertex, normal)
        },
        {
            .location = 2,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            .offset = offsetof(Vertex, uv)
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
            .enable_depth_test = false,
            .enable_depth_write = false,
            .enable_stencil_test = false
        },
        .target_info = {
            .color_target_descriptions = colorTargetDescriptions,
            .num_color_targets = 1,
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
            .has_depth_stencil_target = false
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
    loadShader("assets/shaders/basic");
    createQuad();
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
    return EventState::Propagate;
};

PhaseState RendererLayer::onPrepareFrame() {
    return PhaseState::Continue;
};

PhaseState RendererLayer::onRenderFrame(SDL_GPUCommandBuffer** commandBuffer, SDL_GPUTexture** swapchainTexture) {
    SDL_GPUColorTargetInfo colorTargetInfo = { 
        .texture = *swapchainTexture,
        .mip_level = 0,
        .layer_or_depth_plane = 0,
        .clear_color = (SDL_FColor){ 0.5f, 0.5f, 0.5f, 1.0f },
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .cycle = false
    };
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(*commandBuffer, &colorTargetInfo, 1, NULL);

    SDL_BindGPUGraphicsPipeline(renderPass, graphicsPipeline);
    
    float time = m_host->getTime().totalTime;
    SDL_PushGPUFragmentUniformData(*commandBuffer, 0, &time, sizeof(float));
    
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