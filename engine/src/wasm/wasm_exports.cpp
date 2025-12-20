#define INNER_FACING
#include "wasm/wasm_exports.h"

#include <print>

void put(wasm_exec_env_t env, uint32_t string, uint32_t length) {
    const wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(env);
    
    if (!wasm_runtime_validate_app_str_addr(module_inst, string)) {
        std::println("Invalid string address passed to put function");
        return;
    }

    if (!wasm_runtime_validate_app_addr(module_inst, string, length)) {
        std::println("Invalid string length passed to put function.");
        return;
    }

    const char* buffer = (char*)wasm_runtime_addr_app_to_native(module_inst, string);

    std::println("WASM put: {}", buffer);
}