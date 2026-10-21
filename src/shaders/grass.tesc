#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(vertices = 1) out;

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
    mat4 proj;
} camera;

// TODO: Declare tessellation control shader inputs and outputs
layout(location = 0) in vec4 inV0[];
layout(location = 1) in vec4 inV1[];
layout(location = 2) in vec4 inV2[];
layout(location = 3) in vec4 inUp[];

layout(location = 0) out vec4 outV0[];
layout(location = 1) out vec4 outV1[];
layout(location = 2) out vec4 outV2[];
layout(location = 3) out vec4 outUp[];

void main() {
	// Don't move the origin location of the patch
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

	// Forward the V0, V1, V2, and Up vectors
    outV0[gl_InvocationID] = inV0[gl_InvocationID];
    outV1[gl_InvocationID] = inV1[gl_InvocationID];
    outV2[gl_InvocationID] = inV2[gl_InvocationID];
    outUp[gl_InvocationID] = inUp[gl_InvocationID];

    // Set level of tesselation
    gl_TessLevelInner[0] = 1.0; // Split horizontally just 1 segment
    gl_TessLevelInner[1] = 8.0; // Split vertically in 8 segments
    gl_TessLevelOuter[0] = 8.0; // Split left side in 8 segmnets
    gl_TessLevelOuter[1] = 1.0; // Split top side in just 1 segment
    gl_TessLevelOuter[2] = 8.0; // Split right side in 8 segments
    gl_TessLevelOuter[3] = 1.0; // Split bottom side in just 1 segment
}
