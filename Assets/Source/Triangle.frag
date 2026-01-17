#version 450

layout(location = 0) in vec2 vUV;
layout(location = 1) flat in int vMode;

layout(location = 0) out vec4 OutColor;

layout(set = 0, binding = 0) uniform LightData
{
    mat4 LightViewProj;
    mat4 ViewProj;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D DiffuseTexture;

layout(push_constant) uniform PushConstants
{
    vec4 BaseColorAmbient;
    vec4 AlphaModePadding;
} pc;

void main()
{
    if (vMode == 0)
    {
        vec3 topColor = vec3(0.20, 0.45, 0.80);
        vec3 bottomColor = vec3(0.80, 0.90, 1.00);
        float t = clamp(vUV.y, 0.0, 1.0);
        vec3 color = mix(bottomColor, topColor, t);
        OutColor = vec4(color, 1.0);
    }
    else
    {
        vec4 texColor = texture(DiffuseTexture, vUV);
        float ambient = 0.55;
        vec3 litColor = texColor.rgb * (1.0 + ambient);
        vec3 srgbColor = pow(clamp(litColor, 0.0, 1.0), vec3(1.0 / 2.2));       
        srgbColor = mix(srgbColor, vec3(1.0), 0.15);
        OutColor = vec4(srgbColor, pc.AlphaModePadding.x);
    }
}
