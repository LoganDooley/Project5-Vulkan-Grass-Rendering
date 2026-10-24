import grass_shared;

[shader("fragment")]
float4 fragMain(GrassVertex input) : SV_Target
{
    float3 rootColor = float3(0.05, 0.25, 0.05);
    float3 tipColor = float3(0.45, 0.75, 0.25);
    float3 albedo = lerp(rootColor, tipColor, input.v);

    float3 lightDir = normalize(float3(0.4, 1.0, 0.3));
    float diffuse = abs(dot(normalize(input.normal), lightDir));
    return float4(albedo * (0.3 + 0.7 * diffuse), 1.0);
}