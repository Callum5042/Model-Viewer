#include "Header.hlsli"

float4 main(PixelInput input) : SV_TARGET
{
	float4 diffuse_texture = gTextureDiffuse.Sample(gSamplerAnisotropic, input.Texture);
	return diffuse_texture * input.Colour;
}