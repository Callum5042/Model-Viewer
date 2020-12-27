#version 450 core

uniform vec4 gDirectionLight, gDiffuseLight, gAmbientLight;

layout (location = 0) out vec4 color;
layout (binding = 0) uniform sampler2D tex;

in vec4 fColour;
in vec2 fUV;
in vec3 fNormal;

vec4 CalculateDirectionalLighting()
{
	vec4 material_diffuse = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 material_ambient = vec4(1.0f, 1.0f, 1.0f, 1.0f);

	vec3 normal = normalize(fNormal);

	// Diffuse lighting
	vec3 lightVec = -gDirectionLight.xyz;
	vec4 diffuse_light = max(dot(normal, lightVec), 0.0) * gDiffuseLight * material_diffuse;
	//vec4 diffuse_light = clamp(dot(lightVec, normal), 0.0, 1.0) * gDiffuseLight * material_diffuse;

	// Ambient lightingt
	vec4 ambient_light = gAmbientLight * maerial_ambient;

//	// Specular lighting
//	float3 viewDir = normalize(mDirectionalLight.mCameraPos - position);
//	float3 reflectDir = reflect(-lightVec, no rmal);
//	float spec = pow(max(dot(viewDir, reflectDir), 0.0), mDirectionalLight.mSpecular.w * cMaterial.mSpecular.w);
//	float4 specular_light = lighting * float4(spec * mDirectionalLight.mSpecular.xyz * cMaterial.mSpecular.xyz, 1.0f);

	return diffuse_light;
}

void main()
{
	// Sample texture
	vec4 diffuse_colour = texture(tex, fUV);

	// Calculate light
	vec4 directional_light = CalculateDirectionalLighting();

	// Final pixel
	//color = diffuse_colour * fColour;
	color = directional_light;// *diffuse_colour;
}