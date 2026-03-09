#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <unordered_map>
#include <string>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

struct FConstantBuffer
{
	float x, y, z;
};

struct FVertex
{
	float x, y, z;
	float u, v;
};

class URenderer
{
public:
	URenderer();
	~URenderer();

public:
	bool Create(HWND hWnd);
	void Release();

	void BeginFrame();
	void Render();
	void EndFrame();
	
	void UpdateConstantBuffer(const FConstantBuffer& data);

private:
	bool CreateDeviceAndSwapChain(HWND hWnd);
	void ReleaseDeviceAndSwapChain();

	bool CreateFrameBuffer();
	void ReleaseFrameBuffer();

	bool CreateRasterizerState();
	void ReleaseRasterzerState();

	void CreateConstantBuffer();
	void ReleaseConstantBuffer();

	void CreateDefaultQuad();
	void ReleaseDefaultQuad();

	void CreateDefaultShader();
	void ReleaseDefaultShader();

	void CreateDefaultTexture();
	void ReleaseDefaultTexture();

	void CreateDefaultSamplerState();
	void ReleaseDefaultSamplerState();

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;

	IDXGISwapChain* SwapChain = nullptr;

	ID3D11Texture2D* FrameBuffer = nullptr;
	ID3D11RenderTargetView* RenderTargetView = nullptr;

	ID3D11RasterizerState* RasterizerState = nullptr;

	D3D11_VIEWPORT Viewport = {};
	FLOAT ClearColor[4] = { 0.05f, 0.05f, 0.05f, 1.0f };

	ID3D11Buffer* ConstantBuffer = nullptr;

	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;

	ID3D11VertexShader* VertexShader = nullptr;
	ID3D11PixelShader* PixelShader = nullptr;
	ID3D11InputLayout* InputLayout = nullptr;

	ID3D11Texture2D* DefaultTexture = nullptr;
	ID3D11ShaderResourceView* DefaultSRV = nullptr;
	ID3D11SamplerState* DefaultSamplerState = nullptr;
};

