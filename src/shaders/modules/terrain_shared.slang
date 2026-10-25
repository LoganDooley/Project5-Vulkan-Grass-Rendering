module terrain_shared;

import common;

static const int OCTAVES = 4;
static const float TERRAIN_AMPLITUDE = 25.0f;
static const float TERRAIN_SCALE = 80.0f;

float hash2(int2 p) {
    uint h = pcgHash(uint(p.x) ^ pcgHash(uint(p.y)));
    return float(h) / float(uint.maxValue);
}

float valueNoise(float2 p) {
    float2 i = floor(p);
    float2 f = p - i; // Fractional offset

    // Calculate corner indices
    int2 c00 = int2(i);
    int2 c10 = int2(i) + int2(1, 0);
    int2 c01 = int2(i) + int2(0, 1);
    int2 c11 = int2(i) + int2(1, 1);

    // Get hash values at corners (noise)
    float a = hash2(c00);
    float b = hash2(c10);
    float c = hash2(c01);
    float d = hash2(c11);

    // Quintic curve equation
    float2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

    // Bilinear interpolate and shift to 0 -> 1 range
    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y) * 2.0 - 1.0;
}

float fbm(float2 p) {
    float sum = 0.0;
    float amplitude = 0.5;

    for (int o = 0; o < OCTAVES; o++) {
        sum += amplitude * valueNoise(p);
        amplitude *= 0.5;
        p = p * 2.0 + float2(17.3, -9.1);
    }

    return sum;
}

public float terrainHeight(float2 xz) {
    return TERRAIN_AMPLITUDE * fbm(xz / TERRAIN_SCALE);
}

public float3 terrainNormal(float2 xz) {
    const float e = 0.1;
    float hx = terrainHeight(xz + float2(e, 0)) - terrainHeight(xz - float2(e, 0));
    float hz = terrainHeight(xz + float2(0, e)) - terrainHeight(xz - float2(0, e));
    return normalize(float3(-hx, 2.0 * e, -hz));
}