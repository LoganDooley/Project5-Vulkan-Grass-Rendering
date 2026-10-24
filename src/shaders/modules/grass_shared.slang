module grass_shared;

import common;

public struct Blade {
    public float4 v0;
    public float4 v1;
    public float4 v2;
    public float4 up;
};

public struct GrassInitPushConstants {
    public Blade *blades;
    public uint32_t bladeCount;
    public uint32_t gridWidth;
    public float areaSize;
    public uint32_t seed;
};

public struct GrassDrawPushConstants {
    public CameraData *camera;
    public Blade *blades;
    public uint bladeCount;
};

public struct GrassSimPushConstants {
    public Blade *blades;
    public uint bladeCount;
    public float deltaTime;
    public float totalTime;
};

public static const uint BLADES_PER_TASK_GROUP = 32;
public struct GrassPayload {
    public uint bladeIndices[BLADES_PER_TASK_GROUP];
};

public struct GrassVertex {
    public float4 position : SV_Position;
    public float3 normal;
    public float v;
};

// From https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
public uint pcgHash(uint x) {
    uint state = x * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

public float random01(inout uint state) {
    // Update state
    state = pcgHash(state);
    return float(state) / float(uint.maxValue);
    //return float(state) / 4294967296.0;
}