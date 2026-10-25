import common;
import ground_shared;

[[vk::push_constant]] GroundPushConstants pc;

static const float HALF_SIZE = 150.0;

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(4, 1, 1)]
void meshMain(uint threadId: SV_GroupThreadID,
              out vertices GroundVertex verts[4],
              out indices uint3 tris[2])
{
    SetMeshOutputCounts(4, 2);
    let corners : float2[4] = {
        float2(-1, -1),
        float2(1, -1),
        float2(1, 1),
        float2(-1, 1)
    };

    float3 world = float3(corners[threadId].x * HALF_SIZE,
                          0.0,
                          corners[threadId].y * HALF_SIZE);
    verts[threadId].position = mul(pc.camera->viewProj, float4(world, 1.0));
    verts[threadId].worldPos = world;
    if (threadId == 0) {
        tris[0] = uint3(0, 1, 2);
        tris[1] = uint3(0, 2, 3);
    }
}