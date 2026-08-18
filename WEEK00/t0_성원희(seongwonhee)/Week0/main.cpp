#include <windows.h>

// D3D 사용에 필요한 라이브러리들을 링크합니다.
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

// D3D 사용에 필요한 헤더파일들을 포함합니다.
#include <d3d11.h>
#include <d3dcompiler.h>

// ImGui 헤더
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

    // Direct3D 11 장치(Device)와 장치 컨텍스트(Device Context) 및 스왑 체인(Swap Chain)을 관리하기 위한 포인터들
    ID3D11Device* Device = nullptr; // GPU와 통신하기 위한 Direct3D 장치
    ID3D11DeviceContext* DeviceContext = nullptr; // GPU 명령 실행을 담당하는 컨텍스트
    IDXGISwapChain* SwapChain = nullptr; // 프레임 버퍼를 교체하는 데 사용되는 스왑 체인

    // 렌더링에 필요한 리소스 및 상태를 관리하기 위한 변수들
    ID3D11Texture2D* FrameBuffer = nullptr; // 화면 출력용 텍스처
    ID3D11RenderTargetView* FrameBufferRTV = nullptr; // 텍스처를 렌더 타겟으로 사용하는 뷰
    ID3D11RasterizerState* RasterizerState = nullptr; // 래스터라이저 상태(컬링, 채우기 모드 등 정의)
    ID3D11Buffer* ConstantBuffer = nullptr; // 쉐이더에 데이터를 전달하기 위한 상수 버퍼

    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f }; // 화면을 초기화(clear)할 때 사용할 색상 (RGBA)
    D3D11_VIEWPORT ViewportInfo; // 렌더링 영역을 정의하는 뷰포트 정보

    // Shader
    ID3D11VertexShader* SimpleVertexShader;
    ID3D11PixelShader* SimplePixelShader;
    ID3D11InputLayout* SimpleInputLayout;
    unsigned int Stride;

public:
    // 렌더러 초기화 함수
    void Create(HWND hWindow)
    {
        // Direct3D 장치 및 스왑 체인 생성
        CreateDeviceAndSwapChain(hWindow);

        // 프레임 버퍼 생성
        CreateFrameBuffer();

        // 래스터라이저 상태 생성
        CreateRasterizerState();

        // 깊이 스텐실 버퍼 및 블렌드 상태는 이 코드에서는 다루지 않음
    }

    // Direct3D 장치 및 스왑 체인을 생성하는 함수
    void CreateDeviceAndSwapChain(HWND hWindow)
    {
        // 지원하는 Direct3D 기능 레벨을 정의
        D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

        // 스왑 체인 설정 구조체 초기화
        DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
        swapchaindesc.BufferDesc.Width = 0; // 창 크기에 맞게 자동으로 설정
        swapchaindesc.BufferDesc.Height = 0; // 창 크기에 맞게 자동으로 설정
        swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // 색상 포맷
        swapchaindesc.SampleDesc.Count = 1; // 멀티 샘플링 비활성화
        swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 렌더 타겟으로 사용
        swapchaindesc.BufferCount = 2; // 더블 버퍼링
        swapchaindesc.OutputWindow = hWindow; // 렌더링할 창 핸들
        swapchaindesc.Windowed = TRUE; // 창 모드
        swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 스왑 방식

        // Direct3D 장치와 스왑 체인을 생성
        D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
            featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
            &swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext);

        // 생성된 스왑 체인의 정보 가져오기
        SwapChain->GetDesc(&swapchaindesc);

        // 뷰포트 정보 설정
        ViewportInfo = { 0.0f, 0.0f, (float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height, 0.0f, 1.0f };
    }

    // Direct3D 장치 및 스왑 체인을 해제하는 함수
    void ReleaseDeviceAndSwapChain()
    {
        if (DeviceContext)
        {
            DeviceContext->Flush(); // 남아있는 GPU 명령 실행
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

    // 프레임 버퍼를 생성하는 함수
    void CreateFrameBuffer()
    {
        // 스왑 체인으로부터 백 버퍼 텍스처 가져오기
        SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

        // 렌더 타겟 뷰 생성
        D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
        framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // 색상 포맷
        framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

        Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
    }

    // 프레임 버퍼를 해제하는 함수
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

    // 래스터라이저 상태를 생성하는 함수
    void CreateRasterizerState()
    {
        D3D11_RASTERIZER_DESC rasterizerdesc = {};
        rasterizerdesc.FillMode = D3D11_FILL_SOLID; // 채우기 모드
        rasterizerdesc.CullMode = D3D11_CULL_BACK; // 백 페이스 컬링

        Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState);
    }

    // 래스터라이저 상태를 해제하는 함수
    void ReleaseRasterizerState()
    {
        if (RasterizerState)
        {
            RasterizerState->Release();
            RasterizerState = nullptr;
        }
    }

    // 렌더러에 사용된 모든 리소스를 해제하는 함수
    void Release()
    {
        RasterizerState->Release();

        // 렌더 타겟을 초기화
        DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

        ReleaseFrameBuffer();
        ReleaseDeviceAndSwapChain();
    }

    // 스왑 체인의 백 버퍼와 프론트 버퍼를 교체하여 화면에 출력
    void SwapBuffer()
    {
        SwapChain->Present(1, 0); // 1: VSync 활성화
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

    // URenderer Class에 아래 함수를 추가 하세요.
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

        // 버텍스 쉐이더에 상수 버퍼를 설정합니다.
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
    // 클래스 이름과, 아래 다섯개의 변수 이름은 변경하지 않습니다.
    FVector Location;
    FVector Velocity;
    float Radius;
    float Mass;
    static int TotalNumBalls;

    // 이후 추가할 변수와 함수 이름은 자유롭게 정하세요.

    // 회전 변수
    FVector Rotation = 0.0f;        // 현재 회전 각도 (라디안)
    FVector AngularVelocity = 0.0f; // 각속도

    // 탄성 변수
    float Restitution = 1.0f;

    // 다른 Ball과 충돌 횟수
    int NumHits;

    // 초기화 함수
    void Reset()
    {
        Location.x = ((float)(rand() % 200 - 100)) * 0.005f;
        Location.y = ((float)(rand() % 200 - 100)) * 0.005f;
        Location.z = 0.0f;

        Velocity.x = ((float)(rand() % 100 - 50)) * 0.0001f; 
        Velocity.y = ((float)(rand() % 100 - 50)) * 0.001f;
        Velocity.z = 0.0f;

        Radius = 0.02f + ((float)(rand() % 100)) * 0.001f * (1.0f - 0.02f);
        Mass = Radius;  // 부피에 비례

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
        // 생성될 때 총 개수 증가
        TotalNumBalls++;
        Reset();
    }

    ~UBall()
    {
        // 파괴될 때 총 개수 감소
        TotalNumBalls--;
    }

    virtual void Update(float t, bool bApplyGravity, bool bApplyAngularVelocity) override
    {
        // 중력 적용
        if (bApplyGravity)
        {
            const float GravityConstant = -0.01f; // 중력 가속도 임의 설정
            Velocity.y += GravityConstant * t;
        }

        // 각속도 적용
        if (bApplyAngularVelocity) {
            Rotation.x += AngularVelocity.x * t;
            Rotation.y += AngularVelocity.y * t;
            Rotation.z += AngularVelocity.z * t;
        }

        // 위치 업데이트
        Location = Location + Velocity * t;
    }

    virtual void Render(URenderer& renderer, bool bDarkening) override {
        float factor = 1.0f;

        if (bDarkening)
        {
            // 충돌 1회당 5%씩 어두워지게 설정 (20번 충돌 시 검은색)
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

        // 두 공 사이의 거리 벡터
        FVector diff = Location - otherBall->Location;
        float distance = diff.Length();

        // 두 공의 반지름 합
        float radiusSum = Radius + otherBall->Radius;

        // 충돌 체크
        return distance < radiusSum;
    }

    virtual void ApplyImpulse(const FVector& v) override
    {
        Velocity = Velocity + v * (1.0f / Mass);
    }

    // 벽과의 충돌 체크 및 처리
    void CheckWallCollision(float leftBorder, float rightBorder, float topBorder, float bottomBorder, bool bApplyRestitution)
    {
        float restitution = bApplyRestitution ? Restitution : 1.0f;

        // 왼쪽 벽
        if (Location.x - Radius < leftBorder)
        {
            Location.x = leftBorder + Radius;
            Velocity.x = -Velocity.x * restitution;
        }
        // 오른쪽 벽
        if (Location.x + Radius > rightBorder)
        {
            Location.x = rightBorder - Radius;
            Velocity.x = -Velocity.x * restitution;
        }
        // 위쪽 벽
        if (Location.y - Radius < topBorder)
        {
            Location.y = topBorder + Radius;
            Velocity.y = -Velocity.y * restitution;
        }
        // 아래쪽 벽
        if (Location.y + Radius > bottomBorder)
        {
            Location.y = bottomBorder - Radius;
            Velocity.y = -Velocity.y * restitution;
        }
    }
};

// static 변수 초기화
int UBall::TotalNumBalls = 0;

// ImGui용
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 각종 메시지를 처리할 함수
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
	// 윈도우 클래스 이름
	WCHAR WindowClass[] = L"JungleWindowClass";

	// 윈도우 타이틀바에 표시될 이름
	WCHAR Title[] = L"Game Tech Lab";

	// 각종 메시지를 처리할 함수인 WndProc의 함수 포인터를 WindowClass 구조체에 넣는다.
	WNDCLASSW wndclass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };

	// 윈도우 클래스 등록
	RegisterClassW(&wndclass);

	// 1024 x 1024 크기에 윈도우 생성
    // 윈도우 클래스 이름을 기반으로 클래스 객체 생성 후 핸들 반환
	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024,
		nullptr, nullptr, hInstance, nullptr);

	bool bIsExit = false;

    // Renderer Class를 생성합니다.
    URenderer	renderer;

    // D3D11 생성하는 함수를 호출합니다.
    renderer.Create(hWnd);

    // 렌더러 생성 직후에 쉐이더를 생성하는 함수를 호출합니다.
    renderer.CreateShader();
    renderer.CreateConstantBuffer();

    // 여기에서 ImGui를 생성합니다.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Init((void*)hWnd);
    ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);

    // Renderer와 Shader 생성 이후에 버텍스 버퍼를 생성합니다.
    UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);
    ID3D11Buffer* vertexBufferSphere = renderer.CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));

    // 화면 경계
    const float leftBorder = -1.0f;
    const float rightBorder = 1.0f;
    const float topBorder = -1.0f;
    const float bottomBorder = 1.0f;
    const float sphereRadius = 1.0f;

    // Ball 담는 이중포인터 생성
    UPrimitive** PrimitiveList = nullptr;
    int CurrentCapacity = 0;  // 현재 할당된 배열 크기

    // 중력 설정
    bool bGravity = true;

    // 추가 구현
    bool bAngularVelocity = false;  // 각속도
    bool bRestitution = false;      // 탄성
    bool bHitDarkening = false;     // 충돌 시 어두워짐

    // FPS 제한을 위한 설정
    const int targetFPS = 30;
    const double targetFrameTime = 1000.0 / targetFPS; // 한 프레임의 목표 시간 (밀리초 단위)

    // 고성능 타이머 초기화
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    LARGE_INTEGER startTime, endTime;
    double elapsedTime = 0.0;

	// Main Loop
	while (bIsExit == false)
	{
        QueryPerformanceCounter(&startTime);
		MSG msg;

		// 처리할 메시지가 더 이상 없을때 까지 수행
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			// 키 입력 메시지를 번역
			TranslateMessage(&msg);

			// 메시지를 적절한 윈도우 프로시저에 전달, 메시지가 위에서 등록한 WndProc 으로 전달됨
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				bIsExit = true;
				break;
			}
		}

        // Ball 위치 업데이트
        float deltaTime = 1.0f;
        for (int i = 0; i < UBall::TotalNumBalls; ++i)
        {
            if (PrimitiveList[i] != nullptr)
            {
                UBall* ball = static_cast<UBall*>(PrimitiveList[i]);

                // 위치 업데이트
                ball->Update(deltaTime, bGravity, bAngularVelocity);

                // 벽 충돌 체크
                ball->CheckWallCollision(leftBorder, rightBorder, topBorder, bottomBorder, bRestitution);
            }
        }

        // 공 간 충돌 체크 및 처리
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

                // 충돌 체크
                if (ball1->CheckCollision(ball2))
                {
                    // 탄성 충돌 처리
                    FVector diff = ball1->Location - ball2->Location;
                    float distance = diff.Length();

                    // 겹침 방지 - 공들을 분리
                    if (distance > 0.0001f)
                    {
                        FVector normal = diff.Normalize();
                        float overlap = (ball1->Radius + ball2->Radius) - distance;

                        // 질량 비율에 따라 분리
                        float totalMass = ball1->Mass + ball2->Mass;
                        ball1->Location = ball1->Location + normal * (overlap * ball2->Mass / totalMass);
                        ball2->Location = ball2->Location - normal * (overlap * ball1->Mass / totalMass);

                        // 탄성 충돌 속도 계산
                        // 상대 속도
                        FVector relativeVelocity = ball1->Velocity - ball2->Velocity;

                        // 법선 방향 속도
                        float velocityAlongNormal = relativeVelocity.Dot(normal);

                        // 이미 분리 중이면 충돌 처리 안함
                        if (velocityAlongNormal > 0)
                            continue;

                        // 반발 계수 (완전 탄성 충돌 = 1.0)
                        float restitution = 1.0f;
                        if(bRestitution)
                            restitution = (ball1->Restitution + ball2->Restitution) * 0.5f;

                        // 충격량 크기 계산
                        float impulseMagnitude = -(1 + restitution) * velocityAlongNormal;
                        impulseMagnitude /= (1 / ball1->Mass + 1 / ball2->Mass);

                        // 충격량 벡터
                        FVector impulse = normal * impulseMagnitude;

                        // 속도 업데이트
                        ball1->ApplyImpulse(impulse);
                        ball2->ApplyImpulse(impulse * (-1.0f));

                        // 충돌 횟수 증가
                        if (bHitDarkening) {
                            ball1->NumHits++;
                            ball2->NumHits++;
                        }
                    }
                }
            }
        }

		////////////////////////////////////////////
		// 매번 실행되는 코드를 여기에 추가합니다.

        // 준비 작업
        renderer.Prepare();
        renderer.PrepareShader();

        // ImGui
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 이후 ImGui UI 컨트롤 추가는 ImGui::NewFrame()과 ImGui::Render() 사이인 여기에 위치합니다.
        ImGui::Begin("Jungle Property Window");
        ImGui::Text("Hello Jungle World!");

        ImGui::Checkbox("Gravity", &bGravity);
        ImGui::Checkbox("Angular Velocity", &bAngularVelocity); // 각속도
        ImGui::Checkbox("Restitution", &bRestitution);          // 탄성
        ImGui::Checkbox("Darken on Hits", &bHitDarkening);      // 충돌 시 어두워짐

        int DesiredNumBalls = UBall::TotalNumBalls;
        if (ImGui::InputInt("Number of Balls", &DesiredNumBalls, 1, 100))
        {
            // 입력값을 1 이상으로 제한
            if (DesiredNumBalls < 1)
            {
                DesiredNumBalls = 1;
            }

            // 공 개수가 증가한 경우
            if (DesiredNumBalls > UBall::TotalNumBalls)
            {
                int NumToCreate = DesiredNumBalls - UBall::TotalNumBalls;

                // 배열 크기가 부족하면 확장
                if (DesiredNumBalls > CurrentCapacity)
                {
                    int NewCapacity = DesiredNumBalls + 10; // 여유 공간 확보
                    UPrimitive** NewList = new UPrimitive * [NewCapacity];

                    // 기존 데이터 복사
                    for (int i = 0; i < UBall::TotalNumBalls; ++i)
                    {
                        NewList[i] = PrimitiveList[i];
                    }

                    // 기존 배열 삭제
                    if (PrimitiveList != nullptr)
                    {
                        delete[] PrimitiveList;
                    }

                    PrimitiveList = NewList;
                    CurrentCapacity = NewCapacity;
                }

                // 새 공 생성
                for (int i = 0; i < NumToCreate; ++i)
                {
                    UBall* NewBall = new UBall();
                    PrimitiveList[UBall::TotalNumBalls - 1] = NewBall;
                }
            }
            // 공 개수가 감소한 경우
            else if (DesiredNumBalls < UBall::TotalNumBalls)
            {
                int NumToDelete = UBall::TotalNumBalls - DesiredNumBalls;

                for (int i = 0; i < NumToDelete; ++i)
                {
                    if (UBall::TotalNumBalls > 0)
                    {
                        // 랜덤하게 공 선택하여 삭제
                        int RandomIndex = rand() % UBall::TotalNumBalls;

                        // 선택된 공 삭제
                        delete PrimitiveList[RandomIndex];
                        PrimitiveList[RandomIndex] = nullptr;

                        // 배열에서 제거 (마지막 요소를 빈 자리로 이동)
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

        // 모든 공 렌더링
        for (int i = 0; i < UBall::TotalNumBalls; ++i)
        {
            if (PrimitiveList[i] != nullptr)
            {
                PrimitiveList[i]->Render(renderer, bHitDarkening);
                renderer.RenderPrimitive(vertexBufferSphere, numVerticesSphere);
            }
        }

        // 현재 화면에 보여지는 버퍼와 그리기 작업을 위한 버퍼를 서로 교환합니다.
        renderer.SwapBuffer();

        // 모든 환경에서 FPS 맞추기 위함
        do
        {
            Sleep(0);

            // 루프 종료 시간 기록
            QueryPerformanceCounter(&endTime);

            // 한 프레임이 소요된 시간 계산 (밀리초 단위로 변환)
            elapsedTime = (endTime.QuadPart - startTime.QuadPart) * 1000.0 / frequency.QuadPart;

        } while (elapsedTime < targetFrameTime);
		////////////////////////////////////////////
	}

    // 프로그램 종료 시 모든 공 메모리 해제
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

    // 여기에서 ImGui 소멸
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // D3D11 소멸 시키는 함수를 호출합니다.
    renderer.ReleaseVertexBuffer(vertexBufferSphere);

    renderer.ReleaseConstantBuffer();
    renderer.ReleaseShader();
    renderer.Release();

    return 0;
}