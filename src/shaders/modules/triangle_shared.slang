module triangle_shared;

public struct Payload {
    public uint index;
};

public struct VertexOut {
    public float4 position : SV_Position;
    public float3 color;
};