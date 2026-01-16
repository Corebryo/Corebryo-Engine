#version 450

layout(location = 0) in vec3 InPosition;

layout(push_constant) uniform ShadowPush
{
    mat4 LightViewProj;
    mat4 Model;
} pc;

void main()
{
    gl_Position = pc.LightViewProj * pc.Model * vec4(InPosition, 1.0);
}
