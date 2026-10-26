import common;
import grass_shared;

[[vk::push_constant]] GrassDrawPushConstants pc;

groupshared GrassPayload payload;
groupshared uint visibleCount;

bool passesOrientation(float orientation, float3 v0)
{
    if ((pc.cullFlags & 1u) == 0) {
        return true;
    }

    float3 dirb = float3(cos(orientation), 0.0, sin(orientation));
    float3 cameraPos = pc.cullCamera.position.xyz;
    float3 dirc = normalize(v0 - cameraPos);
    return abs(dot(dirc, dirb)) <= 0.9;
}

bool pointInFrustum(float3 p) {
    float4 clip = mul(pc.cullCamera.viewProj, float4(p, 1.0));
    float h = clip.w + 1.0;
    return (all(clip.xyz >= -h) && all(clip.xyz <= h));
}

bool passesFrustum(float3 v0, float3 v1, float3 v2)
{
    if ((pc.cullFlags & 2u) == 0) {
        return true;
    }

    // eq 14
    float3 m = 0.25 * v0 + 0.5 * v1 + 0.25 * v2;

    return pointInFrustum(v0) || pointInFrustum(m) || pointInFrustum(v2);
}

bool passesDistance(float3 v0, float3 up, uint id)
{
    if ((pc.cullFlags & 4u) == 0) {
        return true;
    }

    float3 c = pc.cullCamera.position.xyz;
    // eq 19
    float dproj = length(v0 - c - up * dot(v0 - c, up));

    // eq 20
    float dmax = pc.distanceCullingMaxDistance;
    float keep = 1.0 - dproj / dmax;
    uint level = pcgHash(id) % 8u;
    return dproj <= dmax && level <= floor(8.0 * (1.0 - dproj / dmax));
}

[shader("amplification")]
[numthreads(BLADES_PER_TASK_GROUP, 1, 1)]
void taskMain(uint3 groupId: SV_GroupID,
              uint3 threadId: SV_GroupThreadID)
{
    uint localIndex = threadId.x;
    uint bladeIndex = groupId.x * BLADES_PER_TASK_GROUP + localIndex;

    if (localIndex == 0) {
        visibleCount = 0;
    }

    // Equivalent of sync threads in CUDA
    GroupMemoryBarrierWithGroupSync();

    if (bladeIndex < pc.bladeCount) {
        // Check visibility
        Blade blade = pc.blades[bladeIndex];
        float3 v0 = blade.v0.xyz;
        float3 v1 = blade.v1.xyz;
        float3 v2 = blade.v2.xyz;
        float3 up = blade.up.xyz;
        float orientation = blade.v0.w;
        float width = blade.v2.w;

        bool visible = blade.v1.w > 0.0 && passesOrientation(orientation, v0) && passesFrustum(v0, v1, v2) && passesDistance(v0, up, bladeIndex);
        if (visible) {
            uint slot;
            // Increment visibleCount atomically
            InterlockedAdd(visibleCount, 1, slot);
            payload.bladeIndices[slot] = bladeIndex;
        }
    }


    // Wait for group payload to be done
    GroupMemoryBarrierWithGroupSync();

    if (localIndex == 0) {
        InterlockedAdd(*pc.visibleCounter, visibleCount);
    }

    DispatchMesh(visibleCount, 1, 1, payload); // One per visible blade
}