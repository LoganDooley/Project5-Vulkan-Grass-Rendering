import ground_shared;

[shader("fragment")]
float4 fragMain(GroundVertex input) : SV_Target
{
    // Make a checkerboard pattern
    int2 cell = int2(floor(input.worldPos.xz));

    bool bDarkGreen = ((cell.x + cell.y) & 1) != 0;

    const float3 darkGreen = float3(0.18, 0.30, 0.12);
    const float3 lightGreen = float3(0.22, 0.36, 0.15);
    float3 color = bDarkGreen ? darkGreen : lightGreen;
    return float4(color, 1.0);
}