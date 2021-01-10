#include "Pch.h"
#include "Model.h"
#include "Renderer.h"
#include "Camera.h"
#include "Shader.h"
#include <DirectXMath.h>
#include "DDSTextureLoader.h"
#include "LoadTextureDDS.h"
#include "ModelLoader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

DxModel::DxModel(IRenderer* renderer)
{
	m_Renderer = reinterpret_cast<DxRenderer*>(renderer);
}

DxModel::~DxModel()
{
}

bool DxModel::Load()
{
	m_MeshData = std::make_unique<MeshData>();
	//std::ifstream file("Data Files/Models/test.bin", std::ios::binary);
	//if (!file.is_open())
	//{
	//	std::cerr << "Big error\n";
	//}

	//// Parse header
	//char magic_number[3];
	//file.read(magic_number, 3);

	//// Get vertex count
	//auto vertex_count = 0;
	//file.read((char*)&vertex_count, sizeof(vertex_count));

	//// Get index count
	//auto index_count = 0;
	//file.read((char*)&index_count, sizeof(index_count));

	//// Get vertices
	//auto vertex_stride = sizeof(Vertex) * vertex_count;
	//auto vertices = new Vertex[vertex_count];
	//file.read((char*)vertices, vertex_stride);

	//// Get indices
	//auto index_stride = sizeof(UINT) * index_count;
	//auto indices = new UINT[index_count];
	//file.read((char*)indices, index_stride);

	//// Copy to MeshData vectors
	//m_MeshData->vertices.assign(&vertices[0], &vertices[vertex_count]);
	//m_MeshData->indices.assign(&indices[0], &indices[index_count]);

	//Subset subset;
	//subset.baseVertex = 0;
	//subset.totalIndex = index_count;
	//m_MeshData->subsets.push_back(subset);

	//delete[] vertices;
	//delete[] indices;


	//if (!ModelLoader::Load("Data Files/Models/simple.glb", m_MeshData.get()))
	//if (!ModelLoader::Load("Data Files/Models/complex_post.glb", m_MeshData.get()))
	if (!ModelLoader::Load("Data Files/Models/crate.glb", m_MeshData.get()))
	{
		return false;
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////

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

	return true;
}

void DxModel::Render(ICamera* camera)
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

	ShaderMaterial material;
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

bool GlModel::Load()
{
	m_MeshData = std::make_unique<MeshData>();
	std::ifstream file("Data Files/Models/test.bin", std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "Big error\n";
	}

	// Parse header
	char magic_number[3];
	file.read(magic_number, 3);

	// Get vertex count
	auto vertex_count = 0;
	file.read((char*)&vertex_count, sizeof(vertex_count));

	// Get index count
	auto index_count = 0;
	file.read((char*)&index_count, sizeof(index_count));

	// Get vertices
	auto vertex_stride = sizeof(Vertex) * vertex_count;
	auto vertices = new Vertex[vertex_count];
	file.read((char*)vertices, vertex_stride);

	// Get indices
	auto index_stride = sizeof(UINT) * index_count;
	auto indices = new UINT[index_count];
	file.read((char*)indices, index_stride);

	// Copy to MeshData vectors
	m_MeshData->vertices.assign(&vertices[0], &vertices[vertex_count]);
	m_MeshData->indices.assign(&indices[0], &indices[index_count]);

	delete[] vertices;
	delete[] indices;

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

	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(3 * sizeof(GL_FLOAT)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(7 * sizeof(GL_FLOAT)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(9 * sizeof(GL_FLOAT)));
	glEnableVertexAttribArray(3);

	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(12 * sizeof(GL_FLOAT)));
	glEnableVertexAttribArray(4);

	glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(15 * sizeof(GL_FLOAT)));
	glEnableVertexAttribArray(5);

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

struct GlWorld
{
	glm::mat4 world;
	glm::mat4 view;
	glm::mat4 proj;
};

void GlModel::Render(ICamera* camera)
{
	auto glCamera = reinterpret_cast<GlCamera*>(camera);

	GlWorld world = {};
	world.world = glm::mat4(1.0f);
	world.view = glCamera->GetView();
	world.proj = glCamera->GetProjection();

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
	auto cameraPos = glCamera->GetPosition();
	glUniform4fv(gCameraPos, 1, glm::value_ptr(cameraPos));

	// Draw
	glBindVertexArray(m_VertexArrayObject);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_MeshData->indices.size()), GL_UNSIGNED_INT, nullptr);
}

float BoneAnimation::GetStartTime() const
{
	// Keyframes are sorted by time, so first keyframe gives start time.
	return Keyframes.front().TimePos;
}

float BoneAnimation::GetEndTime() const
{
	// Keyframes are sorted by time, so last keyframe gives end time.
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
		//XMStoreFloat4x4(&M, DirectX::XMMatrixAffineTransformation(S, zero, Q, P));
	}
	else if (t >= Keyframes.back().TimePos)
	{
		DirectX::XMVECTOR S = XMLoadFloat3(&Keyframes.back().Scale);
		DirectX::XMVECTOR P = XMLoadFloat3(&Keyframes.back().Translation);
		DirectX::XMVECTOR Q = XMLoadFloat4(&Keyframes.back().RotationQuat);

		DirectX::XMVECTOR zero = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		M = DirectX::XMMatrixAffineTransformation(S, zero, Q, P);
		//XMStoreFloat4x4(&M, DirectX::XMMatrixAffineTransformation(S, zero, Q, P));
	}
	else
	{
		for (UINT i = 0; i < Keyframes.size() - 1; ++i)
		{
			if (t >= Keyframes[i].TimePos && t <= Keyframes[i + 1].TimePos)
			{
				float lerpPercent = (t - Keyframes[i].TimePos) / (Keyframes[i + 1].TimePos - Keyframes[i].TimePos);

				DirectX::XMVECTOR s0 = XMLoadFloat3(&Keyframes[i].Scale);
				DirectX::XMVECTOR s1 = XMLoadFloat3(&Keyframes[i + 1].Scale);

				DirectX::XMVECTOR p0 = XMLoadFloat3(&Keyframes[i].Translation);
				DirectX::XMVECTOR p1 = XMLoadFloat3(&Keyframes[i + 1].Translation);

				DirectX::XMVECTOR q0 = XMLoadFloat4(&Keyframes[i].RotationQuat);
				DirectX::XMVECTOR q1 = XMLoadFloat4(&Keyframes[i + 1].RotationQuat);

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