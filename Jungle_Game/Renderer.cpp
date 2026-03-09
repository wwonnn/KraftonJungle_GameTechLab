#include "Renderer.h"

URenderer::URenderer()
{
}

URenderer::~URenderer()
{
}

bool URenderer::Create(HWND hWnd)
{
	if (!CreateDeviceAndSwapChain(hWnd))
		return false;
	if (!CreateFrameBuffer())
		return false;
	if (!CreateRasterizerState())
		return false;
	return true;
}

void URenderer::Release()
{
	ReleaseRasterzerState();
	ReleaseFrameBuffer();
	ReleaseDeviceAndSwapChain();
}

bool URenderer::CreateDeviceAndSwapChain(HWND hWnd)
{
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = 0;
	swapChainDesc.BufferDesc.Height = 0;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = TRUE;

	D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
		featureLevels,
		1,
		D3D11_SDK_VERSION,
		&swapChainDesc,
		&SwapChain,
		&Device,
		nullptr,
		&DeviceContext
	);

	return true;
}

void URenderer::ReleaseDeviceAndSwapChain()
{
	if (DeviceContext)
	{
		DeviceContext->Flush();
	}

	if (SwapChain)
	{
		SwapChain->Release();
		SwapChain = nullptr;
	}

	if (Device)
	{
		Device->Release();
		Device = nullptr;
	}

	if (DeviceContext)
	{
		DeviceContext->Release();
		DeviceContext = nullptr;
	}
}

bool URenderer::CreateFrameBuffer()
{
	SwapChain->GetBuffer(0, IID_PPV_ARGS(&FrameBuffer));
	Device->CreateRenderTargetView(FrameBuffer, nullptr, &RenderTargetView);
	return true;
}

void URenderer::ReleaseFrameBuffer()
{
	if (FrameBuffer)
	{
		FrameBuffer->Release();
		FrameBuffer = nullptr;
	}
}

bool URenderer::CreateRasterizerState()
{
	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;

	Device->CreateRasterizerState(&rasterizerDesc, &RasterizerState);

	return true;
}

void URenderer::ReleaseRasterzerState()
{
	if (RasterizerState)
	{
		RasterizerState->Release();
		RasterizerState = nullptr;
	}
}