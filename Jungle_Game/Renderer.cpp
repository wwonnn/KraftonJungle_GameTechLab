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

	CreateConstantBuffer();
	CreateDefaultQuad();
	CreateDefaultShader();

	return true;
}

void URenderer::Release()
{
	ReleaseDefaultShader();
	ReleaseDefaultQuad();
	ReleaseConstantBuffer();
	ReleaseRasterzerState();
	ReleaseFrameBuffer();
	ReleaseDeviceAndSwapChain();
}

void URenderer::BeginFrame()
{
	DeviceContext->ClearRenderTargetView(RenderTargetView, ClearColor);

	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	DeviceContext->RSSetViewports(1, &Viewport);
	DeviceContext->RSSetState(RasterizerState);

	DeviceContext->OMSetRenderTargets(1, &RenderTargetView, nullptr);

	DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
}

void URenderer::Render()
{
	BeginFrame();

	FConstantBuffer constants = { 0.0f, 0.0f, 0.0f };
	UpdateConstantBuffer(constants);

	UINT stride = sizeof(FVertex);
	UINT offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &stride, &offset);
	DeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	DeviceContext->IASetInputLayout(InputLayout);
	DeviceContext->VSSetShader(VertexShader, nullptr, 0);
	DeviceContext->PSSetShader(PixelShader, nullptr, 0);
	DeviceContext->DrawIndexed(6, 0, 0);

	EndFrame();
}

void URenderer::EndFrame()
{
	SwapChain->Present(1, 0);
}

void URenderer::UpdateConstantBuffer(const FConstantBuffer& data)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	memcpy(mappedResource.pData, &data, sizeof(FConstantBuffer));
	DeviceContext->Unmap(ConstantBuffer, 0);
}

bool URenderer::CreateDeviceAndSwapChain(HWND hWnd)
{
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferDesc.Width = 0;
	swapChainDesc.BufferDesc.Height = 0;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

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

	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
	renderTargetViewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	Device->CreateRenderTargetView(FrameBuffer, &renderTargetViewDesc, &RenderTargetView);

	D3D11_TEXTURE2D_DESC frameBufferDesc = {};
	FrameBuffer->GetDesc(&frameBufferDesc);
	Viewport = { 0.0f, 0.0f, (float)frameBufferDesc.Width, (float)frameBufferDesc.Height, 0.0f, 1.0f};

	return true;
}

void URenderer::ReleaseFrameBuffer()
{
	if (RenderTargetView)
	{
		RenderTargetView->Release();
		RenderTargetView = nullptr;
	}

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

void URenderer::CreateConstantBuffer()
{
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = sizeof(ConstantBuffer) + 0xf & (~0xf);
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Device->CreateBuffer(&bufferDesc, nullptr, &ConstantBuffer);
}

void URenderer::ReleaseConstantBuffer()
{
	if (ConstantBuffer)
	{
		ConstantBuffer->Release();
		ConstantBuffer = nullptr;
	}
}

void URenderer::CreateDefaultQuad()
{
	FVertex quadVertices[] =
	{
		{ -0.5f, 0.5f, 0.0f, 0.0f, 0.0f },
		{ 0.5f, 0.5f, 0.0f, 1.0f, 0.0f },
		{ 0.5f, -0.5f, 0.0f, 1.0f, 1.0f },
		{ -0.5f, -0.5f, 0.0f, 0.0f, 1.0f }
	};
	
	D3D11_BUFFER_DESC vertexBufferDesc = {};
	vertexBufferDesc.ByteWidth = sizeof(FVertex) * 4;
	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexBufferSRD = { quadVertices };

	Device->CreateBuffer(&vertexBufferDesc, &vertexBufferSRD, &VertexBuffer);

	UINT indices[] = { 0, 1, 2, 0, 2, 3 };

	D3D11_BUFFER_DESC indexBufferDesc = {};
	indexBufferDesc.ByteWidth = sizeof(UINT) * 6;
	indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA indexBufferSRD = { indices };

	Device->CreateBuffer(&indexBufferDesc, &indexBufferSRD, &IndexBuffer);
}

void URenderer::ReleaseDefaultQuad()
{
	if (InputLayout)
	{
		InputLayout->Release();
		InputLayout = nullptr;
	}
	if (VertexBuffer)
	{
		VertexBuffer->Release();
		VertexBuffer = nullptr;
	}
	if (IndexBuffer)
	{
		IndexBuffer->Release();
		IndexBuffer = nullptr;
	}
}

void URenderer::CreateDefaultShader()
{
	std::wstring shaderPath = L"DefaultShader.hlsl";

	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* psBlob = nullptr;

	D3DCompileFromFile(shaderPath.c_str(),
		nullptr,
		nullptr,
		"mainVS",
		"vs_5_0",
		0,
		0,
		&vsBlob,
		nullptr
	);
	Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &VertexShader);

	D3DCompileFromFile(shaderPath.c_str(),
		nullptr,
		nullptr,
		"mainPS",
		"ps_5_0",
		0,
		0,
		&psBlob,
		nullptr
	);
	Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &PixelShader);

	D3D11_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	Device->CreateInputLayout(inputLayout, ARRAYSIZE(inputLayout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &InputLayout);
}

void URenderer::ReleaseDefaultShader()
{
	if (InputLayout)
	{
		InputLayout->Release();
		InputLayout = nullptr;
	}
	if (PixelShader)
	{
		PixelShader->Release();
		PixelShader = nullptr;
	}
	if (VertexShader)
	{
		VertexShader->Release();
		VertexShader = nullptr;
	}
}
