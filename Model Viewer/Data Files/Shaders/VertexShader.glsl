#version 400 core

layout (std140) uniform cWorld
{
    mat4 gWorld;
    mat4 gView;
    mat4 gProjection;
};

uniform mat4 gBoneTransform[96];

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec4 vColour;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vNormal;
layout (location = 4) in vec3 vTangent;
layout (location = 5) in vec3 vBiTangent;
layout (location = 6) in vec4 vWeight;
layout (location = 7) in ivec4 vBone;

out vec3 fPosition;
out vec4 fColour;
out vec2 fUV;
out vec3 fNormal;
out vec3 fTangent;
out vec3 fBiTangent;

void main()
{
    // Calculate bone weight
    float weights[4];
    weights[0] = vWeight.x;
    weights[1] = vWeight.y;
    weights[2] = vWeight.z;
    weights[3] = 1.0f - weights[0] - weights[1] - weights[2];

    // Bone
    vec3 position = vec3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; i++)
    {
        float weight = weights[i];
        int bone_index = vBone[i];
        mat4 transform = gBoneTransform[bone_index];

        position += weight * (vec4(vPosition, 1.0f) * transform).xyz;
    }

    // Position
    gl_Position = gProjection * gView * gWorld * vec4(position, 1.0f);
    fPosition = position;

    // Colour
    fColour = vColour;

    // Texture
    fUV = vUV;

    // Lights
    fNormal = (vNormal * inverse(mat3(gWorld))).xyz;
    fNormal = normalize(fNormal);

    // Normals
    fTangent = (vec4(vTangent, 1.0f) * inverse(gWorld)).xyz;
    fTangent = normalize(fTangent);

    fBiTangent = (vec4(vBiTangent, 1.0f) * inverse(gWorld)).xyz;
    fBiTangent = normalize(fBiTangent);
}