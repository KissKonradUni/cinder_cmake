#include "wasm_exports.h"

void WASM_EXPORT(example)() {
    char buffer[255] = "Hello from WebAssembly!";
    wasm_size_t length = 255;
    
    put((wasm_ptr_t)buffer, length);
}

uint32_t WASM_EXPORT(fib)(uint32_t n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

void WASM_EXPORT(trigger_error)() {
    // This will cause an out-of-bounds memory access
    char* invalidPtr = (char*)0xFFFFFFFF;
    invalidPtr[0] = 'X';
}