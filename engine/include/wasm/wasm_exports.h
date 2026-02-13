#pragma once

#define MODULE "cinder"
#include "wasm_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

// ================================================
// PUBLIC API DEFINITIONS
// ================================================

// Put a string from WASM to the host application (Cinder)
// @param string: pointer to the string in WASM memory 
// @param length: length of the string
FUNCTION_IMPORT(void, put, wasm_ptr_t string, wasm_size_t length)

// ================================================

#ifdef __cplusplus
}
#endif