#version 400 core

uniform mat4 view, projection, transform;

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec4 vColour;
layout (location = 2) in vec4 vUV;

out vec4 fColour;

void main()
{
    gl_Position = projection * view * transform * vPosition;
    fColour = vColour;
}