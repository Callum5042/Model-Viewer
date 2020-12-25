#version 450 core

layout (location = 0) out vec4 color;
layout (binding = 0) uniform sampler2D tex;

in vec4 fColour;
in vec2 fUV;

void main()
{
	vec4 diffuse_colour = texture(tex, fUV);
	color = diffuse_colour * fColour;
}