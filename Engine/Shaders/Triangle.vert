#version 450

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec2 InUV;

layout(location = 0) out vec2 vUV;
layout(location = 1) flat out int vMode;

layout(set = 0, binding = 0) uniform LightData
{
    mat4 LightViewProj;
} ubo;

layout(push_constant) uniform PushConstants
{
    mat4 MVP;
    mat4 Model;
    vec3 BaseColor;
    float Ambient;
    float Alpha;
    int Mode;
} pc;

void main()
{
    if (pc.Mode == 0)
    {
        vec2 positions[3] = vec2[](
            vec2(-1.0, -1.0),
            vec2(3.0, -1.0),
            vec2(-1.0, 3.0)
        );
        vec2 uvs[3] = vec2[](
            vec2(0.0, 0.0),
            vec2(2.0, 0.0),
            vec2(0.0, 2.0)
        );

        gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
        vUV = uvs[gl_VertexIndex];
        vMode = 0;
    }
    else
    {
        gl_Position = pc.MVP * vec4(InPosition, 1.0);
        vUV = InUV;
        vMode = 1;
    }
}
