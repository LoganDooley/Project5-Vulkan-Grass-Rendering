#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
    mat4 proj;
} camera;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in float inV;

layout(location = 0) out vec4 outColor;

void main() {
    // Simple blend from darker green to lighter green root to tip
    vec3 root = vec3(0.05, 0.25, 0.05);
    vec3 tip  = vec3(0.45, 0.75, 0.25);
    vec3 albedo = mix(root, tip, inV);

    // Use some hard-coded light direction for now
    vec3 lightDir = normalize(vec3(0.4, 1.0, 0.3));
    // Simple dot product diffuse
    float diffuse = abs(dot(normalize(inNormal), lightDir));
    // Hard-coded ambient + diffuse 
    outColor = vec4(albedo * (0.3 + 0.7 * diffuse), 1.0);
}
