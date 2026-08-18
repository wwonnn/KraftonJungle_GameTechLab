#pragma once

#include <d3d11.h>
#include <memory>

class FVertexShader;
class FPixelShader;

class ENGINE_API CShaderManager
{
public:
    CShaderManager() = default;
    ~CShaderManager();

    bool LoadVertexShader(ID3D11Device* Device, const FWString FilePath, const FWString EntryPoint);
    bool LoadPixelShader(ID3D11Device* Device, const FWString FilePath, const FWString EntryPoint);
    void Bind(ID3D11DeviceContext* DeviceContext);
    void Release();

private:
    std::shared_ptr<FVertexShader> VS;
    std::shared_ptr<FPixelShader>  PS;
};