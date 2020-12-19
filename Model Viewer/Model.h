#pragma once

#include "Pch.h"
class IRenderer;
class DxRenderer;
class Camera;

struct Vertex
{
	Vertex() {}
	Vertex(float x, float y, float z) : x(x), y(y), z(z) {}

	float x = 0;
	float y = 0;
	float z = 0;
};

struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<UINT> indices;
};

class Model
{
public:
	Model(IRenderer* renderer);
	virtual ~Model();

	bool Load();
	void Render(Camera* camera);

private:
	DxRenderer* m_Renderer = nullptr;
	std::unique_ptr<MeshData> m_MeshData = nullptr;

	ComPtr<ID3D11Buffer> m_VertexBuffer = nullptr;
	ComPtr<ID3D11Buffer> m_IndexBuffer = nullptr;

	ComPtr<ID3D11Buffer> m_ConstantBuffer = nullptr;
};