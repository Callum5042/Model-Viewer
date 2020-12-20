#pragma once

#include "Renderer.h"

class Shader
{
public:
	Shader(IRenderer* renderer);
	virtual ~Shader() = default;

	bool Create();
	void Use();

private:
	DxRenderer* m_Renderer = nullptr;

	ComPtr<ID3D11InputLayout> m_VertexLayout = nullptr;
	ComPtr<ID3D11VertexShader> m_VertexShader = nullptr;
	ComPtr<ID3D11PixelShader> m_PixelShader = nullptr;

	bool CreateVertexShader(const std::string& vertex_shader_path);
	bool CreatePixelShader(const std::string& pixel_shader_path);
};