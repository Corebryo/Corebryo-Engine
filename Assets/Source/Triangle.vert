#version 450

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec2 InUV;
layout(location = 2) in vec4 InInstance0;
layout(location = 3) in vec4 InInstance1;
layout(location = 4) in vec4 InInstance2;
layout(location = 5) in vec4 InInstance3;

layout(location = 0) out vec2 vUV;
layout(location = 1) flat out int vMode;

layout(set = 0, binding = 0) uniform LightData
{
    mat4 LightViewProj;
    mat4 ViewProj;
} ubo;

layout(push_constant) uniform PushConstants
{
    vec4 BaseColorAmbient;
    vec4 AlphaModePadding;
} pc;

void main()
{
    int mode = int(pc.AlphaModePadding.y);
    if (mode == 0)
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
        mat4 model = mat4(InInstance0, InInstance1, InInstance2, InInstance3);
        gl_Position = ubo.ViewProj * model * vec4(InPosition, 1.0);
        vUV = InUV;
        vMode = 1;
    }
}
