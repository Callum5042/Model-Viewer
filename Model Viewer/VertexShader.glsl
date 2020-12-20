#version 400 core

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec4 vColour;

out vec4 fColour;

uniform mat4 view, projection, transform;

void main()
{
    fColour = vColour;

    gl_Position = projection * view * transform * vPosition;
}