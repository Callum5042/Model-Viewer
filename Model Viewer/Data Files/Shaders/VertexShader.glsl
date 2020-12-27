#version 400 core

uniform mat4 gView, gProjection, gWorld;

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec4 vColour;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vNormal;
layout (location = 4) in vec3 vTangent;
layout (location = 5) in vec3 vBiTangent;

out vec4 fPosition;
out vec4 fColour;
out vec2 fUV;
out vec3 fNormal;
out vec3 fTangent;
out vec3 fBiTangent;

void main()
{
    gl_Position = gProjection * gView * gWorld * vPosition;

    fPosition = vPosition;

    fColour = vColour;
    fUV = vUV;

    // Texture mapping
    fNormal = (vNormal * inverse(mat3(gWorld))).xyz;
    fNormal = normalize(fNormal);

    // Normal mapping
    fTangent = (vec4(vTangent, 1.0f) * inverse(gWorld)).xyz;
    fTangent = normalize(fTangent);

    fBiTangent = (vec4(vBiTangent, 1.0f) * inverse(gWorld)).xyz;
    fBiTangent = normalize(fBiTangent);
}