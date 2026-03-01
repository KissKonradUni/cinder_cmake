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

struct PushConstants
{
    float time;
}

[[vk::binding(0, 3)]]
ConstantBuffer<PushConstants> pc;

[shader("vertex")]
VOut vertexMain(Vertex input)
{
    VOut output;
    output.position = float4(input.position, 1.0);
    output.normal = input.normal;
    output.uv = input.uv;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VOut input) : SV_Target
{
    let color = float4((pc.time + input.uv.x) % 1.0, input.uv.y, abs(sin(pc.time * 0.5f)), 1.0f);
    return color;
}