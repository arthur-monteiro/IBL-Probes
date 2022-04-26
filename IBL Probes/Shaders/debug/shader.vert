#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObjectMVP
{
    mat4 projection;
    mat4 model;
	mat4 view;
} uboMVP;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in uint inMaterialID;

layout(location = 5) in vec3 inCenterPos;
layout(location = 6) in uint inProbeId;

layout(binding = 0, set = 0) uniform CameraProperties
{
    mat4 model;
    mat4 view;
	mat4 projection;
} cam;

layout(location = 0) out vec3 outCenterPos;
layout(location = 1) out uint outProbeId;
 
out gl_PerVertex
{
    vec4 gl_Position;
};

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

void main() 
{
	vec4 worldPos = cam.model * vec4(inPosition, 1.0) + vec4(inCenterPos, 0.0);
    gl_Position = cam.projection * cam.view * worldPos;

	outCenterPos = inPosition;
	outProbeId = inProbeId;
} 