module common;

public struct CameraData {
    public float4x4 view;
    public float4x4 proj;
    public float4x4 viewProj;
    public float4 position;
};

public struct PushConstants {
    public CameraData *camera;
    public float4 *positions;
};

public struct AnimPushConstants {
    public float4 *positions;
    public float time;
};