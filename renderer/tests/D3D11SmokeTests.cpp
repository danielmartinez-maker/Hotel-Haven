#include "TestFramework.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <filesystem>

TEST_CASE("D3D11 WARP device and production shader compile") {
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL featureLevel{};

    const HRESULT deviceResult = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_WARP,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        device.GetAddressOf(),
        &featureLevel,
        context.GetAddressOf());
    EXPECT_TRUE(SUCCEEDED(deviceResult));
    EXPECT_TRUE(device != nullptr);
    EXPECT_TRUE(context != nullptr);

    const std::filesystem::path shaderPath =
        std::filesystem::path(__FILE__).parent_path().parent_path() /
        "shaders" / "InstancedBox.hlsl";

    Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
    Microsoft::WRL::ComPtr<ID3DBlob> vertexErrors;
    const HRESULT vertexResult = D3DCompileFromFile(
        shaderPath.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "VSMain",
        "vs_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        vertexShader.GetAddressOf(),
        vertexErrors.GetAddressOf());
    EXPECT_TRUE(SUCCEEDED(vertexResult));
    EXPECT_TRUE(vertexShader != nullptr);

    Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;
    Microsoft::WRL::ComPtr<ID3DBlob> pixelErrors;
    const HRESULT pixelResult = D3DCompileFromFile(
        shaderPath.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "PSMain",
        "ps_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        pixelShader.GetAddressOf(),
        pixelErrors.GetAddressOf());
    EXPECT_TRUE(SUCCEEDED(pixelResult));
    EXPECT_TRUE(pixelShader != nullptr);
}
