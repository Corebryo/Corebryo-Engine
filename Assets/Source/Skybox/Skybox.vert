#version 450

layout(location = 0) in vec3 inPos;

layout(push_constant) uniform PushConstants
{
    mat4 viewProj;
    mat4 viewInv;
} pc;

layout(location = 0) out vec3 vDir;

void main()
{
    vDir = (pc.viewInv * vec4(inPos, 0.0)).xyz;
    vec4 clip = pc.viewProj * vec4(inPos, 1.0);
    gl_Position = vec4(clip.xy, clip.w, clip.w);
}
