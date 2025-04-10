#version 450

layout(location = 0) in vec3 in_Colour;

layout(location = 0) out vec4 out_Colour;

void main()
{
    out_Colour = vec4(in_Colour, 1.0);
}