#include "Header.hlsli"

float4 CalculateDirectionalLighting(float3 position, float3 normal, float lighting)
{
	// Diffuse lighting
	float3 lightVec = -mDirectionalLight.mDirection.xyz;
	float4 diffuse_light = lighting * saturate(dot(lightVec, normal)) * mDirectionalLight.mDiffuse * cMaterial.mDiffuse;

	// Ambient lighting
	float4 ambient_light = mDirectionalLight.mAmbient * cMaterial.mAmbient;

	// Specular lighting
	float3 viewDir = normalize(mDirectionalLight.mCameraPos - position);
	float3 reflectDir = reflect(-lightVec, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), mDirectionalLight.mSpecular.w * cMaterial.mSpecular.w);
	float4 specular_light = lighting * float4(spec * mDirectionalLight.mSpecular.xyz * cMaterial.mSpecular.xyz, 1.0f);

	// Combine all 3 lights
	return diffuse_light + ambient_light + specular_light;
}

float3 CalculateBumpMap(PixelInput input)
{
	float3 normalMapSample = gTextureNormal.Sample(gSamplerAnisotropic, input.Texture).rgb;

	// Uncompress each component from [0,1] to [-1,1].
	float3 normalT = 2.0f * normalMapSample - 1.0f;

	// Build orthonormal basis.
	float3 N = input.Normal;
	float3 T = normalize(input.Tangent - dot(input.Tangent, N) * N);
	float3 B = cross(N, T);

	float3x3 TBN = float3x3(T, B, N);

	// Transform from tangent space to world space.
	float3 bumpedNormalW = mul(normalT, TBN);
	return bumpedNormalW;
}

float4 main(PixelInput input) : SV_TARGET
{
	input.Normal = normalize(input.Normal);

	// Sample the texture
	float4 diffuse_texture = gTextureDiffuse.Sample(gSamplerAnisotropic, input.Texture);
	
	// Shaders
	float lighting = 1;

	// Normal Texture
	//input.Normal = CalculateBumpMap(input);

	// Directional Light
	float4 directional_light = CalculateDirectionalLighting(input.Position, input.Normal, lighting);

	// Combine all pixels
	float4 finalColour = (directional_light)*diffuse_texture;// *input.Colour;
	return finalColour;
}