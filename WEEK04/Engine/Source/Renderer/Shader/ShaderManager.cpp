#include "ShaderManager.h"
#include "ShaderMap.h"
#include "Shader.h"

CShaderManager::~CShaderManager()
{
    Release();
}

bool CShaderManager::LoadVertexShader(ID3D11Device* Device, const FWString FilePath, const FWString EntryPoint)
{
    VS = FShaderMap::Get().GetOrCreateVertexShader(Device, FilePath, EntryPoint);
    return VS != nullptr;
}

bool CShaderManager::LoadPixelShader(ID3D11Device* Device, const FWString FilePath, const FWString EntryPoint)
{
    PS = FShaderMap::Get().GetOrCreatePixelShader(Device, FilePath, EntryPoint);
    return PS != nullptr;
}

void CShaderManager::Bind(ID3D11DeviceContext* DeviceContext)
{
    if (VS)
        VS->Bind(DeviceContext);
    if (PS)
        PS->Bind(DeviceContext);
}

void CShaderManager::Release()
{
    VS.reset();
    PS.reset();
}