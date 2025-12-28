#include "data/str.hpp"

#include <string.h>
#include <cstdlib>
#include <print>

namespace cinder {

void str_init(str& str, size_t capacity) {
    if (str.data != nullptr) {
        std::println("Warning: initializing already initialized str, freeing first");
        str_free(str);
    }

    str.data = (char*)malloc(capacity);
    str.length = 0;
    str.capacity = capacity;
}

void str_resize(str& s, size_t new_capacity) {
    if (s.data == nullptr) {
        std::println("Warning: resizing uninitialized str, initializing instead");
        str_init(s, new_capacity);
        return;
    }

    s.data = (char*)realloc((void*)s.data, new_capacity);
    s.capacity = new_capacity;
}

void str_free(str& s) {
    if (s.data != nullptr) {
        free((void*)s.data);
        s.data = nullptr;
        s.length = 0;
        s.capacity = 0;
    } else {
        std::println("Warning: freeing uninitialized str");
    }
}

/**
 * @brief Copies the contents of src str to dest str
 * @note Tries to null-terminate if possible
 * 
 * @param dest The destination str
 * @param src The source str
 * @param max_bytes The maximum number of bytes to copy
 */
void str_copy(str& dest, const str& src, size_t max_bytes) {
    if (dest.data == nullptr) {
        std::println("Warning: copying to uninitialized str, initializing instead");
        str_init(dest, src.length < max_bytes ? src.length : max_bytes);
    }

    if (src.data == nullptr) {
        std::println("Warning: copying from uninitialized str, doing nothing");
        return;
    }

    const size_t bytes_to_copy = (src.length < max_bytes) ? src.length : max_bytes;
    if (dest.capacity < bytes_to_copy) {
        str_resize(dest, bytes_to_copy);
    }

    memcpy((void*)dest.data, (const void*)src.data, bytes_to_copy);
    dest.length = bytes_to_copy;

    // Null-terminate if possible
    if (dest.capacity > dest.length) {
        dest.data[dest.length] = '\0';
    }
}

/**
 * @brief Copies the contents of a C-style string to a str
 * @note Tries to null-terminate if possible
 * 
 * @param dest The destination str
 * @param cstr The source C-style string
 * @param max_bytes The maximum number of bytes to copy
 */
void str_copy(str& dest, const char* cstr, size_t max_bytes) {
    if (dest.data == nullptr) {
        std::println("Warning: copying to uninitialized str, initializing instead");
        str_init(dest, max_bytes);
    }

    if (cstr == nullptr) {
        std::println("Warning: copying from null cstr, doing nothing");
        return;
    }

    const size_t src_length = strnlen(cstr, max_bytes);
    const size_t bytes_to_copy = (src_length < max_bytes) ? src_length : max_bytes;
    if (bytes_to_copy == max_bytes && cstr[bytes_to_copy - 1] != '\0') {
        std::println("Warning: copying cstr without null-terminator, destination str will not be null-terminated");
    }

    if (dest.capacity < bytes_to_copy) {
        str_resize(dest, bytes_to_copy);
    }

    memcpy((void*)dest.data, (const void*)cstr, bytes_to_copy);
    dest.length = bytes_to_copy;

    // Null-terminate if possible
    if (dest.capacity > dest.length) {
        dest.data[dest.length] = '\0';
    }
}

}