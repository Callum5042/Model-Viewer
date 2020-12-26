#pragma once

#include "Pch.h"
class IRenderer;
class DxRenderer;
class Camera;
class ICamera;
class GlShader;
class IShader;
class GlCamera;

struct Position
{
	float x = 0;
	float y = 0;
	float z = 0;
};

struct Colour
{
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	float a = 0.0f;
};

struct Texture
{
	float u = 0;
	float v = 0;
};

struct Normal
{
	float x = 0;
	float y = 0;
	float z = 0;
};

struct Tangent
{
	float x = 0;
	float y = 0;
	float z = 0;
};

struct BiTangent
{
	float x = 0;
	float y = 0;
	float z = 0;
};

struct Vertex
{
	Vertex() {}

	Position position = {};
	Colour colour = {};
	Texture texture = {};
	Normal normal = {};
	Tangent tangent = {};
	BiTangent bi_tangent = {};
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

	// Model buffers
	ComPtr<ID3D11Buffer> m_VertexBuffer = nullptr;
	ComPtr<ID3D11Buffer> m_IndexBuffer = nullptr;
	ComPtr<ID3D11Buffer> m_ConstantBuffer = nullptr;

	// Texture resources
	ComPtr<ID3D11ShaderResourceView> m_DiffuseTexture = nullptr;
	ComPtr<ID3D11ShaderResourceView> m_NormalTexture = nullptr;

	// Light
	ComPtr<ID3D11Buffer> m_LightBuffer = nullptr;
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
	std::unique_ptr<MeshData> m_MeshData = nullptr;

	// Mode buffers
	GLuint m_VertexArrayObject = 0;
	GLuint m_VertexBuffer = 0;
	GLuint m_IndexBuffer = 0;

	// Texture resourcees
	GLuint m_DiffuseTextureId = 0;
};