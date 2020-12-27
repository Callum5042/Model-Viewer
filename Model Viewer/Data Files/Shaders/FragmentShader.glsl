#version 450 core

uniform vec4 gDirectionLight, gDiffuseLight, gAmbientLight, gSpecularLight, gCameraPos;

layout (location = 0) out vec4 color;
layout (binding = 0) uniform sampler2D tex;

in vec4 fPosition;
in vec4 fColour;
in vec2 fUV;
in vec3 fNormal;

vec4 CalculateDirectionalLighting()
{
	vec4 diffuse = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 ambient = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 specular = vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// Diffuse lighting
	vec3 lightVec = -gDirectionLight.xyz;
	vec4 diffuse_light = clamp(dot(lightVec, fNormal), 0.0, 1.0) * gDiffuseLight * diffuse;

	// Ambient lightingt
	vec4 ambient_light = gAmbientLight * ambient;

	// Specular lighting
	vec3 viewDir = normalize(gCameraPos.xyz - fPosition.xyz);
	vec3 reflectDir = reflect(-lightVec, fNormal);

	float spec = pow(max(dot(viewDir, reflectDir), 0.0), gSpecularLight.w * specular.w);
	vec4 specular_light = vec4(spec * gSpecularLight.xyz * specular.xyz, 1.0f);

	return diffuse_light + ambient_light + specular_light;
}

void main()
{ 
	// Sample texture
	vec4 diffuse_colour = texture(tex, fUV);

	// Calculate light
	vec4 directional_light = CalculateDirectionalLighting();

	// Final pixel
	color = (directional_light) * diffuse_colour;// *fColour;
}