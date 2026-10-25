module ground_shared;

import common;

public struct GroundVertex {
    public float4 position : SV_Position;
    public float3 worldPos;
};

public struct GroundPushConstants {
    public CameraData *camera;
};