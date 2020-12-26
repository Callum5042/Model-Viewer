#include "Pch.h"
#include "Shader.h"
#include <SDL_messagebox.h>
#include <fstream>

DxShader::DxShader(IRenderer* renderer)
{
	m_Renderer = reinterpret_cast<DxRenderer*>(renderer);
}

bool DxShader::Create()
{
	if (!CreateVertexShader("Data Files/Shaders/VertexShader.cso"))
	{
		return false;
	}

	if (!CreatePixelShader("Data Files/Shaders/PixelShader.cso"))
	{
		return false;
	}

	return true;
}

void DxShader::Use()
{
	m_Renderer->GetDeviceContext()->IASetInputLayout(m_VertexLayout.Get());
	m_Renderer->GetDeviceContext()->VSSetShader(m_VertexShader.Get(), nullptr, 0);
	m_Renderer->GetDeviceContext()->PSSetShader(m_PixelShader.Get(), nullptr, 0);
}

bool DxShader::CreateVertexShader(const std::string& vertex_shader_path)
{
	std::ifstream vertexFile(vertex_shader_path, std::fstream::in | std::fstream::binary);
	if (!vertexFile.is_open())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Could not read VertexShader.cso", nullptr);
		return false;
	}

	vertexFile.seekg(0, vertexFile.end);
	auto vertexsize = static_cast<int>(vertexFile.tellg());
	vertexFile.seekg(0, vertexFile.beg);

	auto vertexbuffer = new char[vertexsize];
	vertexFile.read(vertexbuffer, vertexsize);

	DX::ThrowIfFailed(m_Renderer->GetDevice()->CreateVertexShader(vertexbuffer, vertexsize, nullptr, m_VertexShader.ReleaseAndGetAddressOf()));

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOUR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BITTANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 60, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = ARRAYSIZE(layout);
	DX::ThrowIfFailed(m_Renderer->GetDevice()->CreateInputLayout(layout, numElements, vertexbuffer, vertexsize, m_VertexLayout.ReleaseAndGetAddressOf()));

	delete[] vertexbuffer;
	return true;
}

bool DxShader::CreatePixelShader(const std::string& pixel_shader_path)
{
	std::ifstream pixelFile(pixel_shader_path, std::fstream::in | std::fstream::binary);
	if (!pixelFile.is_open())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Could not read PixelShader.cso", nullptr);
		return false;
	}

	pixelFile.seekg(0, pixelFile.end);
	auto pixelsize = static_cast<int>(pixelFile.tellg());
	pixelFile.seekg(0, pixelFile.beg);

	auto pixelbuffer = new char[pixelsize];
	pixelFile.read(pixelbuffer, pixelsize);

	DX::ThrowIfFailed(m_Renderer->GetDevice()->CreatePixelShader(pixelbuffer, pixelsize, nullptr, m_PixelShader.ReleaseAndGetAddressOf()));

	delete[] pixelbuffer;
	return true;
}

GlShader::~GlShader()
{
}

bool GlShader::Create()
{
	m_ShaderId = glCreateProgram();
	m_VertexShader = LoadVertexShader("Data Files/Shaders/VertexShader.glsl");
	m_FragmentShader = LoadFragmentShader("Data Files/Shaders/FragmentShader.glsl");

	// Link
	glAttachShader(m_ShaderId, m_VertexShader);
	glAttachShader(m_ShaderId, m_FragmentShader);

	glLinkProgram(m_ShaderId);
	return true;
}

void GlShader::Use()
{
	glUseProgram(m_ShaderId);
}

GLuint GlShader::LoadVertexShader(std::string&& vertexPath)
{
	auto vertexShader = glCreateShader(GL_VERTEX_SHADER);

	auto vertexShaderSource = ReadShader(std::move(vertexPath));
	auto vertexC = vertexShaderSource.c_str();

	glShaderSource(vertexShader, 1, &vertexC, NULL);
	glCompileShader(vertexShader);

	if (!HasCompiled(vertexShader))
	{
		return false;
	}

	return vertexShader;
}

GLuint GlShader::LoadFragmentShader(std::string&& fragmentPath)
{
	auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	auto fragmentShaderSource = ReadShader(std::move(fragmentPath));
	auto fragmentC = fragmentShaderSource.c_str();

	glShaderSource(fragmentShader, 1, &fragmentC, NULL);
	glCompileShader(fragmentShader);

	if (!HasCompiled(fragmentShader))
	{
		return false;
	}

	return fragmentShader;
}

std::string GlShader::ReadShader(std::string&& filename)
{
	std::ifstream file(filename);

	std::stringstream source;
	source << file.rdbuf();

	return source.str();
}

bool GlShader::HasCompiled(GLuint shader)
{
	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
#ifdef _DEBUG
		GLsizei len;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

#pragma warning(suppress: 26451)
		GLchar* log = new GLchar[len + 1];
		glGetShaderInfoLog(shader, len, &len, log);
		std::cerr << "Shader compilation failed: " << log << std::endl;
		delete[] log;
#endif

		return false;
	}

	return true;
}
