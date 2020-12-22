#include "Pch.h"
#include "Model.h"
#include "Renderer.h"
#include "Camera.h"
#include <DirectXMath.h>
#include "Shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

_declspec(align(16)) struct ConstantBuffer
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
};

namespace Geometry
{
	void CreateBox(float width, float height, float depth, MeshData* mesh)
	{
		Vertex vertices[] =
		{
			{ -width, -height, -depth },
			{ -width, +height, -depth },
			{ +width, +height, -depth },
			{ +width, -height, -depth },

			{ -width, -height, +depth },
			{ +width, -height, +depth },
			{ +width, +height, +depth },
			{ -width, +height, +depth },

			{ -width, +height, -depth },
			{ -width, +height, +depth },
			{ +width, +height, +depth },
			{ +width, +height, -depth },

			{ -width, -height, -depth },
			{ +width, -height, -depth },
			{ +width, -height, +depth },
			{ -width, -height, +depth },

			{ -width, -height, +depth },
			{ -width, +height, +depth },
			{ -width, +height, -depth },
			{ -width, -height, -depth },

			{ +width, -height, -depth },
			{ +width, +height, -depth },
			{ +width, +height, +depth },
			{ +width, -height, +depth }
		};

		unsigned int indices[] =
		{
			0, 1, 2,
			0, 2, 3,

			4, 5, 6,
			4, 6, 7,

			8, 9, 10,
			8, 10, 11,

			12, 13, 14,
			12, 14, 15,

			16, 17, 18,
			16, 18, 19,

			20, 21, 22,
			20, 22, 23,
		};

		mesh->vertices.assign(&vertices[0], &vertices[24]);
		mesh->indices.assign(&indices[0], &indices[36]);
	}
}

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
	//Geometry::CreateBox(1.0f, 1.0f, 1.0f, m_MeshData.get());

	std::ifstream file("D:\\test.bin", std::ios::binary);
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
	int vertex_stride = sizeof(Vertex) * vertex_count;
	auto vertices = new Vertex[vertex_count];
	file.read((char*)vertices, vertex_stride);

	// Get indices
	int index_stride = sizeof(UINT) * index_count;
	auto indices = new UINT[index_count];
	file.read((char*)indices, index_stride);

	// Copy to MeshData vectors
	m_MeshData->vertices.assign(&vertices[0], &vertices[vertex_count]);
	m_MeshData->indices.assign(&indices[0], &indices[index_count]);

	delete[] vertices;
	delete[] indices;


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
	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();

	ConstantBuffer cb = {};
	cb.world = DirectX::XMMatrixTranspose(world);
	cb.view = DirectX::XMMatrixTranspose(dxCamera->GetView());
	cb.projection = DirectX::XMMatrixTranspose(dxCamera->GetProjection());

	m_Renderer->GetDeviceContext()->VSSetConstantBuffers(0, 1, m_ConstantBuffer.GetAddressOf());
	m_Renderer->GetDeviceContext()->PSSetConstantBuffers(0, 1, m_ConstantBuffer.GetAddressOf());
	m_Renderer->GetDeviceContext()->UpdateSubresource(m_ConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

	// Render geometry
	m_Renderer->GetDeviceContext()->DrawIndexed(static_cast<UINT>(m_MeshData->indices.size()), 0, 0);
}

GlModel::GlModel(IShader* shader)
{
	m_Shader = reinterpret_cast<GlShader*>(shader);
}

GlModel::~GlModel()
{
	glDeleteVertexArrays(1, &m_VertexArrayObject);
	glDeleteBuffers(1, &m_VertexBuffer);
	glDeleteBuffers(1, &m_IndexBuffer);
}

bool GlModel::Load()
{
	m_MeshData = std::make_unique<MeshData>();
	//Geometry::CreateBox(1.0f, 1.0f, 1.0f, m_MeshData.get());

	std::ifstream file("D:\\test.bin", std::ios::binary);
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
	int vertex_stride = sizeof(Vertex) * vertex_count;
	auto vertices = new Vertex[vertex_count];
	file.read((char*)vertices, vertex_stride);

	// Get indices
	int index_stride = sizeof(UINT) * index_count;
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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GL_FLOAT), (GLvoid*)(3 * sizeof(GL_FLOAT)));
	glEnableVertexAttribArray(1);

	return true;
}

void GlModel::Render(ICamera* camera)
{
	auto glCamera = reinterpret_cast<GlCamera*>(camera);

	// Update shader transform
	glm::mat4 transform = glm::mat4(1.0f);
	unsigned int transformLoc = glGetUniformLocation(m_Shader->GetShaderId(), "transform");
	glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

	// Update shader view
	unsigned int viewLoc = glGetUniformLocation(m_Shader->GetShaderId(), "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(glCamera->GetView()));

	// Update shader projection
	unsigned int projLoc = glGetUniformLocation(m_Shader->GetShaderId(), "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(glCamera->GetProjection()));

	// Draw
	glBindVertexArray(m_VertexArrayObject);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
}
