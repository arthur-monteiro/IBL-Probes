#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 inCenterPos;
layout(location = 1) flat in uint inProbeId;

layout (binding = 1) uniform sampler inCubemapSampler;
layout (binding = 2) uniform textureCube[] inCubemaps;

layout(location = 0) out vec4 outColor;

void main() 
{
	outColor = vec4(texture(samplerCube(inCubemaps[inProbeId], inCubemapSampler), inCenterPos).rgb, 1.0);
}
