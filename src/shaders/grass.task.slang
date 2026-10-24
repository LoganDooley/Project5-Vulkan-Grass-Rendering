import common;
import grass_shared;

[[vk::push_constant]] GrassDrawPushConstants pc;

groupshared GrassPayload payload;
groupshared uint visibleCount;

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

    bool visible = bladeIndex < pc.bladeCount; // Culling for later
    if (visible) {
        uint slot;
        // Increment visibleCount atomically
        InterlockedAdd(visibleCount, 1, slot);
        payload.bladeIndices[slot] = bladeIndex;
    }

    // Wait for group payload to be done
    GroupMemoryBarrierWithGroupSync();

    DispatchMesh(visibleCount, 1, 1, payload); // One per visible blade
}