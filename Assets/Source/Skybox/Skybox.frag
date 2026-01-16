#version 450

layout(set = 0, binding = 0) uniform samplerCube skyboxSampler;

layout(location = 0) in vec3 vDir;
layout(location = 0) out vec4 outColor;

void main()
{
    vec3 color = texture(skyboxSampler, normalize(vDir)).rgb;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));
    outColor = vec4(color, 1.0);
}
