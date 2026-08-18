#include <windows.h>

// D3D ��뿡 �ʿ��� ���̺귯������ ��ũ�մϴ�.
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

// D3D ��뿡 �ʿ��� ������ϵ��� �����մϴ�.
#include <d3d11.h>
#include <d3dcompiler.h>

// ImGui ���
#include <d3d11.h>
#include <d3dcompiler.h>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

// 1. Define the triangle vertices
struct FVertexSimple
{
    float x, y, z;    // Position
    float r, g, b, a; // Color
};

// Structure for a 3D vector
struct FVector
{
    float x, y, z;
    FVector(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}

    float Length() const
    {
        return sqrtf(x * x + y * y + z * z);
    }

    FVector Normalize() const
    {
        float len = Length();
        if (len > 0.0001f)
            return FVector(x / len, y / len, z / len);
        return FVector(0, 0, 0);
    }

    float Dot(const FVector& other) const
    {
        return x * other.x + y * other.y + z * other.z;
    }

    FVector operator-(const FVector& other) const
    {
        return FVector(x - other.x, y - other.y, z - other.z);
    }

    FVector operator+(const FVector& other) const
    {
        return FVector(x + other.x, y + other.y, z + other.z);
    }

    FVector operator*(float scalar) const
    {
        return FVector(x * scalar, y * scalar, z * scalar);
    }
};

#include "Sphere.h"

class URenderer
{
public:
    struct FConstants
    {
        FVector Offset;
        float Scale;
        FVector Rotation;
        float ColorFactor;
        float Pad;
    };

    // Direct3D 11 ��ġ(Device)�� ��ġ ���ؽ�Ʈ(Device Context) �� ���� ü��(Swap Chain)�� �����ϱ� ���� �����͵�
    ID3D11Device* Device = nullptr; // GPU�� ����ϱ� ���� Direct3D ��ġ
    ID3D11DeviceContext* DeviceContext = nullptr; // GPU ��� ������ ����ϴ� ���ؽ�Ʈ
    IDXGISwapChain* SwapChain = nullptr; // ������ ���۸� ��ü�ϴ� �� ���Ǵ� ���� ü��

    // �������� �ʿ��� ���ҽ� �� ���¸� �����ϱ� ���� ������
    ID3D11Texture2D* FrameBuffer = nullptr; // ȭ�� ��¿� �ؽ�ó
    ID3D11RenderTargetView* FrameBufferRTV = nullptr; // �ؽ�ó�� ���� Ÿ������ ����ϴ� ��
    ID3D11RasterizerState* RasterizerState = nullptr; // �����Ͷ����� ����(�ø�, ä��� ��� �� ����)
    ID3D11Buffer* ConstantBuffer = nullptr; // ���̴��� �����͸� �����ϱ� ���� ��� ����

    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f }; // ȭ���� �ʱ�ȭ(clear)�� �� ����� ���� (RGBA)
    D3D11_VIEWPORT ViewportInfo; // ������ ������ �����ϴ� ����Ʈ ����

    // Shader
    ID3D11VertexShader* SimpleVertexShader;
    ID3D11PixelShader* SimplePixelShader;
    ID3D11InputLayout* SimpleInputLayout;
    unsigned int Stride;

public:
    // ������ �ʱ�ȭ �Լ�
    void Create(HWND hWindow)
    {
        // Direct3D ��ġ �� ���� ü�� ����
        CreateDeviceAndSwapChain(hWindow);

        // ������ ���� ����
        CreateFrameBuffer();

        // �����Ͷ����� ���� ����
        CreateRasterizerState();

        // ���� ���ٽ� ���� �� ����� ���´� �� �ڵ忡���� �ٷ��� ����
    }

    // Direct3D ��ġ �� ���� ü���� �����ϴ� �Լ�
    void CreateDeviceAndSwapChain(HWND hWindow)
    {
        // �����ϴ� Direct3D ��� ������ ����
        D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

        // ���� ü�� ���� ����ü �ʱ�ȭ
        DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
        swapchaindesc.BufferDesc.Width = 0; // â ũ�⿡ �°� �ڵ����� ����
        swapchaindesc.BufferDesc.Height = 0; // â ũ�⿡ �°� �ڵ����� ����
        swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // ���� ����
        swapchaindesc.SampleDesc.Count = 1; // ��Ƽ ���ø� ��Ȱ��ȭ
        swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // ���� Ÿ������ ���
        swapchaindesc.BufferCount = 2; // ���� ���۸�
        swapchaindesc.OutputWindow = hWindow; // �������� â �ڵ�
        swapchaindesc.Windowed = TRUE; // â ���
        swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // ���� ���

        // Direct3D ��ġ�� ���� ü���� ����
        D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
            featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
            &swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext);

        // ������ ���� ü���� ���� ��������
        SwapChain->GetDesc(&swapchaindesc);

        // ����Ʈ ���� ����
        ViewportInfo = { 0.0f, 0.0f, (float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height, 0.0f, 1.0f };
    }

    // Direct3D ��ġ �� ���� ü���� �����ϴ� �Լ�
    void ReleaseDeviceAndSwapChain()
    {
        if (DeviceContext)
        {
            DeviceContext->Flush(); // �����ִ� GPU ��� ����
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

    // ������ ���۸� �����ϴ� �Լ�
    void CreateFrameBuffer()
    {
        // ���� ü�����κ��� �� ���� �ؽ�ó ��������
        SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

        // ���� Ÿ�� �� ����
        D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
        framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // ���� ����
        framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D �ؽ�ó

        Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
    }

    // ������ ���۸� �����ϴ� �Լ�
    void ReleaseFrameBuffer()
    {
        if (FrameBuffer)
        {
            FrameBuffer->Release();
            FrameBuffer = nullptr;
        }

        if (FrameBufferRTV)
        {
            FrameBufferRTV->Release();
            FrameBufferRTV = nullptr;
        }
    }

    // �����Ͷ����� ���¸� �����ϴ� �Լ�
    void CreateRasterizerState()
    {
        D3D11_RASTERIZER_DESC rasterizerdesc = {};
        rasterizerdesc.FillMode = D3D11_FILL_SOLID; // ä��� ���
        rasterizerdesc.CullMode = D3D11_CULL_BACK; // �� ���̽� �ø�

        Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState);
    }

    // �����Ͷ����� ���¸� �����ϴ� �Լ�
    void ReleaseRasterizerState()
    {
        if (RasterizerState)
        {
            RasterizerState->Release();
            RasterizerState = nullptr;
        }
    }

    // �������� ���� ��� ���ҽ��� �����ϴ� �Լ�
    void Release()
    {
        RasterizerState->Release();

        // ���� Ÿ���� �ʱ�ȭ
        DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

        ReleaseFrameBuffer();
        ReleaseDeviceAndSwapChain();
    }

    // ���� ü���� �� ���ۿ� ����Ʈ ���۸� ��ü�Ͽ� ȭ�鿡 ���
    void SwapBuffer()
    {
        SwapChain->Present(1, 0); // 1: VSync Ȱ��ȭ
    }

    void CreateShader()
    {
        ID3DBlob* vertexshaderCSO;
        ID3DBlob* pixelshaderCSO;

        D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vertexshaderCSO, nullptr);

        Device->CreateVertexShader(vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), nullptr, &SimpleVertexShader);

        D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &pixelshaderCSO, nullptr);

        Device->CreatePixelShader(pixelshaderCSO->GetBufferPointer(), pixelshaderCSO->GetBufferSize(), nullptr, &SimplePixelShader);

        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        Device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), &SimpleInputLayout);
         
        Stride = sizeof(FVertexSimple);

        vertexshaderCSO->Release();
        pixelshaderCSO->Release();
    }

    void ReleaseShader()
    {
        if (SimpleInputLayout)
        {
            SimpleInputLayout->Release();
            SimpleInputLayout = nullptr;
        }

        if (SimplePixelShader)
        {
            SimplePixelShader->Release();
            SimplePixelShader = nullptr;
        }

        if (SimpleVertexShader)
        {
            SimpleVertexShader->Release();
            SimpleVertexShader = nullptr;
        }
    }

    // URenderer Class�� �Ʒ� �Լ��� �߰� �ϼ���.
    void Prepare()
    {
        DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);

        DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        DeviceContext->RSSetViewports(1, &ViewportInfo);
        DeviceContext->RSSetState(RasterizerState);

        DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);
        DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    }

    void PrepareShader()
    {
        DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
        DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
        DeviceContext->IASetInputLayout(SimpleInputLayout);

        // ���ؽ� ���̴��� ��� ���۸� �����մϴ�.
        if (ConstantBuffer)
        {
            DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
        }
    }

    void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices)
    {
        UINT offset = 0;
        DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);

        DeviceContext->Draw(numVertices, 0);
    }

    ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth)
    {
        // 2. Create a vertex buffer
        D3D11_BUFFER_DESC vertexbufferdesc = {};
        vertexbufferdesc.ByteWidth = byteWidth;
        vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE; // will never be updated 
        vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertexbufferSRD = { vertices };

        ID3D11Buffer* vertexBuffer;

        Device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &vertexBuffer);

        return vertexBuffer;
    }

    void ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer)
    {
        vertexBuffer->Release();
    }

    void CreateConstantBuffer()
    {
        D3D11_BUFFER_DESC constantbufferdesc = {};
        constantbufferdesc.ByteWidth = sizeof(FConstants) + 0xf & 0xfffffff0; // ensure constant buffer size is multiple of 16 bytes
        constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU every frame
        constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
    }

    void ReleaseConstantBuffer()
    {
        if (ConstantBuffer)
        {
            ConstantBuffer->Release();
            ConstantBuffer = nullptr;
        }
    }

    void UpdateConstant(FVector Offset, float Scale = 1.0f, FVector Rotation = 0.0f, float ColorFactor = 1.0f)
    {
        if (ConstantBuffer)
        {
            D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

            DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // update constant buffer every frame
            FConstants* constants = (FConstants*)constantbufferMSR.pData;
            {
                constants->Offset = Offset;
                constants->Scale = Scale;
                constants->Rotation = Rotation;
                constants->ColorFactor = ColorFactor;
            }
            DeviceContext->Unmap(ConstantBuffer, 0);
        }
    }
};

class UPrimitive
{
public:
    virtual ~UPrimitive() {}
    virtual void Update(float t, bool bApplyGravity, bool bApplyAngularVelocity) = 0;
    virtual void Render(URenderer& renderer, bool bDarkening) = 0;
    virtual bool CheckCollision(UPrimitive* other) = 0;
    virtual void ApplyImpulse(const FVector& v) = 0;
};

class UBall : public UPrimitive
{
public:
    // Ŭ���� �̸���, �Ʒ� �ټ����� ���� �̸��� �������� �ʽ��ϴ�.
    FVector Location;
    FVector Velocity;
    float Radius;
    float Mass;
    static int TotalNumBalls;

    // ���� �߰��� ������ �Լ� �̸��� �����Ӱ� ���ϼ���.

    // ȸ�� ����
    FVector Rotation = 0.0f;        // ���� ȸ�� ���� (����)
    FVector AngularVelocity = 0.0f; // ���ӵ�

    // ź�� ����
    float Restitution = 1.0f;

    // �ٸ� Ball�� �浹 Ƚ��
    int NumHits;

    // �ʱ�ȭ �Լ�
    void Reset()
    {
        Location.x = ((float)(rand() % 200 - 100)) * 0.005f;
        Location.y = ((float)(rand() % 200 - 100)) * 0.005f;
        Location.z = 0.0f;

        Velocity.x = ((float)(rand() % 100 - 50)) * 0.0001f; 
        Velocity.y = ((float)(rand() % 100 - 50)) * 0.001f;
        Velocity.z = 0.0f;

        Radius = 0.02f + ((float)(rand() % 100)) * 0.001f * (1.0f - 0.02f);
        Mass = Radius;  // ���ǿ� ���

        Rotation = FVector(0, 0, 0);
        AngularVelocity = FVector(
            ((float)(rand() % 100) * 0.001f),
            ((float)(rand() % 100) * 0.001f),
            ((float)(rand() % 100) * 0.001f)
        );

        Restitution = 0.5f + ((float)(rand() % 500)) * 0.001f;

        NumHits = 0;
    }

    UBall()
    {
        // ������ �� �� ���� ����
        TotalNumBalls++;
        Reset();
    }

    ~UBall()
    {
        // �ı��� �� �� ���� ����
        TotalNumBalls--;
    }

    virtual void Update(float t, bool bApplyGravity, bool bApplyAngularVelocity) override
    {
        // �߷� ����
        if (bApplyGravity)
        {
            const float GravityConstant = -0.01f; // �߷� ���ӵ� ���� ����
            Velocity.y += GravityConstant * t;
        }

        // ���ӵ� ����
        if (bApplyAngularVelocity) {
            Rotation.x += AngularVelocity.x * t;
            Rotation.y += AngularVelocity.y * t;
            Rotation.z += AngularVelocity.z * t;
        }

        // ��ġ ������Ʈ
        Location = Location + Velocity * t;
    }

    virtual void Render(URenderer& renderer, bool bDarkening) override {
        float factor = 1.0f;

        if (bDarkening)
        {
            // �浹 1ȸ�� 5%�� ��ο����� ���� (20�� �浹 �� ������)
            factor = 1.0f - (NumHits * 0.05f);
            if (factor < 0.0f) factor = 0.0f;
        }

        renderer.UpdateConstant(Location, Radius, Rotation, factor);
    }

    virtual bool CheckCollision(UPrimitive* other) override
    {
        UBall* otherBall = dynamic_cast<UBall*>(other);
        if (otherBall == nullptr)
            return false;

        // �� �� ������ �Ÿ� ����
        FVector diff = Location - otherBall->Location;
        float distance = diff.Length();

        // �� ���� ������ ��
        float radiusSum = Radius + otherBall->Radius;

        // �浹 üũ
        return distance < radiusSum;
    }

    virtual void ApplyImpulse(const FVector& v) override
    {
        Velocity = Velocity + v * (1.0f / Mass);
    }

    // ������ �浹 üũ �� ó��
    void CheckWallCollision(float leftBorder, float rightBorder, float topBorder, float bottomBorder, bool bApplyRestitution)
    {
        float restitution = bApplyRestitution ? Restitution : 1.0f;

        // ���� ��
        if (Location.x - Radius < leftBorder)
        {
            Location.x = leftBorder + Radius;
            Velocity.x = -Velocity.x * restitution;
        }
        // ������ ��
        if (Location.x + Radius > rightBorder)
        {
            Location.x = rightBorder - Radius;
            Velocity.x = -Velocity.x * restitution;
        }
        // ���� ��
        if (Location.y - Radius < topBorder)
        {
            Location.y = topBorder + Radius;
            Velocity.y = -Velocity.y * restitution;
        }
        // �Ʒ��� ��
        if (Location.y + Radius > bottomBorder)
        {
            Location.y = bottomBorder - Radius;
            Velocity.y = -Velocity.y * restitution;
        }
    }
};

// static ���� �ʱ�ȭ
int UBall::TotalNumBalls = 0;

// ImGui��
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ���� �޽����� ó���� �Լ�
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }

	switch (message)
	{
	case WM_DESTROY:
		// Signal that the app should quit
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// ������ Ŭ���� �̸�
	WCHAR WindowClass[] = L"JungleWindowClass";

	// ������ Ÿ��Ʋ�ٿ� ǥ�õ� �̸�
	WCHAR Title[] = L"Game Tech Lab";

	// ���� �޽����� ó���� �Լ��� WndProc�� �Լ� �����͸� WindowClass ����ü�� �ִ´�.
	WNDCLASSW wndclass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };

	// ������ Ŭ���� ���
	RegisterClassW(&wndclass);

	// 1024 x 1024 ũ�⿡ ������ ����
    // ������ Ŭ���� �̸��� ������� Ŭ���� ��ü ���� �� �ڵ� ��ȯ
	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024,
		nullptr, nullptr, hInstance, nullptr);

	bool bIsExit = false;

    // Renderer Class�� �����մϴ�.
    URenderer	renderer;

    // D3D11 �����ϴ� �Լ��� ȣ���մϴ�.
    renderer.Create(hWnd);

    // ������ ���� ���Ŀ� ���̴��� �����ϴ� �Լ��� ȣ���մϴ�.
    renderer.CreateShader();
    renderer.CreateConstantBuffer();

    // ���⿡�� ImGui�� �����մϴ�.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Init((void*)hWnd);
    ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);

    // Renderer�� Shader ���� ���Ŀ� ���ؽ� ���۸� �����մϴ�.
    UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);
    ID3D11Buffer* vertexBufferSphere = renderer.CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));

    // ȭ�� ���
    const float leftBorder = -1.0f;
    const float rightBorder = 1.0f;
    const float topBorder = -1.0f;
    const float bottomBorder = 1.0f;
    const float sphereRadius = 1.0f;

    // Ball ��� ���������� ����
    UPrimitive** PrimitiveList = nullptr;
    int CurrentCapacity = 0;  // ���� �Ҵ�� �迭 ũ��

    // �߷� ����
    bool bGravity = true;

    // �߰� ����
    bool bAngularVelocity = false;  // ���ӵ�
    bool bRestitution = false;      // ź��
    bool bHitDarkening = false;     // �浹 �� ��ο���

    // FPS ������ ���� ����
    const int targetFPS = 30;
    const double targetFrameTime = 1000.0 / targetFPS; // �� �������� ��ǥ �ð� (�и��� ����)

    // ����� Ÿ�̸� �ʱ�ȭ
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    LARGE_INTEGER startTime, endTime;
    double elapsedTime = 0.0;

	// Main Loop
	while (bIsExit == false)
	{
        QueryPerformanceCounter(&startTime);
		MSG msg;

		// ó���� �޽����� �� �̻� ������ ���� ����
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			// Ű �Է� �޽����� ����
			TranslateMessage(&msg);

			// �޽����� ������ ������ ���ν����� ����, �޽����� ������ ����� WndProc ���� ���޵�
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				bIsExit = true;
				break;
			}
		}

        // Ball ��ġ ������Ʈ
        float deltaTime = 1.0f;
        for (int i = 0; i < UBall::TotalNumBalls; ++i)
        {
            if (PrimitiveList[i] != nullptr)
            {
                UBall* ball = static_cast<UBall*>(PrimitiveList[i]);

                // ��ġ ������Ʈ
                ball->Update(deltaTime, bGravity, bAngularVelocity);

                // �� �浹 üũ
                ball->CheckWallCollision(leftBorder, rightBorder, topBorder, bottomBorder, bRestitution);
            }
        }

        // �� �� �浹 üũ �� ó��
        for (int i = 0; i < UBall::TotalNumBalls; ++i)
        {
            if (PrimitiveList[i] == nullptr)
                continue;

            UBall* ball1 = static_cast<UBall*>(PrimitiveList[i]);

            for (int j = i + 1; j < UBall::TotalNumBalls; ++j)
            {
                if (PrimitiveList[j] == nullptr)
                    continue;

                UBall* ball2 = static_cast<UBall*>(PrimitiveList[j]);

                // �浹 üũ
                if (ball1->CheckCollision(ball2))
                {
                    // ź�� �浹 ó��
                    FVector diff = ball1->Location - ball2->Location;
                    float distance = diff.Length();

                    // ��ħ ���� - ������ �и�
                    if (distance > 0.0001f)
                    {
                        FVector normal = diff.Normalize();
                        float overlap = (ball1->Radius + ball2->Radius) - distance;

                        // ���� ������ ���� �и�
                        float totalMass = ball1->Mass + ball2->Mass;
                        ball1->Location = ball1->Location + normal * (overlap * ball2->Mass / totalMass);
                        ball2->Location = ball2->Location - normal * (overlap * ball1->Mass / totalMass);

                        // ź�� �浹 �ӵ� ���
                        // ��� �ӵ�
                        FVector relativeVelocity = ball1->Velocity - ball2->Velocity;

                        // ���� ���� �ӵ�
                        float velocityAlongNormal = relativeVelocity.Dot(normal);

                        // �̹� �и� ���̸� �浹 ó�� ����
                        if (velocityAlongNormal > 0)
                            continue;

                        // �ݹ� ��� (���� ź�� �浹 = 1.0)
                        float restitution = 1.0f;
                        if(bRestitution)
                            restitution = (ball1->Restitution + ball2->Restitution) * 0.5f;

                        // ��ݷ� ũ�� ���
                        float impulseMagnitude = -(1 + restitution) * velocityAlongNormal;
                        impulseMagnitude /= (1 / ball1->Mass + 1 / ball2->Mass);

                        // ��ݷ� ����
                        FVector impulse = normal * impulseMagnitude;

                        // �ӵ� ������Ʈ
                        ball1->ApplyImpulse(impulse);
                        ball2->ApplyImpulse(impulse * (-1.0f));

                        // �浹 Ƚ�� ����
                        if (bHitDarkening) {
                            ball1->NumHits++;
                            ball2->NumHits++;
                        }
                    }
                }
            }
        }

		////////////////////////////////////////////
		// �Ź� ����Ǵ� �ڵ带 ���⿡ �߰��մϴ�.

        // �غ� �۾�
        renderer.Prepare();
        renderer.PrepareShader();

        // ImGui
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ���� ImGui UI ��Ʈ�� �߰��� ImGui::NewFrame()�� ImGui::Render() ������ ���⿡ ��ġ�մϴ�.
        ImGui::Begin("Jungle Property Window");
        ImGui::Text("Hello Jungle World!");

        ImGui::Checkbox("Gravity", &bGravity);
        ImGui::Checkbox("Angular Velocity", &bAngularVelocity); // ���ӵ�
        ImGui::Checkbox("Restitution", &bRestitution);          // ź��
        ImGui::Checkbox("Darken on Hits", &bHitDarkening);      // �浹 �� ��ο���

        int DesiredNumBalls = UBall::TotalNumBalls;
        if (ImGui::InputInt("Number of Balls", &DesiredNumBalls, 1, 100))
        {
            // �Է°��� 1 �̻����� ����
            if (DesiredNumBalls < 1)
            {
                DesiredNumBalls = 1;
            }

            // �� ������ ������ ���
            if (DesiredNumBalls > UBall::TotalNumBalls)
            {
                int NumToCreate = DesiredNumBalls - UBall::TotalNumBalls;

                // �迭 ũ�Ⱑ �����ϸ� Ȯ��
                if (DesiredNumBalls > CurrentCapacity)
                {
                    int NewCapacity = DesiredNumBalls + 10; // ���� ���� Ȯ��
                    UPrimitive** NewList = new UPrimitive * [NewCapacity];

                    // ���� ������ ����
                    for (int i = 0; i < UBall::TotalNumBalls; ++i)
                    {
                        NewList[i] = PrimitiveList[i];
                    }

                    // ���� �迭 ����
                    if (PrimitiveList != nullptr)
                    {
                        delete[] PrimitiveList;
                    }

                    PrimitiveList = NewList;
                    CurrentCapacity = NewCapacity;
                }

                // �� �� ����
                for (int i = 0; i < NumToCreate; ++i)
                {
                    UBall* NewBall = new UBall();
                    PrimitiveList[UBall::TotalNumBalls - 1] = NewBall;
                }
            }
            // �� ������ ������ ���
            else if (DesiredNumBalls < UBall::TotalNumBalls)
            {
                int NumToDelete = UBall::TotalNumBalls - DesiredNumBalls;

                for (int i = 0; i < NumToDelete; ++i)
                {
                    if (UBall::TotalNumBalls > 0)
                    {
                        // �����ϰ� �� �����Ͽ� ����
                        int RandomIndex = rand() % UBall::TotalNumBalls;

                        // ���õ� �� ����
                        delete PrimitiveList[RandomIndex];
                        PrimitiveList[RandomIndex] = nullptr;

                        // �迭���� ���� (������ ��Ҹ� �� �ڸ��� �̵�)
                        if (RandomIndex < UBall::TotalNumBalls)
                        {
                            PrimitiveList[RandomIndex] = PrimitiveList[UBall::TotalNumBalls];
                            PrimitiveList[UBall::TotalNumBalls] = nullptr;
                        }
                    }
                }
            }

        }

        ImGui::End();

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // ��� �� ������
        for (int i = 0; i < UBall::TotalNumBalls; ++i)
        {
            if (PrimitiveList[i] != nullptr)
            {
                PrimitiveList[i]->Render(renderer, bHitDarkening);
                renderer.RenderPrimitive(vertexBufferSphere, numVerticesSphere);
            }
        }

        // ���� ȭ�鿡 �������� ���ۿ� �׸��� �۾��� ���� ���۸� ���� ��ȯ�մϴ�.
        renderer.SwapBuffer();

        // ��� ȯ�濡�� FPS ���߱� ����
        do
        {
            Sleep(0);

            // ���� ���� �ð� ���
            QueryPerformanceCounter(&endTime);

            // �� �������� �ҿ�� �ð� ��� (�и��� ������ ��ȯ)
            elapsedTime = (endTime.QuadPart - startTime.QuadPart) * 1000.0 / frequency.QuadPart;

        } while (elapsedTime < targetFrameTime);
		////////////////////////////////////////////
	}

    // ���α׷� ���� �� ��� �� �޸� ����
    for (int i = 0; i < UBall::TotalNumBalls; ++i)
    {
        if (PrimitiveList[i] != nullptr)
        {
            delete PrimitiveList[i];
            PrimitiveList[i] = nullptr;
        }
    }
    if (PrimitiveList != nullptr)
    {
        delete[] PrimitiveList;
        PrimitiveList = nullptr;
    }

    // ���⿡�� ImGui �Ҹ�
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // D3D11 �Ҹ� ��Ű�� �Լ��� ȣ���մϴ�.
    renderer.ReleaseVertexBuffer(vertexBufferSphere);

    renderer.ReleaseConstantBuffer();
    renderer.ReleaseShader();
    renderer.Release();

    return 0;
}