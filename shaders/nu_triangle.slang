// explicit float2 array initialization
// static means its the same for all calls to the shader
static float2 positions[3] = float2[](
    float2(0.0, -0.5),
    float2(0.5, 0.5),
    float2(-0.5, 0.5)
);

static float3 colors[3] = float3[](
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0)
);

// this combines all the info for each vertex
struct VertexOutput {
    float3 color;
    float4 sv_position : SV_position; // SV_Position is the output
}

// shader function called for each vertex
[shader("vertex")]
VertexOutput vertMain(uint vid : SV_VertexID) { // SV_VertexID is the ID of the vertex being processed, usually a vertex buffer index
    VertexOutput output;
    output.sv_position = float4(positions[vid], 0.0, 1.0);
    output.color = colors[vid];
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput in_vert) : SV_Target
{
    float3 color = in_vert.color;
    // colors in slang are RGBA in [0,1] range
    return float4(color, 1.0);
}