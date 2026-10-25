import common;
import heightmap_shared;
import terrain_shared;

[[vk::push_constant]] HeightmapPushConstants pc;

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(VERT_COUNT, 1, 1)]
void meshMain(uint3 groupId: SV_GroupID,
              uint3 threadId: SV_GroupThreadId,
              out vertices HeightmapVertex verts[VERT_COUNT],
              out indices uint3 tris[TRI_COUNT])
{
    SetMeshOutputCounts(VERT_COUNT, TRI_COUNT);

    uint t = threadId.x;

    int col = t % VERTS_PER_SIDE;
    int row = t / VERTS_PER_SIDE;
    float3 worldPos = float3(0.0);
    worldPos.xz = pc.origin + (float2(groupId.xy) + float2(col, row) / QUADS_PER_SIDE) * pc.patchSize;
    worldPos.y = terrainHeight(worldPos.xz);
    float3 normal = terrainNormal(worldPos.xz);

    verts[t].position = mul(pc.camera->viewProj, float4(worldPos, 1.0));
    verts[t].worldPos = worldPos;
    verts[t].normal = normal;

    if (t < QUADS_PER_SIDE * QUADS_PER_SIDE) {
        int2 quad = int2(t % QUADS_PER_SIDE, t / QUADS_PER_SIDE);
        int i00 = quad.y * VERTS_PER_SIDE + quad.x;
        int i10 = i00 + 1;
        int i01 = i00 + VERTS_PER_SIDE;
        int i11 = i01 + 1;

        tris[2 * t] = uint3(i00, i10, i01);
        tris[2 * t + 1] = uint3(i01, i10, i11);
    }
}