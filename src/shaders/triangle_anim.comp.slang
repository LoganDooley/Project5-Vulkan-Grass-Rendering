import common;

[[vk::push_constant]] AnimPushConstants pc;

[shader("compute")]
[numthreads(32, 1, 1)]
void computeMain(uint3 tid: SV_DispatchThreadID)
{
    if (tid.x >= 3) {
        // Only 3 vertices to update
        return;
    }

    // Reference vertices
    let refVerts : float2[3] = { 
        float2(0, 0.5), 
        float2(0.5, -0.5), 
        float2(-0.5, -0.5) 
    };

    float s = sin(pc.time);
    float c = cos(pc.time);
    float2 v = refVerts[tid.x];
    pc.positions[tid.x] = float4(c * v.x - s * v.y, s * v.x + c * v.y, 0.0, 1.0); // 2D rotation
}