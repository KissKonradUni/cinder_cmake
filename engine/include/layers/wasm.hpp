#pragma once

#include "../layer.hpp"

#include "wasm_export.h"

#include <filesystem>
#include <unordered_map>
#include <vector>

namespace hex {

using namespace cinder;

class WasmLayer : public Layer {
public:
    WasmLayer(std::filesystem::path bytecodePath): m_bytecodePath(std::move(bytecodePath)) {}
    ~WasmLayer() override = default;

    std::optional<wasm_function_inst_t> tryGetFunction(const std::string& name);

    template<typename Ret = void>
    Ret call(wasm_function_inst_t func);

    template<typename Ret = void, typename... Args>
    Ret call(wasm_function_inst_t func, Args... args);

    void restartRuntime();
protected:
    PhaseState onAttach() override;
    PhaseState onDetach() override;
    PhaseState onUpdate() override;

    uint32_t m_stackSize = 64 * 1024; // 64KB
    uint32_t m_heapSize = 1024 * 1024; // 1MB

    std::filesystem::path m_bytecodePath;
    std::vector<uint8_t> m_bytecodeBuffer;
    std::array<char, 255> m_errorBuffer;
    std::vector<NativeSymbol> m_nativeSymbols;
    
    wasm_module_t m_module = nullptr;
    wasm_module_inst_t m_moduleInstance = nullptr;
    wasm_exec_env_t m_execEnvironment = nullptr;

    std::unordered_map<std::string, wasm_function_inst_t> m_functionCache;
};

} // namespace hex