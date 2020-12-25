struct Material
{
	float4 mDiffuse;
	float4 mAmbient;
	float4 mSpecular;
};

cbuffer WorldBuffer : register(b0)
{
	matrix cWorld;
	matrix cView;
	matrix cProjection;

	matrix cTextureTransform;
	Material cMaterial;
}

SamplerState gSamplerAnisotropic : register(s0);
Texture2D gTextureDiffuse : register(t0);

struct VertexInput
{
	float3 Position : POSITION;
	float4 Colour : COLOUR;
	float2 Texture : TEXTURE;
};

struct PixelInput
{
	float3 Position : POSITION;
	float4 PositionH : SV_POSITION;
	float4 Colour : COLOUR;
	float2 Texture : TEXTURE;
};
