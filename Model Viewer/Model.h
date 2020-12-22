#pragma once

#include "Pch.h"
class IRenderer;
class DxRenderer;
class Camera;
class ICamera;
class GlShader;
class IShader;
class GlCamera;

struct Colour
{
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	float a = 0.0f;
};

struct Vertex
{
	Vertex() {}
	Vertex(float x, float y, float z) : x(x), y(y), z(z) {}

	float x = 0;
	float y = 0;
	float z = 0;
	Colour colour = {};
};

struct MeshData
{
	MeshData() = default;
	virtual ~MeshData() = default;

	std::vector<Vertex> vertices;
	std::vector<UINT> indices;
};

class IModel
{
public:
	IModel() = default;
	virtual ~IModel() = default;

	virtual bool Load() = 0;
	virtual void Render(ICamera* camera) = 0;
};

class DxModel : public IModel
{
public:
	DxModel(IRenderer* renderer);
	virtual ~DxModel();

	bool Load() override;
	void Render(ICamera* camera) override;

private:
	DxRenderer* m_Renderer = nullptr;
	std::unique_ptr<MeshData> m_MeshData = nullptr;

	ComPtr<ID3D11Buffer> m_VertexBuffer = nullptr;
	ComPtr<ID3D11Buffer> m_IndexBuffer = nullptr;
	ComPtr<ID3D11Buffer> m_ConstantBuffer = nullptr;
};

class GlModel : public IModel
{
public:
	GlModel(IShader* shader);
	virtual ~GlModel();

	bool Load() override;
	void Render(ICamera* camera) override;

private:
	GlShader* m_Shader = nullptr;

	GLuint m_VertexArrayObject = 0;
	GLuint m_VertexBuffer = 0;
	GLuint m_IndexBuffer = 0;

	std::unique_ptr<MeshData> m_MeshData = nullptr;
};