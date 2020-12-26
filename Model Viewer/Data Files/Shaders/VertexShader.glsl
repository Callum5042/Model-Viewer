#version 400 core

uniform mat4 view, projection, transform;

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec4 vColour;
layout (location = 2) in vec2 vUV;

out vec4 fColour;
out vec2 fUV;

void main()
{
    gl_Position = projection * view * transform * vPosition;
    fColour = vColour;
    fUV = vUV;
}