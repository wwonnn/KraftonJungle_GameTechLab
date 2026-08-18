#include "ShaderMap.h"
#include "Shader.h"
#include "ShaderResource.h"

FShaderMap& FShaderMap::Get()
{
    static FShaderMap Instance;
    return Instance;
}

std::shared_ptr<FVertexShader> FShaderMap::GetOrCreateVertexShader(
    ID3D11Device* Device,
    const FWString       FilePath,
    const FWString       EntryPoint
    )
{
    FWString Key;
    // Convert FString(a.k.a. std::string) to std::wstring
    Key.assign(FilePath.begin(), FilePath.end());

    auto It = VertexShaders.find(Key);
    if (It != VertexShaders.end())
    {
        return It->second;
    }

    auto Resource = FShaderResource::GetOrCompile(FilePath.c_str(), EntryPoint.c_str(), L"vs_5_0");
    if (!Resource)
    {
        return nullptr;
    }

    auto VS = FVertexShader::Create(Device, Resource);
    if (!VS)
    {
        return nullptr;
    }

    VertexShaders[Key] = VS;
    return VS;
}

std::shared_ptr<FPixelShader> FShaderMap::GetOrCreatePixelShader(
    ID3D11Device*  Device,
    const FWString FilePath,
    const FWString EntryPoint
    )
{
    std::wstring Key(FilePath);

    auto It = PixelShaders.find(Key);
    if (It != PixelShaders.end())
    {
        return It->second;
    }

    auto Resource = FShaderResource::GetOrCompile(FilePath, EntryPoint, L"ps_5_0");
    if (!Resource)
    {
        return nullptr;
    }

    auto PS = FPixelShader::Create(Device, Resource);
    if (!PS)
    {
        return nullptr;
    }

    PixelShaders[Key] = PS;
    return PS;
}

void FShaderMap::Clear()
{
    VertexShaders.clear();
    PixelShaders.clear();
    FShaderResource::ClearCache();
}