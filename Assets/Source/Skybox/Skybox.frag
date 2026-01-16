#version 450

layout(set = 0, binding = 0) uniform samplerCube skyboxSampler;

layout(location = 0) in vec3 vDir;
layout(location = 0) out vec4 outColor;

/* Reinhard tonemapping. */
vec3 TonemapReinhard(vec3 color)
{
    return color / (color + vec3(1.0));
}

void main()
{
    /* Sample the HDR cubemap in linear space. */
    vec3 color = texture(skyboxSampler, normalize(vDir)).rgb;
    /* Tonemap before gamma correction. */
    color = TonemapReinhard(color);
    /* Apply gamma correction for display output. */
    color = pow(color, vec3(1.0 / 2.2));
    outColor = vec4(color, 1.0);
}
