#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <unordered_map>
#include <string>
#include "Math.h"

#include "stb_image.h"
#include "stb_truetype.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

enum class ETextAlign
{
    Left,
    Center,
    Right
};

struct FConstantBuffer
{
    FVector position;
    float padding1 = 0.0f;
    FVector rotation;
    float padding2 = 0.0f;
    FVector scale;
    float padding3 = 0.0f;
};

struct FSpriteUVBuffer
{
    float UVOffsetX;
    float UVOffsetY;
    float UVScaleX;
    float UVScaleY;
};

struct FEffectConstantBuffer
{
    float time;
    float padding[3];
};

struct FVertex
{
    FVector position;
	float u, v;
};

struct FTextVertex
{
    FVector position;
    float u, v;
    FVector color;
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
	
	void SwapBuffer();
	void UpdateConstantBuffer(const FConstantBuffer& data);
    void UpdateSpriteUV(FSpriteUVBuffer& data);
    void UpdateEffectConstantBuffer(const FEffectConstantBuffer& data);

    void CreateTexture(const std::string& filePath, const std::string& name);
    void ReleaseTexture(const std::string& filePath);
    void BindTexture(const std::string& name);

    void GetQuad(wchar_t c, float* x, float* y, stbtt_aligned_quad* q);
    void DrawString(const std::wstring& name, float x, float y, const FVector& color = FVector(1.0f, 1.0f, 1.0f), ETextAlign align = ETextAlign::Center);

	ID3D11Device* GetDevice();
	ID3D11DeviceContext* GetDeviceContext();

private:
	bool CreateDeviceAndSwapChain(HWND hWnd);
	void ReleaseDeviceAndSwapChain();

	bool CreateFrameBuffer();
	void ReleaseFrameBuffer();

	bool CreateRasterizerState();
	void ReleaseRasterzerState();

	void CreateConstantBuffer();
    void ReleaseConstantBuffer();

    void CreateEffectConstantBuffer();
    void ReleaseEffectConstantBuffer();

    void CreateSpriteConstantBuffer();
    void ReleaseSpriteConstantBuffer();

	void CreateDefaultQuad();
	void ReleaseDefaultQuad();

	void CreateDefaultShader();
	void ReleaseDefaultShader();

	void CreateDefaultSamplerState();
	void ReleaseDefaultSamplerState();

	void CreateBlendState();
	void ReleaseBlendState();

    void CreateDefaultFontAtlasAndVertexBuffer();
    void ReleaseDefaultFontAtlasAndVertexBuffer();

    void CreateTextShader();
    void ReleaseTextShader();

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;

	IDXGISwapChain* SwapChain = nullptr;

	ID3D11Texture2D* FrameBuffer = nullptr;
	ID3D11RenderTargetView* RenderTargetView = nullptr;

	ID3D11RasterizerState* RasterizerState = nullptr;

	D3D11_VIEWPORT Viewport = {};
	FLOAT ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    ID3D11Buffer* ConstantBuffer = nullptr;
    ID3D11Buffer* EffectConstantBuffer = nullptr;
    ID3D11Buffer* SpriteConstantBuffer = nullptr;

    std::unordered_map<std::string, ID3D11Texture2D*> Textures;
    std::unordered_map<std::string, ID3D11ShaderResourceView*> SRVs;

	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;

	ID3D11VertexShader* VertexShader = nullptr;
	ID3D11PixelShader* PixelShader = nullptr;
	ID3D11InputLayout* InputLayout = nullptr;

	ID3D11SamplerState* DefaultSamplerState = nullptr;

	ID3D11BlendState* BlendState = nullptr;

    stbtt_packedchar FontAtlas[96];
    stbtt_packedchar FontHangulAtlas[12];
    ID3D11ShaderResourceView* FontAtlasSRV = nullptr;

    ID3D11Buffer* TextVertexBuffer = nullptr;

    ID3D11VertexShader* TextVertexShader = nullptr;
    ID3D11PixelShader* TextPixelShader = nullptr;
    ID3D11InputLayout* TextInputLayout = nullptr;
};

