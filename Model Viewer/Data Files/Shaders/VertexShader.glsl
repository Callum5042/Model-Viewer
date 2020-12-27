#version 400 core

uniform mat4 gView, gProjection, gWorld;

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec4 vColour;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vNormal;

out vec4 fPosition;
out vec4 fColour;
out vec2 fUV;
out vec3 fNormal;

void main()
{
    gl_Position = gProjection * gView * gWorld * vPosition;

    fPosition = gl_Position;
    fColour = vColour;
    fUV = vUV;
    fNormal = normalize(vNormal);
}