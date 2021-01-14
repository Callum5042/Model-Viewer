#include "Pch.h"
#include "Renderer.h"
#include <DirectXColors.h>
#include "Model.h"
#include "LoadTextureDDS.h" // OpenGL built with care
#include "DDSTextureLoader.h" // DirectX from microsoft

HWND DX::GetHwnd(Window* window)
{
	SDL_SysWMinfo wmInfo = {};
	SDL_GetVersion(&wmInfo.version);
	SDL_GetWindowWMInfo(window->GetSdlWindow(), &wmInfo);
	return wmInfo.info.win.window;
}

DXRenderer::~DXRenderer()
{
#ifdef _DEBUG
	ComPtr<ID3D11Debug> debug = nullptr;
	m_Device.As(&debug);
	debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
#endif
}

bool DXRenderer::Create(Window* window)
{
	auto width = window->GetWidth();
	auto height = window->GetHeight();

	if (!CreateDevice())
		return false;

	if (!CreateSwapChain(window, width, height))
		return false;

	if (!CreateRenderTargetAndDepthStencilView(width, height))
		return false;

	SetViewport(width, height);
	QueryHardwareInfo();

	CreateRasterStateSolid();
	CreateRasterStateWireframe();
	m_DeviceContext->RSSetState(m_RasterStateSolid.Get());

	// Create an MSAA render target.
	auto max_sample_count = 8;
	for (int i = max_sample_count; i >= 2; i /= 2)
	{
		auto quality_levels = 0u;
		DX::Check(m_Device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, i, &quality_levels));
		if (quality_levels != 0)
		{
			if (m_MaxMsaaLevel == 0) 
			{
				m_MaxMsaaLevel = i;
			}

			m_SupportMsaaLevels.push_back(i);
		}
	}

	// Create shader sampler
	D3D11_SAMPLER_DESC comparisonSamplerDesc;
	ZeroMemory(&comparisonSamplerDesc, sizeof(D3D11_SAMPLER_DESC));
	comparisonSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.BorderColor[0] = 0.0f;
	comparisonSamplerDesc.BorderColor[1] = 0.0f;
	comparisonSamplerDesc.BorderColor[2] = 0.0f;
	comparisonSamplerDesc.BorderColor[3] = 0.0f;
	comparisonSamplerDesc.MinLOD = 0.f;
	comparisonSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	comparisonSamplerDesc.MipLODBias = 0.0f;
	comparisonSamplerDesc.MaxAnisotropy = 0;
	comparisonSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	comparisonSamplerDesc.Filter = D3D11_FILTER_COMPARISON_ANISOTROPIC;

	DX::Check(m_Device->CreateSamplerState(&comparisonSamplerDesc, &m_ShadowSampler));

	return true;
}

void DXRenderer::Resize(int width, int height)
{
	m_RenderTarget.ReleaseAndGetAddressOf();
	m_DepthStencilView.ReleaseAndGetAddressOf();
	m_RenderTargetView.ReleaseAndGetAddressOf();

	m_MsaaRenderTarget.ReleaseAndGetAddressOf();
	m_MsaaDepthStencilView.ReleaseAndGetAddressOf();
	m_MsaaRenderTargetView.ReleaseAndGetAddressOf();

	DX::Check(m_SwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
	CreateRenderTargetAndDepthStencilView(width, height);
	CreateAntiAliasingTarget(m_CurrentMsaaLevel, width, height);
	SetViewport(width, height);
}

void DXRenderer::Clear()
{
	if (m_UseMsaa)
	{
		m_DeviceContext->ClearRenderTargetView(m_MsaaRenderTargetView.Get(), reinterpret_cast<const float*>(&DirectX::Colors::SteelBlue));
		m_DeviceContext->ClearDepthStencilView(m_MsaaDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
		m_DeviceContext->OMSetRenderTargets(1, m_MsaaRenderTargetView.GetAddressOf(), m_MsaaDepthStencilView.Get());
	}
	else
	{
		m_DeviceContext->ClearRenderTargetView(m_RenderTargetView.Get(), reinterpret_cast<const float*>(&DirectX::Colors::SteelBlue));
		m_DeviceContext->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
		m_DeviceContext->OMSetRenderTargets(1, m_RenderTargetView.GetAddressOf(), m_DepthStencilView.Get());
	}

	m_DeviceContext->PSSetSamplers(0, 1, m_AnisotropicSampler.GetAddressOf());
	m_DeviceContext->PSSetSamplers(1, 1, m_ShadowSampler.GetAddressOf());
}

void DXRenderer::Present()
{
	if (m_UseMsaa)
	{
		m_DeviceContext->ResolveSubresource(m_RenderTarget.Get(), 0, m_MsaaRenderTarget.Get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
	}

	ComPtr<IDXGISwapChain1> swapChain1 = nullptr;
	m_SwapChain.As(&swapChain1);

	if (swapChain1 != nullptr)
	{
		DXGI_PRESENT_PARAMETERS presentParameters = {};
		DX::Check(swapChain1->Present1(static_cast<int>(m_Vsync), 0, &presentParameters));
	}
	else
	{
		DX::Check(m_SwapChain->Present(static_cast<int>(m_Vsync), 0));
	}
}

void DXRenderer::DrawIndex(UINT total_indices, UINT start_index, UINT base_vertex)
{
	m_DeviceContext->DrawIndexed(total_indices, start_index, base_vertex);
}

std::unique_ptr<VertexBuffer> DXRenderer::CreateVertexBuffer(const std::vector<Vertex>& vertices)
{
	auto vertex_buffer = std::make_unique<DXVertexBuffer>();

	D3D11_BUFFER_DESC vertexbuffer_desc = {};
	vertexbuffer_desc.Usage = D3D11_USAGE_DEFAULT;
	vertexbuffer_desc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * vertices.size());
	vertexbuffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexbuffer_data = {};
	vertexbuffer_data.pSysMem = vertices.data();
	DX::Check(m_Device->CreateBuffer(&vertexbuffer_desc, &vertexbuffer_data, vertex_buffer->buffer.ReleaseAndGetAddressOf()));

	return std::move(vertex_buffer);
}

void DXRenderer::ApplyVertexBuffer(VertexBuffer* buffer)
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	auto vertex_buffer = reinterpret_cast<DXVertexBuffer*>(buffer);
	m_DeviceContext->IASetVertexBuffers(0, 1, vertex_buffer->buffer.GetAddressOf(), &stride, &offset);
}

std::unique_ptr<IndexBuffer> DXRenderer::CreateIndexBuffer(const std::vector<UINT>& indices)
{
	auto index_buffer = std::make_unique<DXIndexBuffer>();

	D3D11_BUFFER_DESC ibd = {};
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.ByteWidth = static_cast<UINT>(sizeof(UINT) * indices.size());
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA iInitData = {};
	iInitData.pSysMem = indices.data();

	DX::Check(m_Device->CreateBuffer(&ibd, &iInitData, index_buffer->buffer.ReleaseAndGetAddressOf()));

	return std::move(index_buffer);
}

void DXRenderer::ApplyIndexBuffer(IndexBuffer* index_buffer)
{
	auto buffer = reinterpret_cast<DXIndexBuffer*>(index_buffer);
	m_DeviceContext->IASetIndexBuffer(buffer->buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
}

void DXRenderer::SetPrimitiveTopology()
{
	m_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

std::unique_ptr<Texture2D> DXRenderer::CreateTexture2D(const std::string& path)
{
	auto texture = std::make_unique<DXTexture2D>();

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide_path = converter.from_bytes(path);

	ComPtr<ID3D11Resource> resource = nullptr;
	DX::Check(DirectX::CreateDDSTextureFromFile(m_Device.Get(), wide_path.c_str(), resource.ReleaseAndGetAddressOf(), texture->resource.ReleaseAndGetAddressOf()));

	return std::move(texture);
}

void DXRenderer::ApplyTexture2D(UINT slot, Texture2D* resource)
{
	auto res = reinterpret_cast<DXTexture2D*>(resource);
	m_DeviceContext->PSSetShaderResources(slot, 1, res->resource.GetAddressOf());
}

void DXRenderer::ToggleWireframe(bool wireframe)
{
	if (wireframe)
	{
		m_DeviceContext->RSSetState(m_RasterStateWireframe.Get());
	}
	else
	{
		m_DeviceContext->RSSetState(m_RasterStateSolid.Get());
	}
}

void DXRenderer::QueryHardwareInfo()
{
	std::vector<ComPtr<IDXGIAdapter>> adapters;
	ComPtr<IDXGIAdapter> adapter = nullptr;

	auto i = 0;
	while (m_DxgiFactory2->EnumAdapters(i++, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		adapters.push_back(adapter);
	}

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	for (size_t i = 0; i < adapters.size(); i++)
	{
		DXGI_ADAPTER_DESC adapterDescription;
		adapters[i]->GetDesc(&adapterDescription);

		// Check for Microsoft Basic Render Driver
		if (adapterDescription.VendorId != 0x1414 && adapterDescription.DeviceId != 0x8c)
		{
			m_DeviceName = converter.to_bytes(adapterDescription.Description);
			m_DeviceVideoMemoryMb = adapterDescription.DedicatedVideoMemory;

			/*int outputcount = 0;
			ComPtr<IDXGIOutput> output = nullptr;
			while (adapters[i]->EnumOutputs(outputcount++, &output) != DXGI_ERROR_NOT_FOUND)
			{
				DXGI_OUTPUT_DESC desc;
				output->GetDesc(&desc);
				result += converter.to_bytes(desc.DeviceName) + '\n';

				UINT numModes = 0;
				DX::ThrowIfFailed(output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL));

				DXGI_MODE_DESC* modes = new DXGI_MODE_DESC[numModes];
				DX::ThrowIfFailed(output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, modes));

				for (size_t j = 0; j < numModes; j++)
				{
					result += "Width: " + std::to_string(modes[j].Width) +
						"\t- Height: " + std::to_string(modes[j].Height) +
						"\t- Refresh Rate: " + std::to_string(modes[j].RefreshRate.Numerator / modes[j].RefreshRate.Denominator) +
						"\t - Format: " + std::to_string(modes[j].Format) + '\n';
				}

				delete[] modes;
			}*/
		}
	}
}

bool DXRenderer::CreateDevice()
{
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	UINT numFeatureLevels = ARRAYSIZE(featureLevels);
	D3D_FEATURE_LEVEL featureLevel;

	D3D11_CREATE_DEVICE_FLAG deviceFlag = (D3D11_CREATE_DEVICE_FLAG)0;
#ifdef _DEBUG
	deviceFlag = D3D11_CREATE_DEVICE_DEBUG;
#endif

	DX::Check(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, deviceFlag, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, m_Device.ReleaseAndGetAddressOf(), &featureLevel, m_DeviceContext.ReleaseAndGetAddressOf()));

	return true;
}

bool DXRenderer::CreateSwapChain(Window* window, int width, int height)
{
	auto hwnd = DX::GetHwnd(window);

	ComPtr<IDXGIDevice> dxgiDevice = nullptr;
	DX::Check(m_Device.As(&dxgiDevice));

	ComPtr<IDXGIAdapter> adapter = nullptr;
	DX::Check(dxgiDevice->GetAdapter(adapter.GetAddressOf()));

	DX::Check(adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(m_DxgiFactory1.GetAddressOf())));
	DX::Check(m_DxgiFactory1.As(&m_DxgiFactory2));

	if (m_DxgiFactory2 != nullptr)
	{
		// DirectX 11.1
		DXGI_SWAP_CHAIN_DESC1 sd = {};
		sd.Width = width;
		sd.Height = height;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 2;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsd = {};
		fsd.Windowed = window->GetWindowMode() != WindowMode::FULLSCREEN;

		ComPtr<IDXGISwapChain1> swapChain1 = nullptr;
		DX::Check(m_DxgiFactory2->CreateSwapChainForHwnd(m_Device.Get(), hwnd, &sd, &fsd, nullptr, &swapChain1));
		DX::Check(swapChain1.As(&m_SwapChain));
	}
	else
	{
		// DirectX 11
		DXGI_SWAP_CHAIN_DESC sd = {};
		sd.BufferCount = 2;
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = hwnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = window->GetWindowMode() != WindowMode::FULLSCREEN;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		DX::Check(m_DxgiFactory1->CreateSwapChain(m_Device.Get(), &sd, &m_SwapChain));
	}

	return true;
}

bool DXRenderer::CreateRenderTargetAndDepthStencilView(int width, int height)
{
	// Render target view
	DX::Check(m_SwapChain->GetBuffer(0, __uuidof(ID3D11Resource), reinterpret_cast<void**>(m_RenderTarget.GetAddressOf())));
	DX::Check(m_Device->CreateRenderTargetView(m_RenderTarget.Get(), nullptr, m_RenderTargetView.GetAddressOf()));

	// Depth stencil view
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ComPtr<ID3D11Texture2D> depthStencil = nullptr;
	DX::Check(m_Device->CreateTexture2D(&descDepth, nullptr, &depthStencil));
	DX::Check(m_Device->CreateDepthStencilView(depthStencil.Get(), nullptr, m_DepthStencilView.GetAddressOf()));

	m_DeviceContext->OMSetRenderTargets(1, m_RenderTargetView.GetAddressOf(), m_DepthStencilView.Get());
	return true;
}

void DXRenderer::SetViewport(int width, int height)
{
	m_Viewport.Width = static_cast<float>(width);
	m_Viewport.Height = static_cast<float>(height);
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;
	m_Viewport.TopLeftX = 0;
	m_Viewport.TopLeftY = 0;

	m_DeviceContext->RSSetViewports(1, &m_Viewport);
}

bool DXRenderer::CreateAntiAliasingTarget(int msaa_level, int window_width, int window_height)
{
	m_CurrentMsaaLevel = msaa_level;
	if (msaa_level == 0)
	{
		m_UseMsaa = false;
		return true;
	}
	else
	{
		m_UseMsaa = true;
	}

	// Render target view
	CD3D11_TEXTURE2D_DESC renderTargetDesc(DXGI_FORMAT_R8G8B8A8_UNORM, window_width, window_height, 1, 1,
		D3D11_BIND_RENDER_TARGET, D3D11_USAGE_DEFAULT, 0, m_CurrentMsaaLevel);

	DX::Check(m_Device->CreateTexture2D(&renderTargetDesc, nullptr, m_MsaaRenderTarget.ReleaseAndGetAddressOf()));

	CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(D3D11_RTV_DIMENSION_TEXTURE2DMS, DXGI_FORMAT_R8G8B8A8_UNORM);
	DX::Check(m_Device->CreateRenderTargetView(m_MsaaRenderTarget.Get(), &renderTargetViewDesc, m_MsaaRenderTargetView.ReleaseAndGetAddressOf()));

	// Depth stencil view
	CD3D11_TEXTURE2D_DESC depthStencilDesc(DXGI_FORMAT_D32_FLOAT, window_width, window_height, 1, 1,
		D3D11_BIND_DEPTH_STENCIL, D3D11_USAGE_DEFAULT, 0, m_CurrentMsaaLevel);

	ComPtr<ID3D11Texture2D> depthStencil;
	DX::Check(m_Device->CreateTexture2D(&depthStencilDesc, nullptr, depthStencil.GetAddressOf()));
	DX::Check(m_Device->CreateDepthStencilView(depthStencil.Get(), nullptr, m_MsaaDepthStencilView.ReleaseAndGetAddressOf()));

	return true;
}

int DXRenderer::GetMaxAnisotropicFilterLevel()
{
	return D3D11_REQ_MAXANISOTROPY;
}

void DXRenderer::SetAnisotropicFilter(int level)
{
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = level;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = 1000.0f;

	DX::Check(m_Device->CreateSamplerState(&samplerDesc, &m_AnisotropicSampler));
}

void DXRenderer::SetVync(bool enable)
{
	m_Vsync = enable;
}

void DXRenderer::CreateRasterStateSolid()
{
	D3D11_RASTERIZER_DESC rasterizerState = {};
	rasterizerState.AntialiasedLineEnable = true;
	rasterizerState.CullMode = D3D11_CULL_FRONT;
	rasterizerState.FillMode = D3D11_FILL_SOLID;
	rasterizerState.DepthClipEnable = true;
	rasterizerState.FrontCounterClockwise = true;
	rasterizerState.MultisampleEnable = true;

	rasterizerState.DepthBias = 0;
	rasterizerState.DepthBiasClamp = 1.0f;
	rasterizerState.SlopeScaledDepthBias = 1.0f;

	DX::Check(m_Device->CreateRasterizerState(&rasterizerState, m_RasterStateSolid.ReleaseAndGetAddressOf()));
}

void DXRenderer::CreateRasterStateWireframe()
{
	D3D11_RASTERIZER_DESC rasterizerState = {};
	rasterizerState.AntialiasedLineEnable = true;
	rasterizerState.CullMode = D3D11_CULL_NONE;
	rasterizerState.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerState.DepthClipEnable = true;
	rasterizerState.FrontCounterClockwise = true;
	rasterizerState.MultisampleEnable = true;

	rasterizerState.DepthBias = 0;
	rasterizerState.DepthBiasClamp = 1.0f;
	rasterizerState.SlopeScaledDepthBias = 1.0f;

	DX::Check(m_Device->CreateRasterizerState(&rasterizerState, m_RasterStateWireframe.ReleaseAndGetAddressOf()));
}

GLRenderer::~GLRenderer()
{
	glDeleteSamplers(1, &m_TextureSampler);
	glDeleteTextures(1, &m_BackBuffer);
	glDeleteBuffers(1, &m_FrameBuffer);
	glDeleteRenderbuffers(1, &m_DepthBuffer);
}

bool GLRenderer::Create(Window* window)
{
	m_Window = window;

	auto window_width = window->GetWidth();
	auto window_height = window->GetHeight();

	// Glew
	GLenum error = glewInit();
	if (error != GLEW_OK)
	{
		auto glew_error = reinterpret_cast<const char*>(glewGetErrorString(error));
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", glew_error, nullptr);
		return false;
	}

	glEnable(GL_DEBUG_OUTPUT);

	QueryHardwareInfo();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// MSAA
	auto maxSamples = 0;
	glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
	for (auto i = maxSamples; i >= 2; i /= 2)
	{
		m_SupportMsaaLevels.push_back(i);
	}

	m_MaxMsaaLevel = maxSamples;

	return true;
}

void GLRenderer::Resize(int width, int height)
{
	CreateAntiAliasingTarget(m_CurrentMsaaLevel, width, height);
	glViewport(0, 0, width, height);
}

void GLRenderer::Clear()
{
	static const GLfloat blue[] = { 0.274509817f, 0.509803951f, 0.705882370f, 1.000000000f };
	static GLfloat depth = 1.0f;

	glClearBufferfv(GL_COLOR, 0, blue);
	glClearBufferfv(GL_DEPTH, 0, &depth);

	if (m_UseMsaa)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
		glClearBufferfv(GL_COLOR, 0, blue);
		glClearBufferfv(GL_DEPTH, 0, &depth);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	glBindSampler(0, m_TextureSampler);
	glBindSampler(1, m_TextureSampler);
}

void GLRenderer::Present()
{
	if (m_UseMsaa)
	{
		auto window_width = m_Window->GetWidth();
		auto window_height = m_Window->GetHeight();

		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FrameBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, window_width, window_height, 0, 0, window_width, window_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	SDL_GL_SetSwapInterval(static_cast<int>(m_Vsync));
	SDL_GL_SwapWindow(m_Window->GetSdlWindow());
}

void GLRenderer::DrawIndex(UINT total_indices, UINT start_index, UINT base_vertex)
{
	glDrawElementsBaseVertex(m_PrimitiveTopology, total_indices, GL_UNSIGNED_INT, nullptr, base_vertex);
}

std::unique_ptr<VertexBuffer> GLRenderer::CreateVertexBuffer(const std::vector<Vertex>& vertices)
{
	auto vertex_buffer = std::make_unique<GLVertexBuffer>();

	// Vertex array object
	glCreateVertexArrays(1, &vertex_buffer->vertexArrayObject);
	glBindVertexArray(vertex_buffer->vertexArrayObject);

	// Create vertex buffer
	glCreateBuffers(1, &vertex_buffer->buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer->buffer);

	glNamedBufferStorage(vertex_buffer->buffer, sizeof(Vertex) * vertices.size(), nullptr, GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferSubData(vertex_buffer->buffer, 0, sizeof(Vertex) * vertices.size(), vertices.data());

	return std::move(vertex_buffer);
}

void GLRenderer::ApplyVertexBuffer(VertexBuffer* vertex_buffer)
{
	auto buffer = reinterpret_cast<GLVertexBuffer*>(vertex_buffer);
	glBindVertexArray(buffer->vertexArrayObject);
}

std::unique_ptr<IndexBuffer> GLRenderer::CreateIndexBuffer(const std::vector<UINT>& indices)
{
	auto buffer = std::make_unique<GLIndexBuffer>();

	glCreateBuffers(1, &buffer->buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);

	return std::move(buffer);
}

void GLRenderer::ApplyIndexBuffer(IndexBuffer* index_buffer)
{
	// OpenGL doesn't require us to specific bind the index buffer, since it will bind it to the currently bound vertex array object
}

void GLRenderer::SetPrimitiveTopology()
{
	// Remember primitive topology as it's part of the OpenGL draw function
	m_PrimitiveTopology = GL_TRIANGLES;
}

std::unique_ptr<Texture2D> GLRenderer::CreateTexture2D(const std::string& path)
{
	auto resource = std::make_unique<GLTexture2D>();

	Rove::LoadDDS dds;
	std::string normal_texture_path = path;
	dds.Load(std::move(normal_texture_path));

	glCreateTextures(GL_TEXTURE_2D, 1, &resource->resource);
	glTextureStorage2D(resource->resource, dds.MipmapCount(), dds.Format(), dds.Width(), dds.Height());

	for (auto& mipmap : dds.mipmaps)
	{
		glCompressedTextureSubImage2D(resource->resource, mipmap.level, 0, 0, mipmap.width, mipmap.height, dds.Format(), mipmap.texture_size, mipmap.data);
	}

	return std::move(resource);
}

void GLRenderer::ApplyTexture2D(UINT slot, Texture2D* resource)
{
	auto res = reinterpret_cast<GLTexture2D*>(resource);
	glBindTextureUnit(slot, res->resource);
}

void GLRenderer::ToggleWireframe(bool wireframe)
{
	if (wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

void GLRenderer::QueryHardwareInfo()
{
	std::string vendor = (char*)glGetString(GL_VENDOR);
	m_DeviceName = (char*)glGetString(GL_RENDERER);

	if (vendor == "NVIDIA Corporation")
	{
		GLint videoMemoryKb = 0;
		glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &videoMemoryKb);
		m_DeviceVideoMemoryMb = static_cast<SIZE_T>(videoMemoryKb) * 1024;
	}
	else if (vendor == "ATI Technologies Inc.")
	{
		auto n = wglGetGPUIDsAMD(0, 0);
		auto ids = std::make_unique<UINT[]>(n);

		wglGetGPUIDsAMD(n, ids.get());
		wglGetGPUInfoAMD(ids[0], WGL_GPU_RAM_AMD, GL_UNSIGNED_INT, sizeof(size_t), &m_DeviceVideoMemoryMb);
	}
}

bool GLRenderer::CreateAntiAliasingTarget(int msaa_level, int window_width, int window_height)
{
	glDeleteRenderbuffers(1, &m_DepthBuffer);
	glDeleteTextures(1, &m_BackBuffer);
	glDeleteFramebuffers(1, &m_FrameBuffer);

	m_CurrentMsaaLevel = msaa_level;
	if (msaa_level == 0)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		m_UseMsaa = false;
		return true;
	}
	else
	{
		m_UseMsaa = true;
	}

	GLenum err = 0;

	// Back buffer
	glGenTextures(1, &m_BackBuffer);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_BackBuffer);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, msaa_level, GL_RGB, window_width, window_height, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		std::cout << "Error: " << err;
	}

	// Stencil buffer
	glGenRenderbuffers(1, &m_DepthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBuffer);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa_level, GL_DEPTH24_STENCIL8, window_width, window_height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		std::cout << "Error\n";
	}

	// Frame buffer
	glGenFramebuffers(1, &m_FrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_BackBuffer, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthBuffer);

	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		std::cout << "Error\n";
	}

	auto t = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (t != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "Big bug\n";
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return true;
}

int GLRenderer::GetMaxAnisotropicFilterLevel()
{
	auto max_anisotrophic = 0;
	glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_anisotrophic);
	return max_anisotrophic;
}

void GLRenderer::SetAnisotropicFilter(int level)
{
	if (level == 0)
	{
		glDeleteSamplers(1, &m_TextureSampler);
		return;
	}

	glCreateSamplers(1, &m_TextureSampler);

	glSamplerParameterf(m_TextureSampler, GL_TEXTURE_MAX_ANISOTROPY, static_cast<GLfloat>(level));
	glSamplerParameterf(m_TextureSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glSamplerParameterf(m_TextureSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void GLRenderer::SetVync(bool enable)
{
	m_Vsync = enable;
}
