module heightmap_shared;

import common;

public struct HeightmapPushConstants {
    public CameraData* camera;
    public float2 origin;
    public float patchSize;
    public uint patchesPerSide;
};

public struct HeightmapVertex {
    public float4 position : SV_Position;
    public float3 worldPos;
    public float3 normal;
};

// Per-patch constants
public static const uint QUADS_PER_SIDE = 8;
public static const uint VERTS_PER_SIDE = QUADS_PER_SIDE + 1;
public static const uint VERT_COUNT = VERTS_PER_SIDE * VERTS_PER_SIDE;
public static const uint TRI_COUNT = QUADS_PER_SIDE * QUADS_PER_SIDE * 2;