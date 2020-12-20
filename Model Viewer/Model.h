#pragma once

#include "Pch.h"
class IRenderer;
class DxRenderer;
class Camera;

struct Vertex
{
	Vertex() {}
	Vertex(float x, float y, float z) : x(x), y(y), z(z) {}
	Vertex(float x, float y, float z, float r, float g, float b) : x(x), y(y), z(z), r(r), g(g), b(b) {}

	float x = 0;
	float y = 0;
	float z = 0;

	float r;
	float g;
	float b;
};

struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<UINT> indices;
};

class IModel
{
public:
	IModel() = default;
	virtual ~IModel() = default;

	virtual bool Load() = 0;
	virtual void Render() = 0;
};

class DxModel : public IModel
{
public:
	DxModel(IRenderer* renderer, Camera* camera);
	virtual ~DxModel();

	bool Load() override;
	void Render() override;

private:
	DxRenderer* m_Renderer = nullptr;
	Camera* m_Camera = nullptr;
	std::unique_ptr<MeshData> m_MeshData = nullptr;

	ComPtr<ID3D11Buffer> m_VertexBuffer = nullptr;
	ComPtr<ID3D11Buffer> m_IndexBuffer = nullptr;

	ComPtr<ID3D11Buffer> m_ConstantBuffer = nullptr;
};

class GlShader;
class IShader;
class GlCamera;

class GlModel : public IModel
{
public:
	GlModel(IShader* shader, GlCamera* camera);
	virtual ~GlModel();

	bool Load() override;
	void Render() override;

private:
	GlShader* m_Shader = nullptr;
	GlCamera* m_Camera = nullptr;

	GLuint m_VertexArrayObject = 0;
	GLuint m_VertexBuffer = 0;
	GLuint m_IndexBuffer = 0;

	std::unique_ptr<MeshData> m_MeshData = nullptr;
};