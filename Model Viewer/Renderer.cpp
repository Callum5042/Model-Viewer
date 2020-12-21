#include "Pch.h"
#include "Renderer.h"
#include <DirectXColors.h>

HWND DX::GetHwnd(Window* window)
{
	SDL_SysWMinfo wmInfo = {};
	SDL_GetVersion(&wmInfo.version);
	SDL_GetWindowWMInfo(window->GetSdlWindow(), &wmInfo);
	return wmInfo.info.win.window;
}

DxRenderer::~DxRenderer()
{
#ifdef _DEBUG
	ComPtr<ID3D11Debug> debug = nullptr;
	m_Device.As(&debug);
	debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
#endif
}

bool DxRenderer::Create(Window* window)
{
	int width = window->GetWidth();
	int height = window->GetHeight();

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

	return true;
}

void DxRenderer::Resize(int width, int height)
{
	m_DepthStencilView.ReleaseAndGetAddressOf();
	m_RenderTargetView.ReleaseAndGetAddressOf();

	DX::ThrowIfFailed(m_SwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
	CreateRenderTargetAndDepthStencilView(width, height);
	SetViewport(width, height);
}

void DxRenderer::Clear()
{
	m_DeviceContext->OMSetRenderTargets(1, m_RenderTargetView.GetAddressOf(), NULL);
	m_DeviceContext->ClearRenderTargetView(m_RenderTargetView.Get(), reinterpret_cast<const float*>(&DirectX::Colors::SteelBlue));
	m_DeviceContext->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void DxRenderer::Present()
{
	ComPtr<IDXGISwapChain1> swapChain1 = nullptr;
	m_SwapChain.As(&swapChain1);

	DXGI_PRESENT_PARAMETERS presentParameters = {};
	DX::ThrowIfFailed(swapChain1->Present1(0, 0, &presentParameters));
}

void DxRenderer::ToggleWireframe(bool wireframe)
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

void DxRenderer::QueryHardwareInfo()
{
	std::vector<ComPtr<IDXGIAdapter>> adapters;
	ComPtr<IDXGIAdapter> adapter = nullptr;

	int i = 0;
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

bool DxRenderer::CreateDevice()
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

	DX::ThrowIfFailed(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, deviceFlag, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &m_Device, &featureLevel, &m_DeviceContext));

	return true;
}

bool DxRenderer::CreateSwapChain(Window* window, int width, int height)
{
	HWND hwnd = DX::GetHwnd(window);

	ComPtr<IDXGIDevice> dxgiDevice = nullptr;
	DX::ThrowIfFailed(m_Device.As(&dxgiDevice));

	ComPtr<IDXGIAdapter> adapter = nullptr;
	DX::ThrowIfFailed(dxgiDevice->GetAdapter(adapter.GetAddressOf()));

	DX::ThrowIfFailed(adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(m_DxgiFactory1.GetAddressOf())));
	DX::ThrowIfFailed(m_DxgiFactory1.As(&m_DxgiFactory2));

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
		DX::ThrowIfFailed(m_DxgiFactory2->CreateSwapChainForHwnd(m_Device.Get(), hwnd, &sd, &fsd, nullptr, &swapChain1));
		DX::ThrowIfFailed(swapChain1.As(&m_SwapChain));
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

		DX::ThrowIfFailed(m_DxgiFactory1->CreateSwapChain(m_Device.Get(), &sd, &m_SwapChain));
	}

	return true;
}

bool DxRenderer::CreateRenderTargetAndDepthStencilView(int width, int height)
{
	// Render target view
	ComPtr<ID3D11Resource> backBuffer = nullptr;
	DX::ThrowIfFailed(m_SwapChain->GetBuffer(0, __uuidof(ID3D11Resource), reinterpret_cast<void**>(backBuffer.GetAddressOf())));
	DX::ThrowIfFailed(m_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_RenderTargetView.GetAddressOf()));

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
	DX::ThrowIfFailed(m_Device->CreateTexture2D(&descDepth, nullptr, &depthStencil));
	DX::ThrowIfFailed(m_Device->CreateDepthStencilView(depthStencil.Get(), nullptr, m_DepthStencilView.GetAddressOf()));

	m_DeviceContext->OMSetRenderTargets(1, m_RenderTargetView.GetAddressOf(), m_DepthStencilView.Get());
	return true;
}

void DxRenderer::SetViewport(int width, int height)
{
	m_Viewport.Width = static_cast<float>(width);
	m_Viewport.Height = static_cast<float>(height);
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;
	m_Viewport.TopLeftX = 0;
	m_Viewport.TopLeftY = 0;

	m_DeviceContext->RSSetViewports(1, &m_Viewport);
}

void DxRenderer::CreateRasterStateSolid()
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

	DX::ThrowIfFailed(m_Device->CreateRasterizerState(&rasterizerState, m_RasterStateSolid.ReleaseAndGetAddressOf()));
}

void DxRenderer::CreateRasterStateWireframe()
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

	DX::ThrowIfFailed(m_Device->CreateRasterizerState(&rasterizerState, m_RasterStateWireframe.ReleaseAndGetAddressOf()));
}

bool GlRenderer::Create(Window* window)
{
	// Glew
	GLenum error = glewInit();
	if (error != GLEW_OK)
	{
		//std::cout << "Error: " << glewGetErrorString(error) << '\n';
		return false;
	}

#ifdef _DEBUG
	const unsigned char* glewVersion = glewGetString(GLEW_VERSION);
	//std::cout << "Glew: " << glewVersion << '\n';
#endif

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	m_Window = window;

	QueryHardwareInfo();
	return true;
}

void GlRenderer::Resize(int width, int height)
{
	glViewport(0, 0, width, height);
}

void GlRenderer::Clear()
{
	static const GLfloat blue[] = { 0.274509817f, 0.509803951f, 0.705882370f, 1.000000000f };
	glClearBufferfv(GL_COLOR, 0, blue);

	static GLfloat depth = 1.0f;
	glClearBufferfv(GL_DEPTH, 0, &depth);
}

void GlRenderer::Present()
{
	SDL_GL_SetSwapInterval(0);
	SDL_GL_SwapWindow(m_Window->GetSdlWindow());
}

void GlRenderer::ToggleWireframe(bool wireframe)
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

void GlRenderer::QueryHardwareInfo()
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
