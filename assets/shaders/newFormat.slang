struct Vertex
{
    float4 position;  // pre-normalized by vertex fetch
    float2 normal;    // pre-normalized by vertex fetch
    float2 uv;        // pre-normalized by vertex fetch
}

struct VOut
{
    float4 position : SV_Position;
    float3 normal;
    float2 uv;
}

struct UniformValues
{
    float4 boundsMin;       // .xyz = quantizationBounds.min, .w unused
    float4 boundsMax;       // .xyz = quantizationBounds.max, .w unused
    float  time;
}

[[vk::binding(0, 3)]]
ConstantBuffer<UniformValues> uniforms;

// Decode an octahedral-encoded normal back to a unit float3
float3 octahedralDecode(float2 f)
{
    float3 n = float3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    if (n.z < 0.0)
    {
        float2 s = float2(n.x >= 0.0 ? 1.0 : -1.0,
                          n.y >= 0.0 ? 1.0 : -1.0);
        n = float3((1.0 - abs(n.yx)) * s, n.z);
    }
    return normalize(n);
}

// Temporary static camera: model at origin, camera pulled back 5 units on Z
float4x4 buildViewProj(float aspect)
{
    float nearZ = 0.1;
    float farZ  = 100.0;
    float fovY  = 0.7854; // 45 degrees
    float f     = 1.0 / tan(fovY * 0.5);

    // Depth [0,1] for Vulkan/SDL_GPU
    float A  = farZ / (nearZ - farZ);
    float B  = nearZ * farZ / (nearZ - farZ);
    float tz = -3.0; // translate model 3 units in front of the camera

    // Combined projection * translation (no lookAt, camera looks down -Z)
    // P*T applied to a column vector:
    //   clip.x = (f/aspect) * x
    //   clip.y = f * y
    //   clip.z = A*(z+tz) + B
    //   clip.w = -(z+tz)
    return float4x4(
        f / aspect, 0.0,  0.0,  0.0,
        0.0,        f,    0.0,  0.0,
        0.0,        0.0,  A,    A * tz + B,
        0.0,        0.0, -1.0, -tz
    );
}

[shader("vertex")]
VOut vertexMain(Vertex input)
{
    // Position arrives as [0, 1] from USHORT4_NORM; remap to world space
    float3 pos = uniforms.boundsMin.xyz + input.position.xyz * (uniforms.boundsMax.xyz - uniforms.boundsMin.xyz);

    // Normal arrives as [-1, 1] from SHORT2_NORM; decode octahedral
    float3 normal = octahedralDecode(input.normal);

    float4x4 viewProj = buildViewProj(16.0 / 9.0);

    VOut output;
    output.position = mul(viewProj, float4(pos, 1.0));
    output.normal   = normal;
    output.uv       = input.uv;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VOut input) : SV_Target
{
    let third = float.getPi() * 2.0 / 3.0;
    let red   = 0.5 * sin(uniforms.time);
    let green = 0.5 * sin(uniforms.time + third) + 0.5;
    let blue  = 0.5 * sin(uniforms.time + third * 2.0) + 0.5;

    let lightPos = float3(cos(uniforms.time) * 5.0, sin(uniforms.time) * 5.0, 5.0);
    let diffuse  = max(0.0, dot(input.normal, normalize(lightPos)));

    return float4(float3(red, green, blue) * diffuse, 1.0);
}
