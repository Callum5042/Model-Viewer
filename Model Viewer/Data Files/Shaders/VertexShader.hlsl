#include "Header.hlsli"

PixelInput main(VertexInput input)
{
	PixelInput output;

	// Calculate bone weight
	float weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	weights[0] = input.weight.x;
	weights[1] = input.weight.y;
	weights[2] = input.weight.z;
	weights[3] = 1.0f - weights[0] - weights[1] - weights[2];

	// Transform by bone influence
	float3 position = float3(0, 0, 0);
	for (int i = 0; i < 4; i++)
	{
		float weight = weights[i];
		int bone_index = input.bone[i];
		matrix transform = cBoneTransform[bone_index];

		position += weight * mul(float4(input.Position, 1.0f), transform).xyz;
	}

	// Transform to homogeneous clip space.
	output.PositionH = mul(float4(position, 1.0f), cWorld);
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
	output.Normal = normalize(output.Normal);

	// Normal mapping
	output.Tangent = mul(input.Tangent, (float3x3)cInverseWorld);
	output.BitTangent = mul(input.BitTangent, (float3x3)cInverseWorld);

	output.Tangent = normalize(output.Tangent);
	output.BitTangent = normalize(output.BitTangent);

	return output;
}