import grass_shared;

[[vk::push_constant]] GrassSimPushConstants pc;

static const float waveScale = 0.25;
static const float waveSpeed = 1.5;
static const float windStrength = 6.0;
static const float gustStrength = 14.0;

static const float bezierDegree = 2.0;

float3 windInfluence(float3 v0, float time)
{
    float2 dir = normalize(float2(1.0, 0.3));
    float phase = dot(v0.xz, dir) * waveScale - time * waveSpeed;
    float strength = windStrength + gustStrength * (0.5 + 0.5 * sin(phase));
    return float3(dir.x, 0.0, dir.y) * strength;
}

float3 gravityForce(float3 f)
{
    float4 D = float4(0.0, -1.0, 0.0, 9.8);
    // Assume mass = 1.0 for now
    float3 gE = normalize(D.xyz) * D.w;
    float3 gF = 0.25 * length(gE) * f;
    return gE + gF;
}

float3 windForce(float3 wi, float3 v0, float3 v2, float3 up, float h)
{
    float windLen = length(wi);
    if (windLen < 1e-5) {
        return float3(0);
    }

    float3 tipDir = v2 - v0;
    float tipLen = length(tipDir);
    if (tipLen < 1e-5) {
        return float3(0);
    }

    float fd = 1.0 - abs(dot(wi / windLen, tipDir / tipLen));
    float fr = dot(v2 - v0, up) / h;

    float theta = fd * fr;
    return wi * theta;
}

float3 recoveryForce(float3 v2, float3 Iv2, float s)
{
    return (Iv2 - v2) * s;
}

void validateBlade(float3 v0, inout float3 v1, inout float3 v2, float3 up, float h)
{
    // eq 9
    v2 = v2 - up * min(dot(up, v2 - v0), 0.0);
    // eq 10
    float lproj = length(v2 - v0 - up * dot(v2 - v0, up));
    // eq 11
    v1 = v0 + h * up * max(1.0 - lproj / h, 0.05 * max(lproj / h, 1.0));
    // eq 12
    float L0 = distance(v0, v2);
    float L1 = distance(v0, v1) + distance(v1, v2);
    float L = (2.0 * L0 + (bezierDegree - 1.0) * L1) / (bezierDegree + 1.0);
    // eq 13
    float r = h / L;
    float3 v1corr = v0 + r * (v1 - v0);
    float3 v2corr = v1corr + r * (v2 - v1);
    v1 = v1corr;
    v2 = v2corr;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 threadId: SV_DispatchThreadID)
{
    uint i = threadId.x;
    if (i >= pc.bladeCount) {
        return;
    }

    Blade blade = pc.blades[i];

    float3 v0 = blade.v0.xyz;
    float3 v1 = blade.v1.xyz;
    float3 v2 = blade.v2.xyz;
    float3 up = blade.up.xyz;
    float orientation = blade.v0.w;
    float h = blade.v1.w;
    float s = blade.up.w;

    float3 t1 = float3(cos(orientation), 0.0, sin(orientation));
    float3 f = normalize(cross(up, t1));

    float3 Iv2 = v0 + up * h;

    float3 r = recoveryForce(v2, Iv2, s);
    float3 g = gravityForce(f);
    float3 wi = windInfluence(v0, pc.totalTime);
    float3 w = windForce(wi, v0, v2, up, h);

    float3 delta = (r + g + w) * pc.deltaTime;

    v2 += delta;

    validateBlade(v0, v1, v2, up, h);

    blade.v1.xyz = v1;
    blade.v2.xyz = v2;

    pc.blades[i] = blade;
}