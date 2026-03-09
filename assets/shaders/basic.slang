struct Vertex
{
    float3 position;
    float3 normal;
    float2 uv;
}

struct VOut
{
    float4 position : SV_Position;
    float3 normal;
    float2 uv;
}

struct UniformValues
{
    float time;
}

[[vk::binding(0, 3)]]
ConstantBuffer<UniformValues> uniforms;

[shader("vertex")]
VOut vertexMain(Vertex input)
{
    VOut output;
    output.position = float4(input.position, 1.0);
    output.position.z += 0.25f;

    output.normal = input.normal;
    output.uv = input.uv;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VOut input) : SV_Target
{
    let third = float.getPi() * 2.0f / 3.0f;
    let red   = 0.5f * (sin(uniforms.time));
    let green = 0.5f * (sin(uniforms.time + third)) + 0.5f;
    let blue  = 0.5f * (sin(uniforms.time + third * 2.0f)) + 0.5f;

    let lightPos = float3(cos(uniforms.time) * 5.0f, sin(uniforms.time) * 5.0f, 5.0f);
    let diffuse = max(0.0f, dot(input.normal, normalize(lightPos)));

    return float4(float3(red, green, blue) * diffuse, 1.0f);
}