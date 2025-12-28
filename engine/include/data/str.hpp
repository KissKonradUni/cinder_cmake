#pragma once

#include <cstddef>
#include <stdint.h>

// A simple C-style string structure
// Used in places where complex types are not allowed (e.g. ecs)

namespace cinder {

struct str {
    char* data;
    size_t length;
    size_t capacity;

    str() : data(nullptr), length(0), capacity(0) {}
};

void str_init(str& str, size_t capacity = 0);
void str_resize(str& s, size_t new_capacity);
void str_free(str& s);

void str_copy(str& dest, const str& src, size_t max_bytes = 1024);
void str_copy(str& dest, const char* cstr, size_t max_bytes = 1024);

}