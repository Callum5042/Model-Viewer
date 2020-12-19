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

	virtual void Clear() = 0;
	virtual void Present() = 0;

	virtual std::string GetDescription() = 0;

	virtual RenderAPI GetRenderAPI() = 0;
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

	constexpr ComPtr<ID3D11Device>& GetDevice() { return m_Device; }
	constexpr ComPtr<ID3D11DeviceContext>& GetDeviceContext() { return m_DeviceContext; }

	std::string GetDescription() override;

	RenderAPI GetRenderAPI() { return RenderAPI::DIRECTX; }

private:
	ComPtr<ID3D11Device> m_Device = nullptr;
	ComPtr<ID3D11DeviceContext> m_DeviceContext = nullptr;
	ComPtr<IDXGISwapChain> m_SwapChain = nullptr;
	ComPtr<IDXGIFactory1> m_DxgiFactory1 = nullptr;
	ComPtr<IDXGIFactory2> m_DxgiFactory2 = nullptr;

	ComPtr<ID3D11RenderTargetView> m_RenderTargetView = nullptr;
	ComPtr<ID3D11DepthStencilView> m_DepthStencilView = nullptr;
	D3D11_VIEWPORT m_Viewport = {};

	bool CreateDevice();
	bool CreateSwapChain(Window* window, int width, int height);
	bool CreateRenderTargetAndDepthStencilView(int width, int height);
	void SetViewport(int width, int height);

	// Raster states
	ComPtr<ID3D11RasterizerState> m_RasterStateSolid = nullptr;
	void CreateRasterStateSolid();
};

class GlRenderer : public IRenderer
{
public:
	GlRenderer() = default;
	virtual ~GlRenderer() = default;

	bool Create(Window* window) override;
	void Resize(int width, int height) override;

	void Clear() override;
	void Present() override;

	std::string GetDescription() override;

	RenderAPI GetRenderAPI() { return RenderAPI::OPENGL; }

private:
	Window* m_Window = nullptr;
};