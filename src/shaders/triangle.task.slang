import triangle_shared;

groupshared Payload payload;

[shader("amplification")]
[numthreads(1, 1, 1)]
void taskMain(uint groupId: SV_GroupID)
{
    payload.index = groupId;
    DispatchMesh(1, 1, 1, payload);
}