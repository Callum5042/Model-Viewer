#pragma once

#include "Renderer.h"

// Shader interface
class IShader
{
public:
	IShader() = default;
	virtual ~IShader() = default;

	virtual bool Create() = 0;
	virtual void Use() = 0;
};

// Direct3D 11 shader
class DxShader : public IShader
{
public:
	DxShader(IRenderer* renderer);
	virtual ~DxShader() = default;

	bool Create() override;
	void Use() override;

private:
	DxRenderer* m_Renderer = nullptr;

	ComPtr<ID3D11InputLayout> m_VertexLayout = nullptr;
	ComPtr<ID3D11VertexShader> m_VertexShader = nullptr;
	ComPtr<ID3D11PixelShader> m_PixelShader = nullptr;

	bool CreateVertexShader(const std::string& vertex_shader_path);
	bool CreatePixelShader(const std::string& pixel_shader_path);
};

// OpenGL 4 shader
class GlShader : public IShader
{
public:
	GlShader() = default;
	virtual ~GlShader();

	bool Create() override;
	void Use() override;

	constexpr GLuint GetShaderId() { return m_ShaderId; }

private:
	GLuint m_ShaderId = -1;
	GLuint m_VertexShader = -1;
	GLuint m_FragmentShader = -1;

	GLuint LoadVertexShader(std::string&& vertexPath);
	GLuint LoadFragmentShader(std::string&& fragmentPath);
	std::string ReadShader(std::string&& filename);
	bool HasCompiled(GLuint shader);
};