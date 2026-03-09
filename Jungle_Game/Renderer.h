#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

class URenderer
{
public:
	URenderer();
	~URenderer();

public:
	bool Create(HWND hWnd);
	void Release();

private:
	bool CreateDeviceAndSwapChain(HWND hWnd);
	void ReleaseDeviceAndSwapChain();

	bool CreateFrameBuffer();
	void ReleaseFrameBuffer();

	bool CreateRasterizerState();
	void ReleaseRasterzerState();

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;

	IDXGISwapChain* SwapChain = nullptr;

	ID3D11Texture2D* FrameBuffer = nullptr;
	ID3D11RenderTargetView* RenderTargetView = nullptr;

	ID3D11RasterizerState* RasterizerState = nullptr;

	D3D11_VIEWPORT Viewport = {};
	FLOAT ClearColor[4] = { 0.05f, 0.05f, 0.05f, 1.0f };
};

