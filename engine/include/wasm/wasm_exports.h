#pragma once

#define MODULE "cinder"
#include "wasm_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

// ================================================
// PUBLIC API DEFINITIONS
// ================================================

FUNCTION_IMPORT(void, put, wasm_ptr_t string, wasm_size_t length)

// ================================================

#ifdef __cplusplus
}
#endif