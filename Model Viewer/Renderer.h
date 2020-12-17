#pragma once

#include "Window.h"
#include <d3d11_4.h>
#include <wrl\client.h>
using Microsoft::WRL::ComPtr;

class Renderer
{
public:
	Renderer() = default;
	virtual ~Renderer();

	bool Create(Window* window);
	void Resize(int width, int height);

	void Clear();
	void Present();

private:
	ComPtr<ID3D11Device> m_Device = nullptr;
	ComPtr<ID3D11DeviceContext> m_DeviceContext = nullptr;
	ComPtr<IDXGISwapChain> m_SwapChain = nullptr;

	ComPtr<ID3D11RenderTargetView> m_RenderTargetView = nullptr;
	ComPtr<ID3D11DepthStencilView> m_DepthStencilView = nullptr;
	D3D11_VIEWPORT m_Viewport = {};

	bool CreateDevice();
	bool CreateSwapChain(HWND hwnd, int width, int height);
	bool CreateRenderTargetAndDepthStencilView(int width, int height);
	void SetViewport(int width, int height);
};