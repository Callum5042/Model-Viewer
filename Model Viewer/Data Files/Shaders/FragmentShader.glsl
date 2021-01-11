#version 450 core

uniform vec4 gDirectionLight, gDiffuseLight, gAmbientLight, gSpecularLight, gCameraPos;

layout (location = 0) out vec4 color;
layout (binding = 0) uniform sampler2D diffuse_texture;
layout (binding = 1) uniform sampler2D normal_texture;

in vec3 fPosition;
in vec4 fColour;
in vec2 fUV;
in vec3 fNormal;
in vec3 fTangent;
in vec3 fBiTangent;

vec4 CalculateDirectionalLighting(vec3 bumped_normal)
{
	vec4 diffuse = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 ambient = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 specular = vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// Diffuse lighting
	vec3 lightVec = -gDirectionLight.xyz;
	vec4 diffuse_light = clamp(dot(lightVec, bumped_normal), 0.0, 1.0) * gDiffuseLight * diffuse;

	// Ambient lightingt
	vec4 ambient_light = gAmbientLight * ambient;

	// Specular lighting
	vec3 viewDir = normalize(gCameraPos.xyz - fPosition);
	vec3 reflectDir = reflect(-lightVec, bumped_normal);

	float spec = pow(max(dot(viewDir, reflectDir), 0.0), gSpecularLight.w * specular.w);
	vec4 specular_light = vec4(spec * gSpecularLight.xyz * specular.xyz, 1.0f);

	return diffuse_light + ambient_light + specular_light;
}

vec3 CalculateBumpMap()
{
	vec3 normalMapSample = texture(normal_texture, fUV).rgb;

	// Uncompress each component from [0,1] to [-1,1].
	vec3 normalT = 2.0f * normalMapSample - 1.0f;

	// Build orthonormal basis.
	vec3 N = fNormal;
	vec3 T = normalize(fTangent - dot(fTangent, N) * N);
	vec3 B = cross(N, T);

	mat3 TBN = mat3(T, B, N);

	// Transform from tangent space to world space.
	vec3 bumpedNormalW = TBN * normalT;
	return bumpedNormalW;
}

void main()
{ 
	// Diffuse texture
	vec4 diffuse_colour = texture(diffuse_texture, fUV);

	// Normal Texture
	vec3 bumped_normal = CalculateBumpMap();

	// Calculate light
	vec4 directional_light = CalculateDirectionalLighting(bumped_normal);

	// Final pixel
	color = (directional_light) * diffuse_colour;// *fColour;
}