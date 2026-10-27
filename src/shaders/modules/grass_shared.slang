module grass_shared;

import common;

public struct Blade {
    public float4 v0;
    public float4 v1;
    public float4 v2;
    public float4 up;
};

public struct GrassUpdatePushConstants {
    public Blade *blades;
    public int2 base; // Coordnate of first grass cell in world space
    public int2 prevBase; // Coordinate of first grass cell last frame
    public int2 baseMod; // base % gridWidth. This is passed from the CPU since negative mod is defined in c++
    public int2 prevBaseMod; // prevBase % gridWidth
    public uint bladeCount;
    public uint gridWidth; // How many cells there are per side
    public float cellSize; // Each cell holds one blade
    public uint seed;
    public uint terrainType;
    public uint forceAll; // (boolean) forces all cells to reinitialize
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