#include "Renderer.h"

#include <iostream>
#include <fstream>
#include <vector>

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
    CreateSpriteConstantBuffer();
    CreateDefaultQuad();
    CreateDefaultShader();
    CreateDefaultTexture();
    CreateDefaultSamplerState();
    CreateBlendState();
    CreateDefaultFontAtlasAndVertexBuffer();
    CreateTextShader();

    return true;
}

void URenderer::Release()
{
    ReleaseTextShader();
    ReleaseDefaultFontAtlasAndVertexBuffer();
    ReleaseBlendState();
    ReleaseDefaultSamplerState();
    ReleaseDefaultTexture();
    ReleaseDefaultShader();
    ReleaseDefaultQuad();
    ReleaseConstantBuffer();
    ReleaseSpriteConstantBuffer();
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
    DeviceContext->OMSetBlendState(BlendState, NULL, 0xffffffff);

    DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
}

void URenderer::Render()
{
    //BeginFrame();

/*    FConstantBuffer constants;
    constants.position = FVector(0.0f, 0.0f, 0.0f);
    constants.scale = FVector(0.5f, 0.5f, 1.0f);
    UpdateConstantBuffer(constants);*/

    UINT stride = sizeof(FVertex);
    UINT offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &stride, &offset);
    DeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    DeviceContext->IASetInputLayout(InputLayout);
    DeviceContext->VSSetShader(VertexShader, nullptr, 0);

    DeviceContext->PSSetSamplers(0, 1, &DefaultSamplerState);

    DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    DeviceContext->DrawIndexed(6, 0, 0);

    //EndFrame();
}

void URenderer::EndFrame()
{

}

void URenderer::SwapBuffer() {
    SwapChain->Present(1, 0);
}

void URenderer::UpdateConstantBuffer(const FConstantBuffer& data)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    memcpy(mappedResource.pData, &data, sizeof(FConstantBuffer));
    DeviceContext->Unmap(ConstantBuffer, 0);
}

void URenderer::UpdateSpriteUV(FSpriteUVBuffer& data)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    DeviceContext->Map(SpriteConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    FSpriteUVBuffer* buf = reinterpret_cast<FSpriteUVBuffer*>(mappedResource.pData);
    buf->UVOffsetX = data.UVOffsetX;
    buf->UVOffsetY = data.UVOffsetY;
    buf->UVScaleX = data.UVScaleX;
    buf->UVScaleY = data.UVScaleY;

    DeviceContext->Unmap(SpriteConstantBuffer, 0);
    DeviceContext->PSSetConstantBuffers(1, 1, &SpriteConstantBuffer);
}

void URenderer::CreateTexture(const std::string& filePath, const std::string& name)
{
    int width, height, channels;
    unsigned char* textureData = stbi_load(filePath.c_str(), &width, &height, &channels, 4);

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA textureSRD = {};
    textureSRD.pSysMem = textureData;
    textureSRD.SysMemPitch = width * 4;

    ID3D11Texture2D* newTexture = nullptr;
    ID3D11ShaderResourceView* newSRV = nullptr;

    Device->CreateTexture2D(&textureDesc, &textureSRD, &newTexture);
    if (newTexture) {
        Device->CreateShaderResourceView(newTexture, nullptr, &newSRV);
    }

    if (textureData) {
        stbi_image_free(textureData);
        textureData = nullptr;
    }

    Textures[name] = newTexture;
    SRVs[name] = newSRV;
}

void URenderer::ReleaseTexture(const std::string& name)
{
    auto textureIt = Textures.find(name);
    if (textureIt != Textures.end()) {
        if (textureIt->second) {
            textureIt->second->Release();
        }
        Textures.erase(textureIt);
    }
    auto srvIt = SRVs.find(name);
    if (srvIt != SRVs.end()) {
        if (srvIt->second) {
            srvIt->second->Release();
        }
        SRVs.erase(srvIt);
    }
}

void URenderer::BindTexture(const std::string& name)
{
    auto srvIt = SRVs.find(name);
    if (srvIt != SRVs.end()) {
        DeviceContext->PSSetShaderResources(0, 1, &srvIt->second);
    }
}

void URenderer::GetQuad(wchar_t c, float* x, float* y, stbtt_aligned_quad* q)
{
    if (c >= 32 && c < 128)
    {
        stbtt_GetPackedQuad(FontAtlas, 512, 512, c - 32, x, y, q, 1);
    }
    else
    {
        int index = HangulMap[c];

        stbtt_GetPackedQuad(FontHangulAtlas, 512, 512, index, x, y, q, 1);
    }
}

void URenderer::DrawString(const std::wstring& name, float x, float y, const FVector& color, ETextAlign align)
{
    std::vector<FTextVertex> vertices;

    float viewportWidth = Viewport.Width;
    float viewportHeight = Viewport.Height;

    float totalWidth = 0.0f;
    float tempX = 0.0f;
    float tempY = 0.0f;

    for (wchar_t c : name)
    {
        stbtt_aligned_quad q;
        GetQuad(c, &tempX, &tempY, &q);
    }
    totalWidth = tempX;

    float currX = x;
    float currY = y;
    switch (align)
    {
    case ETextAlign::Center:
        currX = x - (totalWidth / 2.0f);
        break;
    case ETextAlign::Right:
        currX = x - totalWidth;
        break;
    case ETextAlign::Left:
    default:
        currX = x;
    }

    for (wchar_t c : name)
    {
        stbtt_aligned_quad q;
        GetQuad(c, &currX, &currY, &q);

        // 픽셀 좌표를 NDC로 변환 (-1 ~ 1)
        float x0 = (q.x0 / viewportWidth) * 2.0f - 1.0f;
        float y0 = (q.y0 / viewportHeight) * 2.0f - 1.0f;
        float x1 = (q.x1 / viewportWidth) * 2.0f - 1.0f;
        float y1 = (q.y1 / viewportHeight) * 2.0f - 1.0f;

        FTextVertex v0 = { FVector(x0, -y0, 0.0f), q.s0, q.t0, color }; // Y축 반전
        FTextVertex v1 = { FVector(x1, -y0, 0.0f), q.s1, q.t0, color };
        FTextVertex v2 = { FVector(x1, -y1, 0.0f), q.s1, q.t1, color };
        FTextVertex v3 = { FVector(x0, -y1, 0.0f), q.s0, q.t1, color };

        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v0);
        vertices.push_back(v2);
        vertices.push_back(v3);
    }

    if (vertices.empty())
        return;

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    DeviceContext->Map(TextVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    memcpy(mappedResource.pData, vertices.data(), vertices.size() * sizeof(FTextVertex));
    DeviceContext->Unmap(TextVertexBuffer, 0);

    UINT stride = sizeof(FTextVertex);
    UINT offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &TextVertexBuffer, &stride, &offset);
    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    DeviceContext->IASetInputLayout(TextInputLayout);

    DeviceContext->VSSetShader(TextVertexShader, nullptr, 0);
    DeviceContext->PSSetShader(TextPixelShader, nullptr, 0);

    DeviceContext->PSSetShaderResources(0, 1, &FontAtlasSRV);
    DeviceContext->PSSetSamplers(0, 1, &DefaultSamplerState);

    FConstantBuffer constants;
    constants.position = FVector(0.0f, 0.0f, 0.0f);
    constants.rotation = FVector(0.0f, 0.0f, 0.0f);
    constants.scale = FVector(1.0f, 1.0f, 1.0f);
    UpdateConstantBuffer(constants);

    DeviceContext->Draw(vertices.size(), 0);
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

ID3D11Device* URenderer::GetDevice() {
    return Device;
}

ID3D11DeviceContext* URenderer::GetDeviceContext() {
    return DeviceContext;
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
    Viewport = { 0.0f, 0.0f, (float)frameBufferDesc.Width, (float)frameBufferDesc.Height, 0.0f, 1.0f };

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
    bufferDesc.ByteWidth = sizeof(FConstantBuffer) + 0xf & ~0xf;
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

void URenderer::CreateSpriteConstantBuffer()
{
    FSpriteUVBuffer initData = { 0.f, 0.f, 1.f, 1.f }; // 기본값: 전체 텍스처

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(FSpriteUVBuffer);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = &initData;

    Device->CreateBuffer(&bufferDesc, &data, &SpriteConstantBuffer);
}

void URenderer::ReleaseSpriteConstantBuffer()
{
    if (SpriteConstantBuffer)
    {
        SpriteConstantBuffer->Release();
        SpriteConstantBuffer = nullptr;
    }
}

void URenderer::CreateDefaultQuad()
{
    FVertex quadVertices[] =
    {
        { FVector(-0.5f, 0.5f, 0.0f), 0.0f, 0.0f },
        { FVector(0.5f, 0.5f, 0.0f), 1.0f, 0.0f },
        { FVector(0.5f, -0.5f, 0.0f), 1.0f, 1.0f },
        { FVector(-0.5f, -0.5f, 0.0f), 0.0f, 1.0f }
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
    std::wstring shaderPath = L"asset/shader/DefaultShader.hlsl";

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

void URenderer::CreateDefaultTexture()
{
    int width, height, channels;
    unsigned char* textureData = stbi_load("player.png", &width, &height, &channels, 4);

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA textureSRD = {};
    textureSRD.pSysMem = textureData;
    textureSRD.SysMemPitch = width * 4;

    Device->CreateTexture2D(&textureDesc, &textureSRD, &DefaultTexture);
    if (DefaultTexture) {
        Device->CreateShaderResourceView(DefaultTexture, nullptr, &DefaultSRV);
    }

    if (textureData) {
        stbi_image_free(textureData);
        textureData = nullptr;
    }
}

void URenderer::ReleaseDefaultTexture()
{
    if (DefaultSRV) {
        DefaultSRV->Release();
        DefaultSRV = nullptr;
    }
    if (DefaultTexture) {
        DefaultTexture->Release();
        DefaultTexture = nullptr;
    }
}

void URenderer::CreateDefaultSamplerState()
{
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    Device->CreateSamplerState(&sampDesc, &DefaultSamplerState);
}

void URenderer::ReleaseDefaultSamplerState()
{
    if (DefaultSamplerState)
    {
        DefaultSamplerState->Release();
        DefaultSamplerState = nullptr;
    }
}

void URenderer::CreateBlendState()
{
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;

    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    Device->CreateBlendState(&blendDesc, &BlendState);
}

void URenderer::ReleaseBlendState()
{
    if (BlendState) {
        BlendState->Release();
        BlendState = nullptr;
    }
}

static unsigned char* load_file(const char* fileName)
{
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << fileName << std::endl;
        return nullptr;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    unsigned char* buffer = new unsigned char[size];
    if (!buffer) {
        return nullptr;
    }

    if (file.read((char*)buffer, size)) {
        return buffer;
    }

    delete[] buffer;
    return nullptr;
}

void URenderer::CreateDefaultFontAtlasAndVertexBuffer()
{
    unsigned char* fontBuffer = load_file("asset/font/Pixel-Game.ttf");
    if (!fontBuffer)
    {
        return;
    }

    unsigned char* alphaPixels = new unsigned char[512 * 512];
    memset(alphaPixels, 0, 512 * 512);

    stbtt_pack_context pc;
    stbtt_PackBegin(&pc, alphaPixels, 512, 512, 0, 1, NULL);

    stbtt_pack_range ranges[2];

    ranges[0].font_size = 32.0f;
    ranges[0].first_unicode_codepoint_in_range = 32;
    ranges[0].num_chars = 96;
    ranges[0].chardata_for_range = FontAtlas;
    ranges[0].array_of_unicode_codepoints = nullptr;

    static const wchar_t* hangulChars = L"성원희김기홍오준혁국동진";
    ranges[1].font_size = 32.0f;
    ranges[1].first_unicode_codepoint_in_range = 0;
    ranges[1].num_chars = 12;
    ranges[1].chardata_for_range = FontHangulAtlas;
    ranges[1].array_of_unicode_codepoints = (int*)hangulChars;

    stbtt_PackFontRanges(&pc, fontBuffer, 0, ranges, 2);
    stbtt_PackEnd(&pc);

    for (int i = 0; i < 12; ++i) {
        HangulMap[hangulChars[i]] = i;
    }

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = 512;
    texDesc.Height = 512;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = { alphaPixels, 512, 0 };
    ID3D11Texture2D* fontTexture;
    Device->CreateTexture2D(&texDesc, &initData, &fontTexture);
    if (fontTexture)
    {
        Device->CreateShaderResourceView(fontTexture, nullptr, &FontAtlasSRV);
    }

    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.ByteWidth = sizeof(FTextVertex) * 6 * 100; // Max 100 characters
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    Device->CreateBuffer(&vertexBufferDesc, nullptr, &TextVertexBuffer);
}

void URenderer::ReleaseDefaultFontAtlasAndVertexBuffer()
{
    if (TextVertexBuffer)
    {
        TextVertexBuffer->Release();
        TextVertexBuffer = nullptr;
    }

    if (FontAtlasSRV)
    {
        FontAtlasSRV->Release();
        FontAtlasSRV = nullptr;
    }
}

void URenderer::CreateTextShader()
{
    std::wstring shaderPath = L"asset/shader/TextShader.hlsl";

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
    Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &TextVertexShader);

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
    Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &TextPixelShader);

    D3D11_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    Device->CreateInputLayout(inputLayout, ARRAYSIZE(inputLayout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &TextInputLayout);

    vsBlob->Release();
    psBlob->Release();
}

void URenderer::ReleaseTextShader()
{
    if (TextInputLayout)
    {
        TextInputLayout->Release();
        TextInputLayout = nullptr;
    }

    if (TextPixelShader)
    {
        TextPixelShader->Release();
        TextPixelShader = nullptr;
    }

    if (TextVertexShader)
    {
        TextVertexShader->Release();
        TextVertexShader = nullptr;
    }
}
