import heightmap_shared;

[shader("fragment")]
float4 fragMain(HeightmapVertex input) : SV_Target
{
    float3 n = normalize(input.normal);
    float3 lightDir = normalize(float3(0.4, 1.0, 0.3));
    float diffuse = saturate(dot(n, lightDir));
    float3 grassColor = float3(0.15, 0.35, 0.15);
    float3 rockColor = float3(0.25, 0.3, 0.2);
    float rockAmount = smoothstep(0.91, 0.77, n.y);
    float3 albedo = lerp(grassColor, rockColor, rockAmount);
    return float4(albedo * (0.3 + 0.7 * diffuse), 1.0);
}