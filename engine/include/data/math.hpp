#pragma once

// TODO: this is garbo rn

struct vec2 { float x, y; };
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