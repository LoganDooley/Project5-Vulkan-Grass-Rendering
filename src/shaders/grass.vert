
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform ModelBufferObject {
    mat4 model;
};

// TODO: Declare vertex shader inputs and outputs
layout(location = 0) in vec4 inV0;
layout(location = 1) in vec4 inV1;
layout(location = 2) in vec4 inV2;
layout(location = 3) in vec4 inUp;

layout(location = 0) out vec4 outV0;
layout(location = 1) out vec4 outV1;
layout(location = 2) out vec4 outV2;
layout(location = 3) out vec4 outUp;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	// Transform V0, V1, V2 and Up by the model matrix
    outV0 = vec4((model * vec4(inV0.xyz, 1.0)).xyz, inV0.w);
    outV1 = vec4((model * vec4(inV1.xyz, 1.0)).xyz, inV1.w);
    outV2 = vec4((model * vec4(inV2.xyz, 1.0)).xyz, inV2.w);
    outUp = vec4(normalize(mat3(model) * inUp.xyz), inUp.w);

    gl_Position = vec4(outV0.xyz, 1.0);
}
