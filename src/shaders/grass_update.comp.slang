import common;

import grass_shared;
import terrain_shared;

[[vk::push_constant]] GrassUpdatePushConstants pc;

static const float MIN_HEIGHT = 0.6;
static const float MAX_HEIGHT = 1.2;
static const float MIN_WIDTH = 0.03;
static const float MAX_WIDTH = 0.05;
static const float MIN_STIFF = 20.0;
static const float MAX_STIFF = 25.0;

int wrapOffset(int slot, int baseMod, int W) {
    // Get offset from the base cell
    int d = slot - baseMod;
    return d < 0 ? d + W : d;
}

int2 slotToCell(int2 slot, int2 base, int2 baseMod, int W) {
    // Find what world space cell a slot corresponds to
    return base + int2(wrapOffset(slot.x, baseMod.x, W), wrapOffset(slot.y, baseMod.y, W));
}

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 threadId : SV_DispatchThreadID)
{
    // Global thread index
    uint i = threadId.x;
    if (i >= pc.bladeCount) {
        return;
    }

    int W = int(pc.gridWidth);
    // Find slot in grass blades
    int2 slot = int2(int(i) % W, int(i) / W);
    // Find world space cell
    int2 cell = slotToCell(slot, pc.base, pc.baseMod, W);

    if (pc.forceAll == 0) {
        // Check if cell hasn't changed, in which case we don't need to do anything
        int2 prevCell = slotToCell(slot, pc.prevBase, pc.prevBaseMod, W);
        if (all(prevCell == cell)) {
            return;
        }
    }

    // Create a pcg hash based on the cell and seed
    uint rng = pcgHash(uint(cell.x) ^ pcgHash(uint(cell.y) ^ pc.seed));

    // Stratified random sampling for position
    float2 jitter = float2(random01(rng), random01(rng));
    float2 xz = (float2(cell) + jitter) * pc.cellSize;

    float y = (pc.terrainType == TERRAIN_HEIGHTMAP) ? terrainHeight(xz) : 0.0;
    float3 root = float3(xz.x, y, xz.y);
    // Y is up
    float3 up = float3(0.0, 1.0, 0.0);

    bool bActive = true;
    if (pc.terrainType == TERRAIN_HEIGHTMAP) {
        // Check for slope rejection
        float slopeCos = terrainNormal(xz).y;
        float keep = smoothstep(GRASS_SLOPE_NONE_COS, GRASS_SLOPE_FULL_COS, slopeCos);
        bActive = random01(rng) < keep;
    }

    // Pick random properties
    float orientation = random01(rng) * 3.14159 * 2.0;
    float height = bActive ? lerp(MIN_HEIGHT, MAX_HEIGHT, random01(rng)) : 0.0;
    float width = lerp(MIN_WIDTH, MAX_WIDTH, random01(rng));
    float stiffness = lerp(MIN_STIFF, MAX_STIFF, random01(rng));

    Blade b;
    b.v0 = float4(root, orientation);
    b.v1 = float4(root + up * height, height);
    b.v2 = float4(root + up * height, width);
    b.up = float4(up, stiffness);
    pc.blades[i] = b;
}