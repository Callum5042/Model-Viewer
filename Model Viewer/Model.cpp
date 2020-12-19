#include "Pch.h"
#include "Model.h"
#include "Renderer.h"
#include "Camera.h"
#include <DirectXMath.h>

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

Model::Model(IRenderer* renderer)
{
	m_Renderer = reinterpret_cast<DxRenderer*>(renderer);
}

Model::~Model()
{
}

bool Model::Load()
{
	m_MeshData = std::make_unique<MeshData>();
	Geometry::CreateBox(1.0f, 1.0f, 1.0f, m_MeshData.get());

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

void Model::Render(Camera* camera)
{
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
	cb.view = DirectX::XMMatrixTranspose(camera->GetView());
	cb.projection = DirectX::XMMatrixTranspose(camera->GetProjection());

	m_Renderer->GetDeviceContext()->VSSetConstantBuffers(0, 1, m_ConstantBuffer.GetAddressOf());
	m_Renderer->GetDeviceContext()->PSSetConstantBuffers(0, 1, m_ConstantBuffer.GetAddressOf());
	m_Renderer->GetDeviceContext()->UpdateSubresource(m_ConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

	// Render geometry
	m_Renderer->GetDeviceContext()->DrawIndexed(static_cast<UINT>(m_MeshData->indices.size()), 0, 0);
}
