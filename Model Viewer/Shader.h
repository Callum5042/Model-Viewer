#pragma once

#include "Renderer.h"
#include <DirectXMath.h>

namespace ShaderData
{
	// Material
	__declspec(align(16)) struct ShaderMaterial
	{
		DirectX::XMFLOAT4 mDiffuse;
		DirectX::XMFLOAT4 mAmbient;
		DirectX::XMFLOAT4 mSpecular;
	};

	// Camera / World
	_declspec(align(16)) struct WorldBuffer
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
		DirectX::XMMATRIX worldInverse;
		DirectX::XMMATRIX texture;
		ShaderMaterial mMaterial;
	};

	// Directional Light
	_declspec(align(16)) struct DirectionalLight
	{
		DirectX::XMFLOAT4 mDiffuse;
		DirectX::XMFLOAT4 mAmbient;
		DirectX::XMFLOAT4 mSpecular;
		DirectX::XMFLOAT4 mDirection;
		DirectX::XMFLOAT3 mCameraPos;
		float padding;
	};

	// Light view / Shaders?
	_declspec(align(16)) struct LightBuffer
	{
		DirectionalLight mDirectionalLight;
	};

	// Skeletal bones
	_declspec(align(16)) struct BoneBuffer
	{
		DirectX::XMMATRIX transform[96];
	};
}

// Shader interface
class IShader
{
public:
	IShader() = default;
	virtual ~IShader() = default;

	// Create shaders
	virtual bool Create() = 0;

	// Apply shaders to the pipeline
	virtual void Use() = 0;

	// Update World
	virtual void UpdateWorld(const ShaderData::WorldBuffer& data) = 0;

	// Update Lights
	virtual void UpdateLights(const ShaderData::LightBuffer& data) = 0;

	// Update bone data
	virtual void UpdateBones(const ShaderData::BoneBuffer& data) = 0;
};

// Direct3D 11 shader
class DXShader : public IShader
{
public:
	DXShader(IRenderer* renderer);
	virtual ~DXShader() = default;

	// Create shaders
	bool Create() override;

	// Apply shaders to the pipeline
	void Use() override;

	// Update World
	virtual void UpdateWorld(const ShaderData::WorldBuffer& data) override;

	// Update Lights
	virtual void UpdateLights(const ShaderData::LightBuffer& data) override;

	// Update bone data
	virtual void UpdateBones(const ShaderData::BoneBuffer& data) override;

private:
	DXRenderer* m_Renderer = nullptr;

	ComPtr<ID3D11InputLayout> m_VertexLayout = nullptr;
	ComPtr<ID3D11VertexShader> m_VertexShader = nullptr;
	ComPtr<ID3D11PixelShader> m_PixelShader = nullptr;

	bool CreateVertexShader(const std::string& vertex_shader_path);
	bool CreatePixelShader(const std::string& pixel_shader_path);


	ComPtr<ID3D11Buffer> m_WorldBuffer = nullptr;
	ComPtr<ID3D11Buffer> m_LightBuffer = nullptr;
	ComPtr<ID3D11Buffer> m_BoneConstantBuffer = nullptr;
};

// OpenGL 4 shader
class GLShader : public IShader
{
public:
	GLShader(IRenderer* renderer);
	virtual ~GLShader();

	// Create shaders
	bool Create() override;

	// Apply shaders to the pipeline
	void Use() override;

	// Update World
	virtual void UpdateWorld(const ShaderData::WorldBuffer& data) override;

	// Update Lights
	virtual void UpdateLights(const ShaderData::LightBuffer& data) override;

	// Update bone data
	virtual void UpdateBones(const ShaderData::BoneBuffer& data) override;

	constexpr GLuint GetShaderId() { return m_ShaderId; }

private:
	IRenderer* m_Renderer = nullptr;

	GLuint m_ShaderId = -1;
	GLuint m_VertexShader = -1;
	GLuint m_FragmentShader = -1;

	GLuint LoadVertexShader(std::string&& vertexPath);
	GLuint LoadFragmentShader(std::string&& fragmentPath);
	std::string ReadShader(std::string&& filename);
	bool HasCompiled(GLuint shader);
};