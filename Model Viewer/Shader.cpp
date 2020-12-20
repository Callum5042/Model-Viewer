#include "Pch.h"
#include "Shader.h"
#include <SDL_messagebox.h>
#include <fstream>

Shader::Shader(IRenderer* renderer)
{
	m_Renderer = reinterpret_cast<DxRenderer*>(renderer);
}

bool Shader::Create()
{
	if (!CreateVertexShader("VertexShader.cso"))
		return false;

	if (!CreatePixelShader("PixelShader.cso"))
		return false;

	return true;
}

void Shader::Use()
{
	m_Renderer->GetDeviceContext()->IASetInputLayout(m_VertexLayout.Get());
	m_Renderer->GetDeviceContext()->VSSetShader(m_VertexShader.Get(), nullptr, 0);
	m_Renderer->GetDeviceContext()->PSSetShader(m_PixelShader.Get(), nullptr, 0);
}

bool Shader::CreateVertexShader(const std::string& vertex_shader_path)
{
	std::ifstream vertexFile(vertex_shader_path, std::fstream::in | std::fstream::binary);
	if (!vertexFile.is_open())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Could not read VertexShader.cso", nullptr);
		return false;
	}

	vertexFile.seekg(0, vertexFile.end);
	int vertexsize = static_cast<int>(vertexFile.tellg());
	vertexFile.seekg(0, vertexFile.beg);

	char* vertexbuffer = new char[vertexsize];
	vertexFile.read(vertexbuffer, vertexsize);

	DX::ThrowIfFailed(m_Renderer->GetDevice()->CreateVertexShader(vertexbuffer, vertexsize, nullptr, m_VertexShader.ReleaseAndGetAddressOf()));

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = ARRAYSIZE(layout);
	DX::ThrowIfFailed(m_Renderer->GetDevice()->CreateInputLayout(layout, numElements, vertexbuffer, vertexsize, m_VertexLayout.ReleaseAndGetAddressOf()));

	delete[] vertexbuffer;
	return true;
}

bool Shader::CreatePixelShader(const std::string& pixel_shader_path)
{
	std::ifstream pixelFile(pixel_shader_path, std::fstream::in | std::fstream::binary);
	if (!pixelFile.is_open())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Could not read PixelShader.cso", nullptr);
		return false;
	}

	pixelFile.seekg(0, pixelFile.end);
	int pixelsize = static_cast<int>(pixelFile.tellg());
	pixelFile.seekg(0, pixelFile.beg);

	char* pixelbuffer = new char[pixelsize];
	pixelFile.read(pixelbuffer, pixelsize);

	DX::ThrowIfFailed(m_Renderer->GetDevice()->CreatePixelShader(pixelbuffer, pixelsize, nullptr, m_PixelShader.ReleaseAndGetAddressOf()));

	delete[] pixelbuffer;
	return true;
}
