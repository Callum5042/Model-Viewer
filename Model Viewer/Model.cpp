#include "Pch.h"
#include "Model.h"
#include "Renderer.h"
#include "Camera.h"
#include "Shader.h"
#include <DirectXMath.h>
#include "DDSTextureLoader.h"
#include "LoadTextureDDS.h"
#include "ModelLoader.h"

__declspec(align(16)) struct ShaderMaterial
{
	DirectX::XMFLOAT4 mDiffuse;
	DirectX::XMFLOAT4 mAmbient;
	DirectX::XMFLOAT4 mSpecular;
};

_declspec(align(16)) struct ConstantBuffer
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
	DirectX::XMMATRIX worldInverse;
	DirectX::XMMATRIX texture;
	ShaderMaterial mMaterial;
};

_declspec(align(16)) struct DirectionalLight
{
	DirectX::XMFLOAT4 mDiffuse;
	DirectX::XMFLOAT4 mAmbient;
	DirectX::XMFLOAT4 mSpecular;
	DirectX::XMFLOAT4 mDirection;
	DirectX::XMFLOAT3 mCameraPos;
	float padding;
};

_declspec(align(16)) struct LightBuffer
{
	DirectionalLight mDirectionalLight;
	/*DirectX::XMMATRIX mLightView;
	DirectX::XMMATRIX mLightProj;*/
};

_declspec(align(16)) struct BoneBuffer
{
	DirectX::XMMATRIX transform[96];
};

DxModel::DxModel(IRenderer* renderer)
{
	m_Renderer = reinterpret_cast<DxRenderer*>(renderer);
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
	D3D11_BUFFER_DESC ibd = {};
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.ByteWidth = static_cast<UINT>(sizeof(UINT) * m_MeshData->indices.size());
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA iInitData = {};
	iInitData.pSysMem = m_MeshData->indices.data();

	DX::Check(m_Renderer->GetDevice()->CreateBuffer(&ibd, &iInitData, m_IndexBuffer.ReleaseAndGetAddressOf()));

	// Constant buffer
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	DX::Check(m_Renderer->GetDevice()->CreateBuffer(&bd, nullptr, m_ConstantBuffer.ReleaseAndGetAddressOf()));

	// Load mr texture
	ComPtr<ID3D11Resource> resource = nullptr;
	DX::Check(DirectX::CreateDDSTextureFromFile(m_Renderer->GetDevice().Get(), L"Data Files\\Textures\\crate_diffuse.dds", resource.ReleaseAndGetAddressOf(), m_DiffuseTexture.ReleaseAndGetAddressOf()));

	DX::Check(DirectX::CreateDDSTextureFromFile(m_Renderer->GetDevice().Get(), L"Data Files\\Textures\\crate_normal.dds", resource.ReleaseAndGetAddressOf(), m_NormalTexture.ReleaseAndGetAddressOf()));

	// Mr sun
	D3D11_BUFFER_DESC lbd = {};
	lbd.Usage = D3D11_USAGE_DEFAULT;
	lbd.ByteWidth = sizeof(LightBuffer);
	lbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	DX::Check(m_Renderer->GetDevice()->CreateBuffer(&lbd, nullptr, m_LightBuffer.GetAddressOf()));

	// Mr bone buffer
	D3D11_BUFFER_DESC bone_bd = {};
	bone_bd.Usage = D3D11_USAGE_DEFAULT;
	bone_bd.ByteWidth = sizeof(BoneBuffer);
	bone_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	DX::Check(m_Renderer->GetDevice()->CreateBuffer(&bone_bd, nullptr, m_BoneConstantBuffer.ReleaseAndGetAddressOf()));

	return true;
}

void DxModel::Update(float dt)
{
	static float TimeInSeconds = 0.0f;
	TimeInSeconds += dt * 100.0f;

	auto numBones = m_MeshData->bones.size();
	std::vector<DirectX::XMMATRIX> toParentTransforms(numBones);

	// Animation
	BoneBuffer bone_buffer = {};
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
	m_Renderer->GetDeviceContext()->VSSetConstantBuffers(2, 1, m_BoneConstantBuffer.GetAddressOf());
	m_Renderer->GetDeviceContext()->UpdateSubresource(m_BoneConstantBuffer.Get(), 0, nullptr, &bone_buffer, 0, 0);
}

void DxModel::Render(Camera* camera)
{
	// Bind the vertex buffer
	m_Renderer->ApplyVertexBuffer(m_VertexBuffer.get());

	// Bind the index buffer
	m_Renderer->GetDeviceContext()->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// Set topology
	m_Renderer->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set buffer
	auto world = DirectX::XMMatrixIdentity();

	ConstantBuffer cb = {};
	cb.world = DirectX::XMMatrixTranspose(world);
	cb.view = DirectX::XMMatrixTranspose(camera->GetView());
	cb.projection = DirectX::XMMatrixTranspose(camera->GetProjection());
	cb.worldInverse = DirectX::XMMatrixInverse(nullptr, world);
	cb.texture = DirectX::XMMatrixIdentity();

	ShaderMaterial material = {};
	material.mDiffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.mAmbient = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.mSpecular = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	cb.mMaterial = material;

	m_Renderer->GetDeviceContext()->VSSetConstantBuffers(0, 1, m_ConstantBuffer.GetAddressOf());
	m_Renderer->GetDeviceContext()->PSSetConstantBuffers(0, 1, m_ConstantBuffer.GetAddressOf());
	m_Renderer->GetDeviceContext()->UpdateSubresource(m_ConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

	// Texture
	m_Renderer->GetDeviceContext()->PSSetShaderResources(0, 1, m_DiffuseTexture.GetAddressOf());
	m_Renderer->GetDeviceContext()->PSSetShaderResources(1, 1, m_NormalTexture.GetAddressOf());

	// Light up me life
	auto diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	auto ambient = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 0.0f);
	auto specular = DirectX::XMFLOAT4(0.1f, 0.1f, 0.1f, 32.0f);
	auto direction = DirectX::XMFLOAT4(-0.8f, -0.5f, 0.5f, 1.0f);

	LightBuffer lightBuffer = {};
	lightBuffer.mDirectionalLight.mCameraPos = camera->GetPosition();
	lightBuffer.mDirectionalLight.mDiffuse = diffuse;
	lightBuffer.mDirectionalLight.mAmbient = ambient;
	lightBuffer.mDirectionalLight.mSpecular = specular;
	lightBuffer.mDirectionalLight.mDirection = direction;

	//lightBuffer.mLightView = DirectX::XMMatrixTranspose(ortho->GetView());
	//lightBuffer.mLightProj = DirectX::XMMatrixTranspose(ortho->GetProjection());

	m_Renderer->GetDeviceContext()->VSSetConstantBuffers(1, 1, m_LightBuffer.GetAddressOf());
	m_Renderer->GetDeviceContext()->PSSetConstantBuffers(1, 1, m_LightBuffer.GetAddressOf());
	m_Renderer->GetDeviceContext()->UpdateSubresource(m_LightBuffer.Get(), 0, nullptr, &lightBuffer, 0, 0);

	// Render geometry
	for (auto& subset : m_MeshData->subsets)
	{
		m_Renderer->GetDeviceContext()->DrawIndexed(subset.totalIndex, subset.startIndex, subset.baseVertex);
	}
}

GlModel::GlModel(IRenderer* renderer, IShader* shader)
{
	m_Renderer = renderer;
	m_Shader = reinterpret_cast<GlShader*>(shader);
}

GlModel::~GlModel()
{
	glDeleteTextures(1, &m_NormalTextureId);
	glDeleteTextures(1, &m_DiffuseTextureId);
	glDeleteBuffers(1, &m_IndexBuffer);
}

bool GlModel::Load(const std::string& path)
{
	m_MeshData = std::make_unique<MeshData>();
	if (!ModelLoader::Load(path, m_MeshData.get()))
	{
		return false;
	}

	// Vertex Array Object
	m_VertexBuffer = m_Renderer->CreateVertexBuffer(m_MeshData->vertices);

	// Index Buffer
	glCreateBuffers(1, &m_IndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * m_MeshData->indices.size(), &m_MeshData->indices[0], GL_STATIC_DRAW);

	// Load diffuse texture
	Rove::LoadDDS diffuse_dds;
	std::string diffuse_texture_path = "Data Files\\Textures\\crate_diffuse.dds";
	diffuse_dds.Load(std::move(diffuse_texture_path));

	glCreateTextures(GL_TEXTURE_2D, 1, &m_DiffuseTextureId);
	glTextureStorage2D(m_DiffuseTextureId, diffuse_dds.MipmapCount(), diffuse_dds.Format(), diffuse_dds.Width(), diffuse_dds.Height());

	for (auto& mipmap : diffuse_dds.mipmaps)
	{
		glCompressedTextureSubImage2D(m_DiffuseTextureId, mipmap.level, 0, 0, mipmap.width, mipmap.height, diffuse_dds.Format(), mipmap.texture_size, mipmap.data);
	}

	glBindTextureUnit(0, m_DiffuseTextureId);

	// Load normal texture
	Rove::LoadDDS normal_dds;
	std::string normal_texture_path = "Data Files\\Textures\\crate_normal.dds";
	normal_dds.Load(std::move(normal_texture_path));

	glCreateTextures(GL_TEXTURE_2D, 1, &m_NormalTextureId);
	glTextureStorage2D(m_NormalTextureId, normal_dds.MipmapCount(), normal_dds.Format(), normal_dds.Width(), normal_dds.Height());

	for (auto& mipmap : normal_dds.mipmaps)
	{
		glCompressedTextureSubImage2D(m_NormalTextureId, mipmap.level, 0, 0, mipmap.width, mipmap.height, normal_dds.Format(), mipmap.texture_size, mipmap.data);
	}

	glBindTextureUnit(1, m_NormalTextureId);

	return true;
}

void GlModel::Update(float dt)
{
	static float TimeInSeconds = 0.0f;
	TimeInSeconds += dt * 100.0f;

	auto numBones = m_MeshData->bones.size();
	std::vector<DirectX::XMMATRIX> toParentTransforms(numBones);

	// Animation
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
		std::vector<DirectX::XMMATRIX> tranforms;
		for (size_t i = 0; i < m_MeshData->bones.size(); i++)
		{
			DirectX::XMMATRIX offset = m_MeshData->bones[i].offset;
			DirectX::XMMATRIX toRoot = toRootTransforms[i];
			DirectX::XMMATRIX matrix = DirectX::XMMatrixMultiply(offset, toRoot);
			auto transform = DirectX::XMMatrixTranspose(matrix);

			tranforms.push_back(transform);
		}

		auto bone_pos = glGetUniformLocation(m_Shader->GetShaderId(), "gBoneTransform");
		glUniformMatrix4fv(bone_pos, static_cast<GLsizei>(tranforms.size()), GL_FALSE, reinterpret_cast<float*>(&tranforms[0]));
	}
	else
	{
		auto matrix = DirectX::XMMatrixIdentity();
		auto bone_pos = glGetUniformLocation(m_Shader->GetShaderId(), "gBoneTransform");
		glUniformMatrix4fv(bone_pos, 1, GL_FALSE, reinterpret_cast<float*>(&matrix));
	}
}

struct GlWorld
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX proj;
};

void GlModel::Render(Camera* camera)
{
	GlWorld world = {};
	world.world = DirectX::XMMatrixIdentity();
	world.proj = DirectX::XMMatrixTranspose(camera->GetProjection());
	world.view = DirectX::XMMatrixTranspose(camera->GetView());

	// Update shader transform
	auto world_location = glGetUniformBlockIndex(m_Shader->GetShaderId(), "cWorld");
	glUniformBlockBinding(m_Shader->GetShaderId(), world_location, 0);

	unsigned int uboMatrices;
	glGenBuffers(1, &uboMatrices);

	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(GlWorld), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, sizeof(GlWorld));

	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GlWorld), &world);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// The light man
	auto gDirectionLight = glGetUniformLocation(m_Shader->GetShaderId(), "gDirectionLight");
	DirectX::XMFLOAT4 direction_light(-0.8f, -0.5f, 0.5f, 1.0f);
	glUniform4fv(gDirectionLight, 1, reinterpret_cast<float*>(&direction_light));

	auto gDiffuseLight = glGetUniformLocation(m_Shader->GetShaderId(), "gDiffuseLight");
	DirectX::XMFLOAT4 diffuse_light(1.0f, 1.0f, 1.0f, 1.0f);
	glUniform4fv(gDiffuseLight, 1, reinterpret_cast<float*>(&diffuse_light));

	auto gAmbientLightLoc = glGetUniformLocation(m_Shader->GetShaderId(), "gAmbientLight");
	DirectX::XMFLOAT4 ambient_light(0.5f, 0.5f, 0.5f, 1.0f);
	glUniform4fv(gAmbientLightLoc, 1, reinterpret_cast<float*>(&ambient_light));

	auto gSpecularLight = glGetUniformLocation(m_Shader->GetShaderId(), "gSpecularLight");
	DirectX::XMFLOAT4 specular_light(0.1f, 0.1f, 0.1f, 32.0f);
	glUniform4fv(gSpecularLight, 1, reinterpret_cast<float*>(&specular_light));

	auto gCameraPos = glGetUniformLocation(m_Shader->GetShaderId(), "gCameraPos");
	auto cameraPos = camera->GetPosition();
	glUniform4fv(gCameraPos, 1, reinterpret_cast<float*>(&cameraPos));

	// Draw
	m_Renderer->ApplyVertexBuffer(m_VertexBuffer.get());
	for (auto& subset : m_MeshData->subsets)
	{
		glDrawElementsBaseVertex(GL_TRIANGLES, subset.totalIndex, GL_UNSIGNED_INT, nullptr, subset.baseVertex);
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

//Model::Model(IRenderer* renderer) : m_Renderer(renderer)
//{
//}
//
//bool Model::Load(const std::string& path)
//{
//	m_MeshData = std::make_unique<MeshData>();
//	if (!ModelLoader::Load(path, m_MeshData.get()))
//	{
//		return false;
//	}
//
//	// Create vertex buffer
//	m_Renderer->CreateVertexBuffer(m_MeshData->vertices);
//
//	// Create index buffer
//
//
//	// Create constant buffers
//
//	return true;
//}
//
//void Model::Update(float dt)
//{
//
//}
//
//void Model::Render()
//{
//	// Apply Vertex Buffer
//	// Apply Index Bufer
//	// Apply primitive geometry type
//	// Pass constant values 
//
//	for (auto& subset : m_MeshData->subsets)
//	{
//		// Draw indices for each subset
//	}
//}
// 