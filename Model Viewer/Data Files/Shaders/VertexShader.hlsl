#include "Header.hlsli"

PixelInput main(VertexInput input)
{
	PixelInput output;

	// Transform to homogeneous clip space.
	output.PositionH = mul(float4(input.Position, 1.0f), cWorld);
	output.PositionH = mul(output.PositionH, cView);
	output.PositionH = mul(output.PositionH, cProjection);

	// Transform to world space.
	output.Position = mul(float4(input.Position, 1.0f), cWorld).xyz;

	// Pass colour to pixel shader
	output.Colour = input.Colour;

	// Transform texture to world space.
	output.Texture = mul(float4(input.Texture, 1.0f, 1.0f), cTextureTransform).xy;

	// Transform normals by inverse world
	output.Normal = mul(input.Normal, (float3x3)cInverseWorld).xyz;

	return output;
}