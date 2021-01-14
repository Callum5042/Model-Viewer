#include "Pch.h"
#include "Model.h"
#include "Renderer.h"
#include "Shader.h"
#include "ModelLoader.h"

DxModel::DxModel(IRenderer* renderer, IShader* shader) : m_Shader(shader)
{
	m_Renderer = reinterpret_cast<DXRenderer*>(renderer);
}

DxModel::~DxModel()
{
}

bool DxModel::Load(const std::string& path)
{
	m_MeshData = std::make_unique<MeshData>();
	if (!ModelLoader::Load(path, m_MeshData.get()))
	{
		return false;
	}

	// Create vertex buffer
	m_VertexBuffer = m_Renderer->CreateVertexBuffer(m_MeshData->vertices);

	// Create index buffer
	m_IndexBuffer = m_Renderer->CreateIndexBuffer(m_MeshData->indices);

	// Load mr texture
	m_DiffuseTexture = m_Renderer->CreateTexture2D("Data Files/Textures/crate_diffuse.dds");
	m_NormalTexture = m_Renderer->CreateTexture2D("Data Files/Textures/crate_normal.dds");

	return true;
}

void DxModel::Update(float dt)
{
	static float TimeInSeconds = 0.0f;
	TimeInSeconds += dt * 100.0f;

	auto numBones = m_MeshData->bones.size();
	std::vector<DirectX::XMMATRIX> toParentTransforms(numBones);

	// Animation
	ShaderData::BoneBuffer bone_buffer = {};
	auto clip = m_MeshData->animations.find("Take1");
	if (clip != m_MeshData->animations.end())
	{
		clip->second.Interpolate(TimeInSeconds, toParentTransforms);
		if (TimeInSeconds > clip->second.GetClipEndTime())
		{
			TimeInSeconds = 0.0f;
		}

		// Transform to root
		std::vector<DirectX::XMMATRIX> toRootTransforms(numBones);
		toRootTransforms[0] = toParentTransforms[0];
		for (UINT i = 1; i < numBones; ++i)
		{
			DirectX::XMMATRIX toParent = toParentTransforms[i];
			DirectX::XMMATRIX parentToRoot = toRootTransforms[m_MeshData->bones[i].parentId];
			toRootTransforms[i] = XMMatrixMultiply(toParent, parentToRoot);
		}

		// Transform bone
		for (size_t i = 0; i < m_MeshData->bones.size(); i++)
		{
			DirectX::XMMATRIX offset = m_MeshData->bones[i].offset;
			DirectX::XMMATRIX toRoot = toRootTransforms[i];
			DirectX::XMMATRIX matrix = DirectX::XMMatrixMultiply(offset, toRoot);
			bone_buffer.transform[i] = DirectX::XMMatrixTranspose(matrix);
		}
	}
	else
	{
		bone_buffer.transform[0] = DirectX::XMMatrixIdentity();
	}

	// Update bone buffer
	m_Shader->UpdateBones(bone_buffer);
}

void DxModel::Render(Camera* camera)
{
	// Bind the vertex buffer
	m_Renderer->ApplyVertexBuffer(m_VertexBuffer.get());

	// Bind the index buffer
	m_Renderer->ApplyIndexBuffer(m_IndexBuffer.get());

	// Texture
	m_Renderer->ApplyTexture2D(0, m_DiffuseTexture.get());
	m_Renderer->ApplyTexture2D(1, m_NormalTexture.get());

	// Set topology
	m_Renderer->SetPrimitiveTopology();

	// Render geometry
	for (auto& subset : m_MeshData->subsets)
	{
		m_Renderer->DrawIndex(subset.totalIndex, subset.startIndex, subset.baseVertex);
	}
}

float BoneAnimation::GetStartTime() const
{
	return Keyframes.front().TimePos;
}

float BoneAnimation::GetEndTime() const
{
	return Keyframes.back().TimePos;
}

void BoneAnimation::Interpolate(float t, DirectX::XMMATRIX& M) const
{
	if (t <= Keyframes.front().TimePos)
	{
		DirectX::XMVECTOR S = XMLoadFloat3(&Keyframes.front().Scale);
		DirectX::XMVECTOR P = XMLoadFloat3(&Keyframes.front().Translation);
		DirectX::XMVECTOR Q = XMLoadFloat4(&Keyframes.front().RotationQuat);

		DirectX::XMVECTOR zero = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		M = DirectX::XMMatrixAffineTransformation(S, zero, Q, P);
	}
	else if (t >= Keyframes.back().TimePos)
	{
		DirectX::XMVECTOR S = XMLoadFloat3(&Keyframes.back().Scale);
		DirectX::XMVECTOR P = XMLoadFloat3(&Keyframes.back().Translation);
		DirectX::XMVECTOR Q = XMLoadFloat4(&Keyframes.back().RotationQuat);

		DirectX::XMVECTOR zero = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		M = DirectX::XMMatrixAffineTransformation(S, zero, Q, P);
	}
	else
	{
		for (UINT i = 0; i < Keyframes.size() - 1; ++i)
		{
			// Typedef to stop the compiler from moaning about arithmetic overflow 
			typedef std::vector<Keyframe, std::allocator<Keyframe>>::size_type KeyframeSize;
			if (t >= Keyframes[i].TimePos && t <= Keyframes[static_cast<KeyframeSize>(i) + 1].TimePos)
			{
				float lerpPercent = (t - Keyframes[i].TimePos) / (Keyframes[static_cast<KeyframeSize>(i) + 1].TimePos - Keyframes[i].TimePos);

				DirectX::XMVECTOR s0 = XMLoadFloat3(&Keyframes[i].Scale);
				DirectX::XMVECTOR s1 = XMLoadFloat3(&Keyframes[static_cast<std::vector<Keyframe, std::allocator<Keyframe>>::size_type>(i) + 1].Scale);

				DirectX::XMVECTOR p0 = XMLoadFloat3(&Keyframes[i].Translation);
				DirectX::XMVECTOR p1 = XMLoadFloat3(&Keyframes[static_cast<KeyframeSize>(i) + 1].Translation);

				DirectX::XMVECTOR q0 = XMLoadFloat4(&Keyframes[i].RotationQuat);
				DirectX::XMVECTOR q1 = XMLoadFloat4(&Keyframes[static_cast<KeyframeSize>(i) + 1].RotationQuat);

				DirectX::XMVECTOR S = DirectX::XMVectorLerp(s0, s1, lerpPercent);
				DirectX::XMVECTOR P = DirectX::XMVectorLerp(p0, p1, lerpPercent);
				DirectX::XMVECTOR Q = DirectX::XMQuaternionSlerp(q0, q1, lerpPercent);

				DirectX::XMVECTOR zero = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
				M = DirectX::XMMatrixAffineTransformation(S, zero, Q, P);
				break;
			}
		}
	}
}

float AnimationClip::GetClipStartTime() const
{
	// Find smallest start time over all bones in this clip.
	auto t = std::numeric_limits<float>::max();
	for (auto i = 0u; i < BoneAnimations.size(); ++i)
	{
		t = std::min(t, BoneAnimations[i].GetStartTime());
	}

	return t;
}

float AnimationClip::GetClipEndTime() const
{
	// Find largest end time over all bones in this clip.
	auto t = 0.0f;
	for (auto i = 0u; i < BoneAnimations.size(); ++i)
	{
		t = std::max(t, BoneAnimations[i].GetEndTime());
	}

	return t;
}

void AnimationClip::Interpolate(float t, std::vector<DirectX::XMMATRIX>& boneTransforms) const
{
	for (auto i = 0u; i < BoneAnimations.size(); ++i)
	{
		BoneAnimations[i].Interpolate(t, boneTransforms[i]);
	}
}