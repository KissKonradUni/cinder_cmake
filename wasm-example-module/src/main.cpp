#include "wasm_exports.h"

static uint32_t counter = 0;

void int_to_str(uint32_t integer, char* buffer) {
    if (integer == 0) {
        buffer[0] = '0'; buffer[1] = '\0'; return; 
    } 
    char temp[11];
    int index = 0;
    while (integer > 0) {
        temp[index++] = '0' + (integer % 10); integer /= 10; 
    } 
    for (int i = 0; i < index; i++) 
    { 
        buffer[i] = temp[index - i - 1]; 
    } 
    buffer[index] = '\0'; 
}

void WASM_EXPORT(example)() {
    counter++;

    char buffer[255] = "Hello from WebAssembly! Counter: ";
    wasm_size_t length = 255;

    int_to_str(counter, buffer + 33);
    
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