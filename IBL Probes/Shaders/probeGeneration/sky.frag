#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform samplerCube environmentMap;

layout(location = 0) in vec3 localPos;

layout(location = 0) out vec4 outColor;


void main()
{    
    outColor = vec4(texture(environmentMap, localPos).rgb, 1.0);
}