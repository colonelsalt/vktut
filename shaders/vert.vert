#version 450

layout(set = 0, binding = 0) uniform uniform_buffer_object
{
    mat4 Model;
    mat4 View;
    mat4 Proj;
} u_Mvp;

layout(location = 0) in vec2 in_Position;
layout(location = 1) in vec3 in_Colour;
layout(location = 2) in vec2 in_TexCoord;

layout(location = 0) out vec3 out_FragColour;
layout(location = 1) out vec2 out_TexCoord;

void main()
{
    gl_Position = u_Mvp.Proj * u_Mvp.View * u_Mvp.Model * vec4(in_Position, 0.0, 1.0);
    out_FragColour = in_Colour;
    out_TexCoord = in_TexCoord;
}