#include "Pch.h"
#include "Shader.h"
#include <SDL_messagebox.h>
#include <fstream>
#include "Model.h"

DxShader::DxShader(IRenderer* renderer)
{
	m_Renderer = reinterpret_cast<DXRenderer*>(renderer);
}

bool DxShader::Create()
{
	if (!CreateVertexShader("Data Files/Shaders/VertexShader.cso"))
		return false;

	if (!CreatePixelShader("Data Files/Shaders/PixelShader.cso"))
		return false;

	// World buffer
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ShaderData::WorldBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	DX::Check(m_Renderer->GetDevice()->CreateBuffer(&bd, nullptr, m_WorldBuffer.ReleaseAndGetAddressOf()));

	// Light buffer
	D3D11_BUFFER_DESC lbd = {};
	lbd.Usage = D3D11_USAGE_DEFAULT;
	lbd.ByteWidth = sizeof(ShaderData::LightBuffer);
	lbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	DX::Check(m_Renderer->GetDevice()->CreateBuffer(&lbd, nullptr, m_LightBuffer.ReleaseAndGetAddressOf()));

	// Bone buffer
	D3D11_BUFFER_DESC bone_bd = {};
	bone_bd.Usage = D3D11_USAGE_DEFAULT;
	bone_bd.ByteWidth = sizeof(ShaderData::BoneBuffer);
	bone_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	DX::Check(m_Renderer->GetDevice()->CreateBuffer(&bone_bd, nullptr, m_BoneConstantBuffer.ReleaseAndGetAddressOf()));

	return true;
}

void DxShader::Use()
{
	m_Renderer->GetDeviceContext()->IASetInputLayout(m_VertexLayout.Get());
	m_Renderer->GetDeviceContext()->VSSetShader(m_VertexShader.Get(), nullptr, 0);
	m_Renderer->GetDeviceContext()->PSSetShader(m_PixelShader.Get(), nullptr, 0);
}

void DxShader::UpdateWorld(const ShaderData::WorldBuffer& data)
{
	m_Renderer->GetDeviceContext()->VSSetConstantBuffers(0, 1, m_WorldBuffer.GetAddressOf());
	m_Renderer->GetDeviceContext()->PSSetConstantBuffers(0, 1, m_WorldBuffer.GetAddressOf());
	m_Renderer->GetDeviceContext()->UpdateSubresource(m_WorldBuffer.Get(), 0, nullptr, &data, 0, 0);
}

void DxShader::UpdateLights(const ShaderData::LightBuffer& data)
{
	m_Renderer->GetDeviceContext()->VSSetConstantBuffers(1, 1, m_LightBuffer.GetAddressOf());
	m_Renderer->GetDeviceContext()->PSSetConstantBuffers(1, 1, m_LightBuffer.GetAddressOf());
	m_Renderer->GetDeviceContext()->UpdateSubresource(m_LightBuffer.Get(), 0, nullptr, &data, 0, 0);
}

void DxShader::UpdateBones(const ShaderData::BoneBuffer& data)
{
	m_Renderer->GetDeviceContext()->VSSetConstantBuffers(2, 1, m_BoneConstantBuffer.GetAddressOf());
	m_Renderer->GetDeviceContext()->UpdateSubresource(m_BoneConstantBuffer.Get(), 0, nullptr, &data, 0, 0);
}

bool DxShader::CreateVertexShader(const std::string& vertex_shader_path)
{
	std::ifstream file(vertex_shader_path, std::fstream::in | std::fstream::binary);
	if (!file.is_open())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Could not read VertexShader.cso", nullptr);
		return false;
	}

	std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	DX::Check(m_Renderer->GetDevice()->CreateVertexShader(data.data(), data.size(), nullptr, m_VertexShader.ReleaseAndGetAddressOf()));

	// Create vertex input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOUR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BITTANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 60, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 72, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, 88, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = ARRAYSIZE(layout);
	DX::Check(m_Renderer->GetDevice()->CreateInputLayout(layout, numElements, data.data(), data.size(), m_VertexLayout.ReleaseAndGetAddressOf()));

	return true;
}

bool DxShader::CreatePixelShader(const std::string& pixel_shader_path)
{
	std::ifstream file(pixel_shader_path, std::fstream::in | std::fstream::binary);
	if (!file.is_open())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Could not read PixelShader.cso", nullptr);
		return false;
	}

	std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	DX::Check(m_Renderer->GetDevice()->CreatePixelShader(data.data(), data.size(), nullptr, m_PixelShader.ReleaseAndGetAddressOf()));
	return true;
}

GlShader::GlShader(IRenderer* renderer) : m_Renderer(renderer)
{
}

GlShader::~GlShader()
{
}

bool GlShader::Create()
{
	m_ShaderId = glCreateProgram();
	m_VertexShader = LoadVertexShader("Data Files/Shaders/VertexShader.vs");
	m_FragmentShader = LoadFragmentShader("Data Files/Shaders/FragmentShader.fs");

	// Link
	glAttachShader(m_ShaderId, m_VertexShader);
	glAttachShader(m_ShaderId, m_FragmentShader);

	glLinkProgram(m_ShaderId);
	return true;
}

void GlShader::Use()
{
	// Something pipeline
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(12));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(28));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(36));
	glEnableVertexAttribArray(3);

	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(48));
	glEnableVertexAttribArray(4);

	glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(60));
	glEnableVertexAttribArray(5);

	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(72));
	glEnableVertexAttribArray(6);

	glVertexAttribIPointer(7, 4, GL_INT, sizeof(Vertex), (GLvoid*)(88));
	glEnableVertexAttribArray(7);

	glUseProgram(m_ShaderId);
}

void GlShader::UpdateWorld(const ShaderData::WorldBuffer& data)
{
	auto world_location = glGetUniformBlockIndex(GetShaderId(), "cWorld");
	glUniformBlockBinding(GetShaderId(), world_location, 0);

	unsigned int uboMatrices;
	glGenBuffers(1, &uboMatrices);

	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(data), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, sizeof(data));

	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(data), &data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void GlShader::UpdateLights(const ShaderData::LightBuffer& data)
{
	auto gDirectionLight = glGetUniformLocation(GetShaderId(), "gDirectionLight");
	DirectX::XMFLOAT4 direction_light = data.mDirectionalLight.mDirection;
	glUniform4fv(gDirectionLight, 1, reinterpret_cast<float*>(&direction_light));

	auto gDiffuseLight = glGetUniformLocation(GetShaderId(), "gDiffuseLight");
	DirectX::XMFLOAT4 diffuse_light = data.mDirectionalLight.mDiffuse;
	glUniform4fv(gDiffuseLight, 1, reinterpret_cast<float*>(&diffuse_light));

	auto gAmbientLightLoc = glGetUniformLocation(GetShaderId(), "gAmbientLight");
	DirectX::XMFLOAT4 ambient_light = data.mDirectionalLight.mAmbient;
	glUniform4fv(gAmbientLightLoc, 1, reinterpret_cast<float*>(&ambient_light));

	auto gSpecularLight = glGetUniformLocation(GetShaderId(), "gSpecularLight");
	DirectX::XMFLOAT4 specular_light = data.mDirectionalLight.mSpecular;
	glUniform4fv(gSpecularLight, 1, reinterpret_cast<float*>(&specular_light));

	auto gCameraPos = glGetUniformLocation(GetShaderId(), "gCameraPos");
	auto cameraPos = data.mDirectionalLight.mCameraPos;
	glUniform4fv(gCameraPos, 1, reinterpret_cast<float*>(&cameraPos));
}

void GlShader::UpdateBones(const ShaderData::BoneBuffer& data)
{

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
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", log, nullptr);
		delete[] log;
#endif

		return false;
	}

	return true;
}
