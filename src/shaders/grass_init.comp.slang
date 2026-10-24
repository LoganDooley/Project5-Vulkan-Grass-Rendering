import grass_shared;

[[vk::push_constant]] GrassInitPushConstants pc;

static const float MIN_HEIGHT = 0.6;
static const float MAX_HEIGHT = 1.2;
static const float MIN_WIDTH = 0.03;
static const float MAX_WIDTH = 0.05;
static const float MIN_STIFF = 20.0;
static const float MAX_STIFF = 25.0;

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 threadId : SV_DispatchThreadID)
{
    uint i = threadId.x;
    if (i >= pc.bladeCount) {
        return;
    }

    // Create a pcg hash based on the index and seed
    uint rng = pcgHash(i ^ pc.seed);

    // Stratified random sampling for position
    float cellSize = pc.areaSize / float(pc.gridWidth);
    float2 cell = float2(float(i % pc.gridWidth), float(i / pc.gridWidth));
    float2 jitter = float2(random01(rng), random01(rng));
    float2 xz = (cell + jitter) * cellSize - 0.5 * pc.areaSize;

    // Root is at y = 0
    float3 root = float3(xz.x, 0.0, xz.y);
    // Y is up
    float3 up = float3(0.0, 1.0, 0.0);

    // Pick random properties
    float orientation = random01(rng) * 3.14159 * 2.0;
    float height = lerp(MIN_HEIGHT, MAX_HEIGHT, random01(rng));
    float width = lerp(MIN_WIDTH, MAX_WIDTH, random01(rng));
    float stiffness = lerp(MIN_STIFF, MAX_STIFF, random01(rng));

    Blade b;
    b.v0 = float4(root, orientation);
    b.v1 = float4(root + up * height, height);
    b.v2 = float4(root + up * height, width);
    b.up = float4(up, stiffness);
    pc.blades[i] = b;
}