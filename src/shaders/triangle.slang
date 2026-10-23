struct Payload {
    uint dummy;
};

groupshared Payload payload;

[shader("amplification")] // task shader
[numthreads(1, 1, 1)]
void taskMain()
{
    payload.dummy = 0;
    DispatchMesh(1, 1, 1, payload);
}

struct VertexOut {
    float4 position : SV_Position;
    float3 color;
};

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
        float2(0, -0.5),
        float2(0.5, 0.5),
        float2(-0.5, 0.5)
    };

    let refColor : float3[3] = {
        float3(1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 1.0)
    };

    verts[threadId] = {
        float4(refPos[threadId], 0.0, 1.0),
        refColor[threadId]
    };

    if (threadId == 0) {
        tris[0] = uint3(0, 1, 2);
    }
}

[shader("fragment")]
float4 fragMain(VertexOut input) : SV_Target {
    return float4(input.color, 1.0);
}