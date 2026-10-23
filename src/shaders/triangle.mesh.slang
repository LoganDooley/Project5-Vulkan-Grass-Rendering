import common;
import triangle_shared;

[[vk::push_constant]] PushConstants pc;

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(3, 1, 1)] // One thread per vertex
void meshMain(uint threadId: SV_GroupThreadID,
              in payload Payload p,
              out vertices VertexOut verts[3],
              out indices uint3 tris[1])
{
    SetMeshOutputCounts(3, 1);
    let refPos : float2[3] = {
        float2(0, 0.5),
        float2(0.5, -0.5),
        float2(-0.5, -0.5)
    };

    let refColor : float3[3] = {
        float3(1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 1.0)
    };

    float3 offset = float3(1.0 * p.index, 0.0, -1.0 * p.index);

    verts[threadId].position = mul(pc.camera->viewProj, float4(float3(refPos[threadId], 0.0) + offset, 1.0));
    verts[threadId].color = refColor[threadId];

    if (threadId == 0) {
        tris[0] = uint3(0, 1, 2);
    }
}