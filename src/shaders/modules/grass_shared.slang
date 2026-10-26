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
    public uint bladeCount;
    public uint gridWidth;
    public float areaSize;
    public uint seed;
    public uint terrainType;
};

public struct GrassDrawPushConstants {
    public CameraData *camera;
    public CameraData *cullCamera;
    public Blade *blades;
    public uint *visibleCounter;
    public uint bladeCount;
    public uint cullFlags;
    public float distanceCullingMaxDistance;
};

public struct GrassSimPushConstants {
    public Blade *blades;
    public uint bladeCount;
    public float deltaTime;
    public float totalTime;
    public float windStrength;
    public float gustStrength;
    public float gravity;
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