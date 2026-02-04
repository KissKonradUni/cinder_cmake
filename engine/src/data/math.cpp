#include "data/math.hpp"
#include "SDL3/SDL_stdinc.h"

namespace hex {

// vec2

vec2 vec2::operator+(const vec2& other) const {
    return vec2{ x + other.x, y + other.y };
}
vec2 vec2::operator-(const vec2& other) const {
    return vec2{ x - other.x, y - other.y };
}
vec2 vec2::operator*(const vec2& other) const {
    return vec2{ x * other.x, y * other.y };
}

vec2 vec2::operator*(float scalar) const {
    return vec2{ x * scalar, y * scalar };
}
vec2 vec2::operator/(float scalar) const {
    return vec2{ x / scalar, y / scalar };
}

vec2& vec2::operator+=(const vec2& other) {
    x += other.x;
    y += other.y;
    return *this;
}
vec2& vec2::operator-=(const vec2& other) {
    x -= other.x;
    y -= other.y;
    return *this;
}
vec2& vec2::operator*=(const vec2& other) {
    x *= other.x;
    y *= other.y;
    return *this;
}

vec2& vec2::operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
    return *this;
}
vec2& vec2::operator/=(float scalar) {
    x /= scalar;
    y /= scalar;
    return *this;
}

inline float vec2::lengthSquared() const {
    return x * x + y * y;
}
inline float vec2::length() const {
    return SDL_sqrtf(lengthSquared());
}
inline vec2 vec2::normalized() const {
    float len = length();
    if (len == 0.0f) {
        return vec2::zero();
    }
    return *this / len;
}
inline vec2& vec2::normalizeInPlace() {
    float len = length();
    if (len != 0.0f) {
        *this /= len;
    }
    return *this;
}

inline float vec2::dot(const vec2& a, const vec2& b) {
    return a.x * b.x + a.y * b.y;
}

inline vec3 vec2::cross(const vec2& a, const vec2& b) {
    float crossX = 0.0f;
    float crossY = 0.0f;
    float crossZ = a.x * b.y - a.y * b.x;
    return vec3(crossX, crossY, crossZ);
}

inline vec2 vec2::copy(const vec2& v) {
    return vec2{ v.x, v.y };
}

inline vec2 vec2::copy() const {
    return vec2{ x, y };
}

// vec3

vec3 vec3::operator+(const vec3& other) const {
    return vec3(_mm_add_ps(simd, other.simd));
}
vec3 vec3::operator-(const vec3& other) const {
    return vec3(_mm_sub_ps(simd, other.simd));
}
vec3 vec3::operator*(const vec3& other) const {
    return vec3(_mm_mul_ps(simd, other.simd));
}

vec3 vec3::operator*(float scalar) const {
    return vec3(_mm_mul_ps(simd, _mm_set1_ps(scalar)));
}
vec3 vec3::operator/(float scalar) const {
    return vec3(_mm_div_ps(simd, _mm_set1_ps(scalar)));
}

vec3& vec3::operator+=(const vec3& other) {
    simd = _mm_add_ps(simd, other.simd);
    return *this;
}
vec3& vec3::operator-=(const vec3& other) {
    simd = _mm_sub_ps(simd, other.simd);
    return *this;
}
vec3& vec3::operator*=(const vec3& other) {
    simd = _mm_mul_ps(simd, other.simd);
    return *this;
}

vec3& vec3::operator*=(float scalar) {
    simd = _mm_mul_ps(simd, _mm_set1_ps(scalar));
    return *this;
}
vec3& vec3::operator/=(float scalar) {
    simd = _mm_div_ps(simd, _mm_set1_ps(scalar));
    return *this;
}

inline float vec3::lengthSquared() const {
    return x * x + y * y + z * z;
}
inline float vec3::length() const {
    return SDL_sqrtf(lengthSquared());
}
inline vec3 vec3::normalized() const {
    float len = length();
    if (len == 0.0f) {
        return vec3::zero();
    }
    return *this / len;
}
inline vec3& vec3::normalizeInPlace() {
    float len = length();
    if (len != 0.0f) {
        *this /= len;
    }
    return *this;
}

inline float vec3::dot(const vec3& a, const vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline vec3 vec3::cross(const vec3& a, const vec3& b) {
    float crossX = a.y * b.z - a.z * b.y;
    float crossY = a.z * b.x - a.x * b.z;
    float crossZ = a.x * b.y - a.y * b.x;
    return vec3(crossX, crossY, crossZ);
}

inline vec3 vec3::copy(const vec3& v) {
    return vec3(v.simd);
}
inline vec3 vec3::copy() const {
    return vec3(simd);
}

// vec4

// quaternion

// mat4

// mat3

};