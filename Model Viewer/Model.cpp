#include "Pch.h"
#include "Model.h"
#include "Renderer.h"
#include "Camera.h"
#include "Shader.h"
#include <DirectXMath.h>
#include "DDSTextureLoader.h"
#include "LoadTextureDDS.h"
#include "ModelLoader.h"

#define GLM_FORCE_PURE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace
{
	glm::mat4 DXMatrixToGLM(const DirectX::XMMATRIX& dx_matrix)
	{
		DirectX::XMFLOAT4X4 m;
		DirectX::XMStoreFloat4x4(&m, dx_matrix);

		glm::mat4 mat(
			m._11, m._21, m._31, m._41,
			m._12, m._22, m._32, m._42,
			m._13, m._23, m._33, m._43,
			m._14, m._24, m._34, m._44);

		return mat;
	}
}

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
	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = static_cast<UINT>(sizeof(Vertex) * m_MeshData->vertices.size());
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vInitData = {};
	vInitData.pSysMem = &m_MeshData->vertices[0];

	DX::ThrowIfFailed(m_Renderer->GetDevice()->CreateBuffer(&vbd, &vInitData, m_VertexBuffer.ReleaseAndGetAddressOf()));

	// Create index buffer
	D3D11_BUFFER_DESC ibd = {};
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.ByteWidth = static_cast<UINT>(sizeof(UINT) * m_MeshData->indices.size());
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA iInitData = {};
	iInitData.pSysMem = &m_MeshData->indices[0];

	DX::ThrowIfFailed(m_Renderer->GetDevice()->CreateBuffer(&ibd, &iInitData, m_IndexBuffer.ReleaseAndGetAddressOf()));

	// Constant buffer
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	DX::ThrowIfFailed(m_Renderer->GetDevice()->CreateBuffer(&bd, nullptr, m_ConstantBuffer.ReleaseAndGetAddressOf()));

	// Load mr texture
	ComPtr<ID3D11Resource> resource = nullptr;
	DX::ThrowIfFailed(DirectX::CreateDDSTextureFromFile(m_Renderer->GetDevice().Get(), L"Data Files\\Textures\\crate_diffuse.dds", resource.ReleaseAndGetAddressOf(), m_DiffuseTexture.ReleaseAndGetAddressOf()));

	DX::ThrowIfFailed(DirectX::CreateDDSTextureFromFile(m_Renderer->GetDevice().Get(), L"Data Files\\Textures\\crate_normal.dds", resource.ReleaseAndGetAddressOf(), m_NormalTexture.ReleaseAndGetAddressOf()));

	// Mr sun
	D3D11_BUFFER_DESC lbd = {};
	lbd.Usage = D3D11_USAGE_DEFAULT;
	lbd.ByteWidth = sizeof(LightBuffer);
	lbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	DX::ThrowIfFailed(m_Renderer->GetDevice()->CreateBuffer(&lbd, nullptr, m_LightBuffer.GetAddressOf()));

	// Mr bone buffer
	D3D11_BUFFER_DESC bone_bd = {};
	bone_bd.Usage = D3D11_USAGE_DEFAULT;
	bone_bd.ByteWidth = sizeof(BoneBuffer);
	bone_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	DX::ThrowIfFailed(m_Renderer->GetDevice()->CreateBuffer(&bone_bd, nullptr, m_BoneConstantBuffer.ReleaseAndGetAddressOf()));

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

void DxModel::Render(Camera* camera, GlCamera* glCamera)
{
	auto dxCamera = reinterpret_cast<Camera*>(camera);

	// Bind the vertex buffer
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	m_Renderer->GetDeviceContext()->IASetVertexBuffers(0, 1, m_VertexBuffer.GetAddressOf(), &stride, &offset);

	// Bind the index buffer
	m_Renderer->GetDeviceContext()->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// Set topology
	m_Renderer->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set buffer
	auto world = DirectX::XMMatrixIdentity();

	ConstantBuffer cb = {};
	cb.world = DirectX::XMMatrixTranspose(world);
	cb.view = DirectX::XMMatrixTranspose(dxCamera->GetView());
	cb.projection = DirectX::XMMatrixTranspose(dxCamera->GetProjection());
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
	lightBuffer.mDirectionalLight.mCameraPos = dxCamera->GetPosition();
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

GlModel::GlModel(IShader* shader)
{
	m_Shader = reinterpret_cast<GlShader*>(shader);
}

GlModel::~GlModel()
{
	glDeleteTextures(1, &m_NormalTextureId);
	glDeleteTextures(1, &m_DiffuseTextureId);
	glDeleteVertexArrays(1, &m_VertexArrayObject);
	glDeleteBuffers(1, &m_VertexBuffer);
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
	glCreateVertexArrays(1, &m_VertexArrayObject);
	glBindVertexArray(m_VertexArrayObject);

	// Vertex Buffer
	glCreateBuffers(1, &m_VertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);

	glNamedBufferStorage(m_VertexBuffer, sizeof(Vertex) * m_MeshData->vertices.size(), nullptr, GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferSubData(m_VertexBuffer, 0, sizeof(Vertex) * m_MeshData->vertices.size(), &m_MeshData->vertices[0]);

	// Index Buffer
	glCreateBuffers(1, &m_IndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * m_MeshData->indices.size(), &m_MeshData->indices[0], GL_STATIC_DRAW);

	// Something pipeline
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(12));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(28));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(36));
	glEnableVertexAttribArray(3);

	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(48));
	glEnableVertexAttribArray(4);

	glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(60));
	glEnableVertexAttribArray(5);

	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(72));
	glEnableVertexAttribArray(6);

	glVertexAttribIPointer(7, 4, GL_INT, sizeof(Vertex), (GLvoid*)(88));
	glEnableVertexAttribArray(7);

	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		std::cout << "Error 123: " << err;
	}

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
		std::vector<glm::mat4> tranforms;
		for (size_t i = 0; i < m_MeshData->bones.size(); i++)
		{
			DirectX::XMMATRIX offset = m_MeshData->bones[i].offset;
			DirectX::XMMATRIX toRoot = toRootTransforms[i];
			DirectX::XMMATRIX matrix = DirectX::XMMatrixMultiply(offset, toRoot);
			auto transform = DirectX::XMMatrixTranspose(matrix);

			auto glm_matrix = DXMatrixToGLM(transform);
			tranforms.push_back(glm_matrix);
		}

		auto bone_pos = glGetUniformLocation(m_Shader->GetShaderId(), "gBoneTransform");
		glUniformMatrix4fv(bone_pos, static_cast<GLsizei>(tranforms.size()), GL_TRUE, glm::value_ptr(tranforms[0]));
	}
	else
	{
		auto glm_matrix = glm::mat4(1.0f);
		auto bone_pos = glGetUniformLocation(m_Shader->GetShaderId(), "gBoneTransform");
		glUniformMatrix4fv(bone_pos, 1, GL_TRUE, glm::value_ptr(glm_matrix));
	}
}

struct GlWorld
{
	glm::mat4 world;
	glm::mat4 view;
	glm::mat4 proj;
};

#include <glm/gtx/string_cast.hpp>
std::ostream& operator<<(std::ostream& os, glm::mat4 m)
{
	os << glm::to_string(m);
	return os;
}

void GlModel::Render(Camera* camera, GlCamera* glCamera)
{
	glm::mat4 world_matrix = glm::mat4(1.0f);

	GlWorld world = {};
	world.world = world_matrix;
	world.proj = DXMatrixToGLM(camera->GetProjection());
	world.view = DXMatrixToGLM(camera->GetView());

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
	glm::vec4 direction_light(-0.8f, -0.5f, 0.5f, 1.0f);
	glUniform4fv(gDirectionLight, 1, glm::value_ptr(direction_light));

	auto gDiffuseLight = glGetUniformLocation(m_Shader->GetShaderId(), "gDiffuseLight");
	glm::vec4 diffuse_light(1.0f, 1.0f, 1.0f, 1.0f);
	glUniform4fv(gDiffuseLight, 1, glm::value_ptr(diffuse_light));

	auto gAmbientLightLoc = glGetUniformLocation(m_Shader->GetShaderId(), "gAmbientLight");
	glm::vec4 ambient_light(0.5f, 0.5f, 0.5f, 1.0f);
	glUniform4fv(gAmbientLightLoc, 1, glm::value_ptr(ambient_light));

	auto gSpecularLight = glGetUniformLocation(m_Shader->GetShaderId(), "gSpecularLight");
	glm::vec4 specular_light(0.1f, 0.1f, 0.1f, 32.0f);
	glUniform4fv(gSpecularLight, 1, glm::value_ptr(specular_light));

	auto gCameraPos = glGetUniformLocation(m_Shader->GetShaderId(), "gCameraPos");
	auto cameraPos = camera->GetPosition();
	auto camera_pos = glm::vec3(cameraPos.x, cameraPos.y, cameraPos.z);
	glUniform4fv(gCameraPos, 1, glm::value_ptr(camera_pos));

	// Draw
	glBindVertexArray(m_VertexArrayObject);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_MeshData->indices.size()), GL_UNSIGNED_INT, nullptr);
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
	float t = FLT_MAX;
	for (UINT i = 0; i < BoneAnimations.size(); ++i)
	{
		t = std::min(t, BoneAnimations[i].GetStartTime());
	}

	return t;
}

float AnimationClip::GetClipEndTime() const
{
	// Find largest end time over all bones in this clip.
	float t = 0.0f;
	for (UINT i = 0; i < BoneAnimations.size(); ++i)
	{
		t = std::max(t, BoneAnimations[i].GetEndTime());
	}

	return t;
}

void AnimationClip::Interpolate(float t, std::vector<DirectX::XMMATRIX>& boneTransforms) const
{
	for (UINT i = 0; i < BoneAnimations.size(); ++i)
	{
		BoneAnimations[i].Interpolate(t, boneTransforms[i]);
	}
}