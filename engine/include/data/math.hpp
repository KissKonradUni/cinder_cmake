#pragma once
#include <xmmintrin.h>

// TODO: Actual math lib

namespace hex {

// Forward declare vec3 for cross product
struct vec3;

struct vec2 { 
    union alignas(8) {
        float data[2];
        struct { float x, y; };
    };

    vec2(): x(0.0f), y(0.0f) {}
    vec2(float x, float y) : x(x), y(y) {}
    // No range check
    vec2(float* arr) : x(arr[0]), y(arr[1]) {}

    static constexpr const vec2 zero()  { return vec2{ 0.0f,  0.0f}; }
    static constexpr const vec2 one()   { return vec2{ 1.0f,  1.0f}; }
    static constexpr const vec2 right() { return vec2{ 1.0f,  0.0f}; }
    static constexpr const vec2 up()    { return vec2{ 0.0f,  1.0f}; }
    static constexpr const vec2 left()  { return vec2{-1.0f,  0.0f}; }
    static constexpr const vec2 down()  { return vec2{ 0.0f, -1.0f}; }

    vec2 operator+(const vec2& other) const;
    vec2 operator-(const vec2& other) const;
    vec2 operator*(const vec2& other) const;
    
    vec2 operator*(float scalar) const;
    vec2 operator/(float scalar) const;
    
    vec2& operator+=(const vec2& other);
    vec2& operator-=(const vec2& other);
    vec2& operator*=(const vec2& other);
    
    vec2& operator*=(float scalar);
    vec2& operator/=(float scalar);

    inline float lengthSquared() const;
    inline float length() const;
    inline vec2 normalized() const;
    inline vec2& normalizeInPlace();

    static inline float dot(const vec2& a, const vec2& b);
    static inline vec3 cross(const vec2& a, const vec2& b);
    
    static inline vec2 copy(const vec2& v);
    inline vec2 copy() const;
};

struct vec3 {
    union alignas(16) {
        float data[4]; // Allows access to the padding
        struct { 
            float x, y, z; 
            float _padding; // Allow access to the padding
        };
        __m128 simd;
    };

    vec3(): x(0.0f), y(0.0f), z(0.0f), _padding(0.0f) {}
    vec3(float x, float y, float z) : x(x), y(y), z(z), _padding(0.0f) {}
    // No range check
    vec3(float* arr) : x(arr[0]), y(arr[1]), z(arr[2]), _padding(0.0f) {} 
    vec3(__m128 simd) : simd(simd) {}

    static constexpr const vec3 zero()    { return vec3{ 0.0f,  0.0f,  0.0f}; }
    static constexpr const vec3 one()     { return vec3{ 1.0f,  1.0f,  1.0f}; }
    static constexpr const vec3 right()   { return vec3{ 1.0f,  0.0f,  0.0f}; }
    static constexpr const vec3 up()      { return vec3{ 0.0f,  1.0f,  0.0f}; }
    static constexpr const vec3 forward() { return vec3{ 0.0f,  0.0f,  1.0f}; }
    static constexpr const vec3 left()    { return vec3{-1.0f,  0.0f,  0.0f}; }
    static constexpr const vec3 down()    { return vec3{ 0.0f, -1.0f,  0.0f}; }
    static constexpr const vec3 back()    { return vec3{ 0.0f,  0.0f, -1.0f}; }

    vec3 operator+(const vec3& other) const;
    vec3 operator-(const vec3& other) const;
    vec3 operator*(const vec3& other) const;
    
    vec3 operator*(float scalar) const;
    vec3 operator/(float scalar) const;
    
    vec3& operator+=(const vec3& other);
    vec3& operator-=(const vec3& other);
    vec3& operator*=(const vec3& other);
    
    vec3& operator*=(float scalar);
    vec3& operator/=(float scalar);

    inline float lengthSquared() const;
    inline float length() const;
    inline vec3 normalized() const;
    inline vec3& normalizeInPlace();

    static inline float dot(const vec3& a, const vec3& b);
    static inline vec3 cross(const vec3& a, const vec3& b);
    
    static inline vec3 copy(const vec3& v);
    inline vec3 copy() const;
};

struct alignas(16) vec4 {
    union alignas(16) {
        float data[4]; // Allows access to the padding
        struct { 
            float x, y, z, w; 
            float _padding; // Allow access to the padding
        };
        __m128 simd;
    };
};
struct alignas(16) quaternion { float x, y, z, w; };

struct mat4 { 
    union {
        float m[16];
        struct {
            vec4 row0;
            vec4 row1;
            vec4 row2;
            vec4 row3;
        };
        struct {
            float m00, m01, m02, m03;
            float m10, m11, m12, m13;
            float m20, m21, m22, m23;
            float m30, m31, m32, m33;
        };
    };
};

struct mat3 { 
    union {
        float m[9];
        struct {
            vec3 row0;
            vec3 row1;
            vec3 row2;
        };
        struct {
            float m00, m01, m02;
            float m10, m11, m12;
            float m20, m21, m22;
        };
    };
};

}; // namespace hex