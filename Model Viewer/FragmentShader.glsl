#version 450 core

in vec4 fColour;
layout (location = 0) out vec4 color;

void main()
{
	color = fColour;
}