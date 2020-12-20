#version 400 core

layout (location = 0) in vec4 vPosition;
out vec4 fColour;

uniform mat4 view, projection, transform;

void main()
{
    gl_Position = projection * view * transform * vPosition;
}