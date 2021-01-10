#pragma once

#include "Window.h"

namespace DX
{
	inline void ThrowIfFailed(HRESULT hr)
	{
#ifdef _DEBUG
		if (FAILED(hr))
		{
			throw std::exception();
		}
#endif
	}

	HWND GetHwnd(Window* window);
}

enum class RenderAPI
{
	NONE,
	DIRECTX,
	OPENGL
};

class IRenderer
{
public:
	IRenderer() = default;
	virtual ~IRenderer() = default;

	virtual bool Create(Window* window) = 0;
	virtual void Resize(int width, int height) = 0;

	// Clear buffers
	virtual void Clear() = 0;

	// Present back buffer to screen
	virtual void Present() = 0;

	// Rendering API
	virtual RenderAPI GetRenderAPI() = 0;

	// Query hardware
	virtual const std::string& GetName() = 0;
	virtual SIZE_T GetVRAM() = 0;

	// Toggle wireframe rendering
	virtual void ToggleWireframe(bool wireframe) = 0;

	// Anti-aliasing
	virtual bool CreateAntiAliasingTarget(int msaa_level, int window_width, int window_height) = 0;
	virtual const std::vector<int>& GetSupportMsaaLevels() = 0;
	virtual int GetMaxMsaaLevel() = 0;

	// Texture filtering
	virtual int GetMaxAnisotropicFilterLevel() = 0;
	virtual void SetAnisotropicFilter(int level) = 0;

	// Vsync
	virtual void SetVync(bool enable) = 0;
};

class DxRenderer : public IRenderer
{
public:
	DxRenderer() = default;
	virtual ~DxRenderer();

	bool Create(Window* window);
	void Resize(int width, int height);

	void Clear();
	void Present();

	// Direct3D specific data
	constexpr ComPtr<ID3D11Device>& GetDevice() { return m_Device; }
	constexpr ComPtr<ID3D11DeviceContext>& GetDeviceContext() { return m_DeviceContext; }

	// Rendering API
	RenderAPI GetRenderAPI() { return RenderAPI::DIRECTX; }

	// Hardware information
	const std::string& GetName() override { return m_DeviceName; }
	SIZE_T GetVRAM() override { return m_DeviceVideoMemoryMb; }

	// Wireframe
	void ToggleWireframe(bool wireframe) override;

	// Anti-aliasing
	bool CreateAntiAliasingTarget(int msaa_level, int window_width, int window_height);
	const std::vector<int>& GetSupportMsaaLevels() { return m_SupportMsaaLevels; }
	int GetMaxMsaaLevel() override { return m_MaxMsaaLevel; }

	// Texture filtering
	virtual int GetMaxAnisotropicFilterLevel() override;
	virtual void SetAnisotropicFilter(int level) override;

	// Vsync
	virtual void SetVync(bool enable) override;

private:
	ComPtr<ID3D11Device> m_Device = nullptr;
	ComPtr<ID3D11DeviceContext> m_DeviceContext = nullptr;
	ComPtr<IDXGISwapChain> m_SwapChain = nullptr;
	ComPtr<IDXGIFactory1> m_DxgiFactory1 = nullptr;
	ComPtr<IDXGIFactory2> m_DxgiFactory2 = nullptr;
	D3D11_VIEWPORT m_Viewport = {};

	ComPtr<ID3D11Texture2D> m_RenderTarget = nullptr;
	ComPtr<ID3D11RenderTargetView> m_RenderTargetView = nullptr;
	ComPtr<ID3D11DepthStencilView> m_DepthStencilView = nullptr;

	bool CreateDevice();
	bool CreateSwapChain(Window* window, int width, int height);
	bool CreateRenderTargetAndDepthStencilView(int width, int height);
	void SetViewport(int width, int height);

	// Multi sample anti-aliasing
	bool m_UseMsaa = false;
	int m_CurrentMsaaLevel = 0;
	int m_MaxMsaaLevel = 0;
	std::vector<int> m_SupportMsaaLevels;

	ComPtr<ID3D11Texture2D> m_MsaaRenderTarget = nullptr;
	ComPtr<ID3D11RenderTargetView> m_MsaaRenderTargetView = nullptr;
	ComPtr<ID3D11DepthStencilView> m_MsaaDepthStencilView = nullptr;

	// Raster states
	ComPtr<ID3D11RasterizerState> m_RasterStateSolid = nullptr;
	void CreateRasterStateSolid();

	ComPtr<ID3D11RasterizerState> m_RasterStateWireframe = nullptr;
	void CreateRasterStateWireframe();

	// Query device hardware information
	void QueryHardwareInfo();

	// Device name
	std::string m_DeviceName;

	// Device video memory
	SIZE_T m_DeviceVideoMemoryMb = 0;

	// Texture filtering
	ComPtr<ID3D11SamplerState> m_AnisotropicSampler = nullptr;

	// Shaders
	ComPtr<ID3D11SamplerState> m_ShadowSampler = nullptr;

	// Vsync
	bool m_Vsync = false;
};

class GlRenderer : public IRenderer
{
public:
	GlRenderer() = default;
	virtual ~GlRenderer();

	bool Create(Window* window) override;
	void Resize(int width, int height) override;

	void Clear() override;
	void Present() override;

	RenderAPI GetRenderAPI() { return RenderAPI::OPENGL; }

	const std::string& GetName() override { return m_DeviceName; }
	SIZE_T GetVRAM() override { return m_DeviceVideoMemoryMb; }

	void ToggleWireframe(bool wireframe) override;

	// Anti-aliasing
	const std::vector<int>& GetSupportMsaaLevels() override { return m_SupportMsaaLevels; }
	bool CreateAntiAliasingTarget(int msaa_level, int window_width, int window_height);
	int GetMaxMsaaLevel() override { return m_MaxMsaaLevel; }

	// Texture filtering
	virtual int GetMaxAnisotropicFilterLevel() override;
	virtual void SetAnisotropicFilter(int level) override;

	// Vsync
	virtual void SetVync(bool enable) override;

private:
	Window* m_Window = nullptr;

	GLuint m_FrameBuffer = 0;
	GLuint m_BackBuffer = 0;
	GLuint m_DepthBuffer = 0;

	// MSAA
	bool m_UseMsaa = false;
	int m_CurrentMsaaLevel = 0;
	int m_MaxMsaaLevel = 0;
	std::vector<int> m_SupportMsaaLevels;

	// Query device hardware information
	void QueryHardwareInfo();

	// Device name
	std::string m_DeviceName;

	// Device video memory
	SIZE_T m_DeviceVideoMemoryMb = 0;

	// Texture filtering
	GLuint m_TextureSampler = 0;

	// Vsync
	bool m_Vsync = false;
};