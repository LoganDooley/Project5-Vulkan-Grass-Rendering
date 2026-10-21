#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(quads, equal_spacing, ccw) in;

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
    mat4 proj;
} camera;

layout(location = 0) in vec4 inV0[];
layout(location = 1) in vec4 inV1[];
layout(location = 2) in vec4 inV2[];
layout(location = 3) in vec4 inUp[];

layout(location = 0) out vec3 outNormal;
layout(location = 1) out float outV;

void main() {
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec3 v0 = inV0[0].xyz;
    vec3 v1 = inV1[0].xyz;
    vec3 v2 = inV2[0].xyz;
    float orientation = inV0[0].w;
    float width = inV2[0].w;

    // Equations for a, b, c, t1, c0, c1, t0, and n from the paper
    vec3 a = mix(v0, v1, v);
    vec3 b = mix(v1, v2, v);
    vec3 c = mix(a, b, v);

    // Calculate t1 as the bitangent assuming Y up
    vec3 t1 = vec3(cos(orientation), 0.0, sin(orientation));
    vec3 c0 = c - width * t1;
    vec3 c1 = c + width * t1;

    vec3 up = inUp[0].xyz;
    vec3 tangent = b - a;
    // Avoid getting a nan when normalizing
    vec3 t0 = (dot(tangent, tangent) > 1e-8) ? normalize(tangent) : up;
    outNormal = normalize(cross(t0, t1));

    // Triangle shape from the paper
    float t = u + 0.5 * v - u * v;
    vec3 pos = mix(c0, c1, t);

    outV = v;
    gl_Position = camera.proj * camera.view * vec4(pos, 1.0);
}
