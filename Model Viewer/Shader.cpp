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
	if (!CreateVertexShader("VertexShader.cso"))
	{
		return false;
	}

	if (!CreatePixelShader("PixelShader.cso"))
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
	int vertexsize = static_cast<int>(vertexFile.tellg());
	vertexFile.seekg(0, vertexFile.beg);

	char* vertexbuffer = new char[vertexsize];
	vertexFile.read(vertexbuffer, vertexsize);

	DX::ThrowIfFailed(m_Renderer->GetDevice()->CreateVertexShader(vertexbuffer, vertexsize, nullptr, m_VertexShader.ReleaseAndGetAddressOf()));

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOUR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
	int pixelsize = static_cast<int>(pixelFile.tellg());
	pixelFile.seekg(0, pixelFile.beg);

	char* pixelbuffer = new char[pixelsize];
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
	m_VertexShader = LoadVertexShader("VertexShader.glsl");
	m_FragmentShader = LoadFragmentShader("FragmentShader.glsl");

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
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

	std::string vertexShaderSource = ReadShader(std::move(vertexPath));
	const GLchar* vertexC = vertexShaderSource.c_str();

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
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	std::string fragmentShaderSource = ReadShader(std::move(fragmentPath));
	const GLchar* fragmentC = fragmentShaderSource.c_str();

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
