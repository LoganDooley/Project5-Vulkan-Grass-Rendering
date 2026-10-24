import common;
import grass_shared;

[[vk::push_constant]] GrassDrawPushConstants pc;

static const uint SEGMENTS = 7;
static const uint VERTS = 2 * (SEGMENTS + 1);
static const uint TRIS = 2 * SEGMENTS;

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(VERTS, 1, 1)]
void meshMain(uint3 groupId: SV_GroupID,
              uint3 threadId: SV_GroupThreadId,
              in payload GrassPayload p,
              out vertices GrassVertex verts[VERTS],
              out indices uint3 tris[TRIS])
{
    SetMeshOutputCounts(VERTS, TRIS);

    Blade blade = pc.blades[p.bladeIndices[groupId.x]];

    uint t = threadId.x;

    // Find which vertex this thread is constructing
    float v = float(t / 2) / float(SEGMENTS);
    float u = float(t % 2);

    // Get blade properties
    float3 v0 = blade.v0.xyz;
    float3 v1 = blade.v1.xyz;
    float3 v2 = blade.v2.xyz;
    float3 up = blade.up.xyz;
    float orientation = blade.v0.w;
    float width = blade.v2.w;

    // Calculate parameters from the paper
    float3 a = lerp(v0, v1, v);
    float3 b = lerp(v1, v2, v);
    float3 c = lerp(a, b, v);

    // t1 is the bitangent from the orientation
    float3 t1 = float3(cos(orientation), 0.0, sin(orientation));
    float3 c0 = c - width * t1;
    float3 c1 = c + width * t1;

    float3 tangent = b - a;
    // Use a fallback if normalization would fail
    float3 t0 = dot(tangent, tangent) > 1e-8 ? normalize(tangent) : up;

    // Triangle shape
    float shape = u + 0.5 * v - u * v;
    float3 pos = lerp(c0, c1, shape);

    verts[t].position = mul(pc.camera->viewProj, float4(pos, 1.0));
    verts[t].normal = normalize(cross(t0, t1));
    verts[t].v = v;

    if (t < TRIS) {
        // There are more verts than tris, only record one triangle per tri
        uint row = t / 2;
        uint i0 = 2 * row;
        uint i1 = i0 + 1;
        uint i2 = i0 + 2;
        uint i3 = i0 + 3;

        // Alternate between tris in quads
        tris[t] = (t % 2 == 0) ? uint3(i0, i1, i2) : uint3(i1, i3, i2);
    }
}