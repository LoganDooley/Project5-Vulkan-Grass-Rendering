import triangle_shared;

[shader("fragment")]
float4 fragMain(VertexOut input) : SV_Target {
    return float4(input.color, 1.0);
}