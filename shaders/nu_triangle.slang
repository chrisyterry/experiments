struct VertexInput {
    // vertex attributes; specified per vertex in vertex buffer
    float2 position; // vertex position
    float3 color;    // vertex color
}

// this combines all the info for each vertex
struct VertexOutput {
    float4 position : SV_position; // SV_Position is the output
    float3 color;
}

// shader function called for each vertex
[shader("vertex")]
VertexOutput vertMain(VertexInput input) {
    VertexOutput output;
    output.position = float4(input.position, 0.0, 1.0);
    output.color = input.color;
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput in_vert) : SV_Target
{
    float3 color = in_vert.color;
    // colors in slang are RGBA in [0,1] range
    return float4(color, 1.0);
}