#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBuffer
{
    mat4 proj;
    mat4 model;
    mat4 view;
} ub;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 localPos;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    vec4 pos = ub.proj * mat4(mat3(ub.view)) * vec4(inPosition, 1.0);
    gl_Position = vec4(pos.xy, pos.w, pos.w);

    localPos = inPosition;
}  