#include "layers/wasm.hpp"

#include "imgui.h"

#define INNER_FACING
#include "wasm/wasm_exports.h"

#include <cstring>
#include <fstream>
#include <vector>
#include <print>

namespace hex {

using namespace std::filesystem;

PhaseState WasmLayer::onAttach() {
    wasm_runtime_init();

    if (!exists(m_bytecodePath)) {
        std::println("WASM bytecode file does not exist: {}", m_bytecodePath.string());
        return PhaseState::Failure;
    }

    std::ifstream file(m_bytecodePath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::println("Failed to open WASM bytecode file: {}", m_bytecodePath.string());
        return PhaseState::Failure;
    }

    auto fileSize = file.tellg();
    m_bytecodeBuffer.resize(fileSize);
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(m_bytecodeBuffer.data()), fileSize);

    // TODO: expose api functions
    m_nativeSymbols = {
        EXPORT_WASM_API_WITH_SIG(put, "(ii)")
    };
    if (!wasm_runtime_register_natives("cinder", m_nativeSymbols.data(), m_nativeSymbols.size())) {
        std::println("Failed to register native symbols for WASM module.");
        return PhaseState::Failure;
    }

    m_module = wasm_runtime_load(m_bytecodeBuffer.data(), m_bytecodeBuffer.size(), m_errorBuffer.data(), m_errorBuffer.size());
    if (m_module == NULL) {
        std::println("Failed to load WASM module: {}", m_errorBuffer.data());
        return PhaseState::Failure;
    }

    m_moduleInstance = wasm_runtime_instantiate(m_module, m_stackSize, m_heapSize, m_errorBuffer.data(), m_errorBuffer.size());
    if (m_moduleInstance == NULL) {
        std::println("Failed to instantiate WASM module: {}", m_errorBuffer.data());
        wasm_runtime_unload(m_module);
        return PhaseState::Failure;
    }

    m_execEnvironment = wasm_runtime_create_exec_env(m_moduleInstance, m_stackSize);
    if (m_execEnvironment == NULL) {
        std::println("Failed to create WASM execution environment");
        wasm_runtime_deinstantiate(m_moduleInstance);
        wasm_runtime_unload(m_module);
        return PhaseState::Failure;
    }

    return PhaseState::Success;
}

PhaseState WasmLayer::onDetach() {
    if (m_execEnvironment)
        wasm_runtime_destroy_exec_env(m_execEnvironment);
    if (m_moduleInstance)
        wasm_runtime_deinstantiate(m_moduleInstance);
    if (m_module)
        wasm_runtime_unload(m_module);

    wasm_runtime_destroy();
    
    return PhaseState::Success;
}

// TODO: Remove example calls and allow user to call functions as needed
PhaseState WasmLayer::onUpdate() {
    static bool firstTime = true;
    
    if (firstTime) {
        firstTime = false;
        
        if (auto func = tryGetFunction("example")) {
            call(*func);
        }

        if (auto func = tryGetFunction("fib")) {
            uint32_t n = 10;
            uint32_t result = call<uint32_t>(*func, n);
            std::println("WASM fib({}) = {}", n, result);
        }

        if (auto func = tryGetFunction("non_existent_function")) {
            call(*func);
        }

        if (auto func = tryGetFunction("trigger_error")) { 
            call(*func); 
        }

        if (auto func = tryGetFunction("example")) { 
            call(*func); 
        }
    }

    return PhaseState::Continue;
}

std::optional<wasm_function_inst_t> WasmLayer::tryGetFunction(const std::string& name) {
    if (m_moduleInstance == nullptr) {
        std::println("WASM module instance is not valid when trying to get function '{}'", name);
        return std::nullopt;
    }
    
    if (m_functionCache.find(name) != m_functionCache.end()) {
        return m_functionCache[name];
    }

    wasm_function_inst_t func = wasm_runtime_lookup_function(m_moduleInstance, name.c_str());
    if (func == nullptr) {
        std::println("Failed to find '{}' function in WASM module: {}", name, m_errorBuffer.data());
        return std::nullopt;
    }

    m_functionCache[name] = func;
    return func;
}

template<typename Ret>
Ret WasmLayer::call(wasm_function_inst_t func) {
    uint32_t stack[2] = {};

    if (!wasm_runtime_call_wasm(m_execEnvironment, func, 0, stack)) {
        std::println("WASM function call failed: {}", wasm_runtime_get_exception(m_moduleInstance));
        return Ret();
    }

    if constexpr (!std::is_void_v<Ret>)
        return static_cast<Ret>(stack[0]);
}

template<typename Ret, typename... Args>
Ret WasmLayer::call(wasm_function_inst_t func, Args... args) {
    static_assert(sizeof...(Args) <= 8);

    uint32_t stack[8] = {};
    std::memcpy(stack, &args..., sizeof...(Args) * sizeof(uint32_t));

    if (!wasm_runtime_call_wasm(m_execEnvironment, func, sizeof...(Args), stack)) {
        std::println("WASM function call failed: {}", wasm_runtime_get_exception(m_moduleInstance));
        return Ret();
    }

    if constexpr (!std::is_void_v<Ret>)
        return static_cast<Ret>(stack[0]);
}

void WasmLayer::restartRuntime() {
    if (m_execEnvironment) wasm_runtime_destroy_exec_env(m_execEnvironment);
    if (m_moduleInstance) wasm_runtime_deinstantiate(m_moduleInstance);

    m_moduleInstance = wasm_runtime_instantiate(m_module, m_stackSize, m_heapSize, m_errorBuffer.data(), m_errorBuffer.size());
    if (m_moduleInstance == NULL) {
        std::println("Failed to instantiate WASM module: {}", m_errorBuffer.data());
        wasm_runtime_unload(m_module);
        return;
    }

    m_execEnvironment = wasm_runtime_create_exec_env(m_moduleInstance, m_stackSize); 
    if (m_execEnvironment == NULL) { 
        std::println("Failed to create WASM execution environment"); 
        wasm_runtime_deinstantiate(m_moduleInstance); 
        wasm_runtime_unload(m_module); 
        return; 
    } 
    
    m_functionCache.clear();
}

} // namespace hex