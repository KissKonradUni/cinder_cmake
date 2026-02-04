#pragma once

// TODO: Actual math lib

struct alignas(64) vec2 { 
    union {
        float data[2];
        struct { float x, y; };
    };

    static constexpr vec2 zero()  { return vec2{ 0.0f,  0.0f}; }
    static constexpr vec2 one()   { return vec2{ 1.0f,  1.0f}; }
    static constexpr vec2 right() { return vec2{ 1.0f,  0.0f}; }
    static constexpr vec2 up()    { return vec2{ 0.0f,  1.0f}; }
    static constexpr vec2 left()  { return vec2{-1.0f,  0.0f}; }
    static constexpr vec2 down()  { return vec2{ 0.0f, -1.0f}; }

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
};

struct vec3 { float x, y, z; };
struct vec4 { float x, y, z, w; };
struct quaternion { float x, y, z, w; };

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