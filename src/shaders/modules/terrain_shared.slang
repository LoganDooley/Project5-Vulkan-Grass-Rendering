module terrain_shared;

import common;

public static const uint TERRAIN_PLANE = 0;
public static const uint TERRAIN_HEIGHTMAP = 1;

static const int OCTAVES = 6;

// Rolling hills
static const float HILL_SCALE = 120.0;
static const float HILL_AMPLITUDE = 8.0;

// Mountains
static const float MOUNTAIN_SCALE = 400.0f;
static const float MOUNTAIN_AMPLITUDE = 140.0;

// Mountain mask
static const float MASK_SCALE = 1200.0;

// Domain warping
static const float WARP_SCALE = 250.0;
static const float WARP_STRENGTH = 60.0;

public static const float GRASS_SLOPE_FULL_COS = 0.91;
public static const float GRASS_SLOPE_NONE_COS = 0.85;

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

float ridgeNoise(float2 p) {
    float n = 1.0 - abs(valueNoise(p));
    return n * n;
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

float ridgeFbm(float2 p) {
    float sum = 0.0;
    float amplitude = 0.5;
    float weight = 1.0;

    for (int o = 0; o < OCTAVES; o++) {
        float n = ridgeNoise(p);
        n *= weight;
        weight = saturate(n * 2.0); // If the previous octave had a low value, then start to smooth it out (helps make valleys more smooth)
        sum += amplitude * n;
        amplitude *= 0.5;
        p = p * 2.0 + float2(17.3, -9.1);
    }
    return sum;
}

public float terrainHeight(float2 xz) {
    // Do domain warping
    float2 warp = float2(fbm(xz / WARP_SCALE), fbm(xz / WARP_SCALE + float2(5.2, 1.3))) * WARP_STRENGTH;
    float2 p = xz + warp;

    // Use fbm for hills
    float hills = HILL_AMPLITUDE * fbm(p / HILL_SCALE);
    // Use ridge fbm for mountains
    float mountains = MOUNTAIN_AMPLITUDE * ridgeFbm(p / MOUNTAIN_SCALE);
    // Use value noise to make distinct regions of mountains and hills
    float mask = smoothstep(-0.1, 0.5, valueNoise(xz / MASK_SCALE));
    return hills + mask * mountains;
}

public float3 terrainNormal(float2 xz) {
    const float e = 0.1;
    float hx = terrainHeight(xz + float2(e, 0)) - terrainHeight(xz - float2(e, 0));
    float hz = terrainHeight(xz + float2(0, e)) - terrainHeight(xz - float2(0, e));
    return normalize(float3(-hx, 2.0 * e, -hz));
}