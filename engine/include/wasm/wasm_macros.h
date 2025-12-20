#pragma once

#include <stdint.h>

#ifndef MODULE
#error "MODULE must be defined before including wasm_macros.h"
#endif

typedef uint32_t wasm_ptr_t;
typedef uint32_t wasm_size_t;

#ifdef INNER_FACING
#include "wasm_export.h"

#define FUNCTION_IMPORT(ret, name, ...) \
    ret name(wasm_exec_env_t env, __VA_ARGS__);
#else 
#define FUNCTION_IMPORT(ret, name, ...) \
    ret __attribute__((import_module(MODULE), import_name(#name))) name(__VA_ARGS__);

#define WASM_EXPORT(name) \
    __attribute__((export_name(#name))) name
#endif