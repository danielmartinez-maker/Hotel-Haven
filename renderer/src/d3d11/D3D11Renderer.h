#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <windows.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "hh/renderer/Camera.h"
#include "hh/renderer/RenderScene.h"

namespace hh::renderer {

struct RendererResult {
    bool succeeded{true};
    std::string error;

    [[nodiscard]] explicit operator bool() const noexcept { return succeeded; }
    [[nodiscard]] static RendererResult success() { return {}; }
    [[nodiscard]] static RendererResult failure(std::string message) {
        return RendererResult{false, std::move(message)};
    }
};

class D3D11Renderer {
public:
    D3D11Renderer() = default;
    ~D3D11Renderer();

    D3D11Renderer(const D3D11Renderer&) = delete;
    D3D11Renderer& operator=(const D3D11Renderer&) = delete;

    [[nodiscard]] RendererResult initialize(
        HWND window,
        std::uint32_t width,
        std::uint32_t height,
        const std::filesystem::path& shaderPath);
    [[nodiscard]] RendererResult resize(std::uint32_t width, std::uint32_t height);
    [[nodiscard]] RendererResult render(const ComposedScene& scene, const OrthoCamera& camera);
    void shutdown() noexcept;

private:
    struct Vertex;
    struct InstanceData;
    struct CameraConstants;

    [[nodiscard]] RendererResult createSizeDependentResources(std::uint32_t width, std::uint32_t height);
    [[nodiscard]] RendererResult createGeometryResources();
    [[nodiscard]] RendererResult createShaderResources(const std::filesystem::path& shaderPath);
    [[nodiscard]] RendererResult createPipelineStates();
    [[nodiscard]] RendererResult drawBatch(
        const std::vector<ComposedBox>& boxes,
        bool wireframe,
        bool alphaBlend);

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthTexture_;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> cubeVertexBuffer_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> cubeIndexBuffer_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> instanceBuffer_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> cameraConstantBuffer_;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> solidRasterizer_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> wireframeRasterizer_;
    Microsoft::WRL::ComPtr<ID3D11BlendState> opaqueBlend_;
    Microsoft::WRL::ComPtr<ID3D11BlendState> alphaBlend_;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthWriteState_;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthReadState_;
    D3D11_VIEWPORT viewport_{};
    std::uint32_t width_{};
    std::uint32_t height_{};
};

}  // namespace hh::renderer
