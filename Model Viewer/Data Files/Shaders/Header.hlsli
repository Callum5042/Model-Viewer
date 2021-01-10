// Data
struct Material
{
	float4 mDiffuse;
	float4 mAmbient;
	float4 mSpecular;
};

struct DirectionalLight
{
	float4 mDiffuse;
	float4 mAmbient;
	float4 mSpecular;
	float4 mDirection;
	float3 mCameraPos;
	float padding;
};

// Buffers
cbuffer WorldBuffer : register(b0)
{
	matrix cWorld;
	matrix cView;
	matrix cProjection;
	matrix cInverseWorld;

	matrix cTextureTransform;
	Material cMaterial;
}

cbuffer DirectionalLightBuffer : register(b1)
{
	DirectionalLight mDirectionalLight;
	matrix mLightView;
	matrix mLightProj;
};

// Bone constant buffer
cbuffer BoneBuffer : register(b2)
{
	matrix cBoneTransform[96];
}

// Texture data
SamplerState gSamplerAnisotropic : register(s0);
Texture2D gTextureDiffuse : register(t0);

SamplerComparisonState gShadowSampler : register(s1);
Texture2D gTextureNormal : register(t1);

// Vertex shader input
struct VertexInput
{
	float3 Position : POSITION;
	float4 Colour : COLOUR;
	float2 Texture : TEXTURE;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 BitTangent : BITTANGENT;
	float4 weight : WEIGHT;
	int4 bone : BONE;
};

// Pixel shader input
struct PixelInput
{
	float3 Position : POSITION;
	float4 PositionH : SV_POSITION;
	float4 Colour : COLOUR;
	float2 Texture : TEXTURE;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 BitTangent : BITTANGENT;
};
