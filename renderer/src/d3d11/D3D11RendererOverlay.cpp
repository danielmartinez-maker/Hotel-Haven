#include "d3d11/D3D11Renderer.h"

#include <sstream>
#include <string_view>

namespace hh::renderer {
namespace {

struct OverlayCameraConstants {
    DirectX::XMFLOAT4X4 viewProjection;
};

std::string overlayHresultError(std::string_view operation, HRESULT result) {
    std::ostringstream stream;
    stream << operation << " failed (HRESULT 0x" << std::hex << std::uppercase
           << static_cast<unsigned long>(result) << ')';
    return stream.str();
}

}  // namespace

RendererResult D3D11Renderer::renderWorld(const ComposedScene& scene, const OrthoCamera& camera) {
    if (device_ == nullptr || context_ == nullptr || swapChain_ == nullptr) {
        return RendererResult::failure("renderWorld called before renderer initialization");
    }
    if (width_ == 0 || height_ == 0) {
        return RendererResult::success();
    }

    meshDrawCalls_ = 0;

    D3D11_MAPPED_SUBRESOURCE mapped{};
    const HRESULT mapResult = context_->Map(
        cameraConstantBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(mapResult)) {
        return RendererResult::failure(overlayHresultError("ID3D11DeviceContext::Map(camera)", mapResult));
    }
    auto* constants = static_cast<OverlayCameraConstants*>(mapped.pData);
    DirectX::XMStoreFloat4x4(&constants->viewProjection, camera.viewProjectionMatrix());
    context_->Unmap(cameraConstantBuffer_.Get(), 0);

    constexpr float clearColor[4] = {0.075f, 0.085f, 0.10f, 1.0f};
    context_->ClearRenderTargetView(renderTargetView_.Get(), clearColor);
    context_->ClearDepthStencilView(
        depthStencilView_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    ID3D11RenderTargetView* renderTarget = renderTargetView_.Get();
    context_->OMSetRenderTargets(1, &renderTarget, depthStencilView_.Get());
    context_->RSSetViewports(1, &viewport_);

    RendererResult result = drawBatch(scene.opaque, false, false);
    if (!result) {
        return result;
    }
    result = drawMeshBatch(scene.opaqueMeshes, false, false);
    if (!result) {
        return result;
    }
    result = drawBatch(scene.translucent, false, true);
    if (!result) {
        return result;
    }
    result = drawMeshBatch(scene.translucentMeshes, false, true);
    if (!result) {
        return result;
    }
    result = drawBatch(scene.wireframe, true, true);
    if (!result) {
        return result;
    }
    result = drawMeshBatch(scene.wireframeMeshes, true, true);
    if (!result) {
        return result;
    }

    return RendererResult::success();
}

RendererResult D3D11Renderer::present() {
    if (device_ == nullptr || swapChain_ == nullptr) {
        return RendererResult::failure("present called before renderer initialization");
    }
    if (width_ == 0 || height_ == 0) {
        return RendererResult::success();
    }

    const HRESULT presentResult = swapChain_->Present(1, 0);
    if (presentResult == DXGI_ERROR_DEVICE_REMOVED || presentResult == DXGI_ERROR_DEVICE_RESET) {
        const HRESULT reason = device_->GetDeviceRemovedReason();
        return RendererResult::failure(
            overlayHresultError("IDXGISwapChain::Present(device lost)", reason));
    }
    if (FAILED(presentResult)) {
        return RendererResult::failure(overlayHresultError("IDXGISwapChain::Present", presentResult));
    }
    return RendererResult::success();
}

}  // namespace hh::renderer
