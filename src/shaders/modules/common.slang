module common;

public struct CameraData {
    public float4x4 view;
    public float4x4 proj;
    public float4x4 viewProj;
    public float4 position;
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
}