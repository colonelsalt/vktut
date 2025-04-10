#version 450

layout(location = 0) out vec3 out_FragColour;

vec2 s_Positions[3] = vec2[]
(
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 s_Colours[3] = vec3[]
(
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main()
{
    out_FragColour = s_Colours[gl_VertexIndex];
    gl_Position = vec4(s_Positions[gl_VertexIndex], 0.0, 1.0);
}