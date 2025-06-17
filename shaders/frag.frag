#version 450

layout(binding = 1) uniform sampler2D u_Sampler;

layout(location = 0) in vec3 in_Colour;
layout(location = 1) in vec2 in_TexCoord;

layout(location = 0) out vec4 out_Colour;

void main()
{
    out_Colour = texture(u_Sampler, in_TexCoord);
}