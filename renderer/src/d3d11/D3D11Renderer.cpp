#include "d3d11/D3D11Renderer.h"

#include <d3dcompiler.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string_view>

namespace hh::renderer {
namespace {

constexpr std::size_t kMaxInstancesPerDraw = 16'384;
constexpr UINT kCubeIndexCount = 36;
constexpr float kCutawayHeightFactor = 0.35f;

std::string hresultError(std::string_view operation, HRESULT result) {
    std::ostringstream stream;
    stream << operation << " failed (HRESULT 0x" << std::hex << std::uppercase
           << static_cast<unsigned long>(result) << ')';
    return stream.str();
}

RendererResult compileShader(
    const std::filesystem::path& path,
    const char* entryPoint,
    const char* target,
    Microsoft::WRL::ComPtr<ID3DBlob>& output) {
    Microsoft::WRL::ComPtr<ID3DBlob> diagnostics;
    const HRESULT result = D3DCompileFromFile(
        path.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint,
        target,
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        output.GetAddressOf(),
        diagnostics.GetAddressOf());

    if (SUCCEEDED(result)) {
        return RendererResult::success();
    }

    std::string message = hresultError(std::string("D3DCompileFromFile(") + entryPoint + ')', result);
    if (diagnostics != nullptr && diagnostics->GetBufferPointer() != nullptr) {
        message += ": ";
        message.append(
            static_cast<const char*>(diagnostics->GetBufferPointer()),
            diagnostics->GetBufferSize());
    }
    return RendererResult::failure(std::move(message));
}

}  // namespace

struct D3D11Renderer::Vertex {
    DirectX::XMFLOAT3 position;
};

struct D3D11Renderer::InstanceData {
    DirectX::XMFLOAT4 world0;
    DirectX::XMFLOAT4 world1;
    DirectX::XMFLOAT4 world2;
    DirectX::XMFLOAT4 world3;
    DirectX::XMFLOAT4 color;
};

struct D3D11Renderer::CameraConstants {
    DirectX::XMFLOAT4X4 viewProjection;
};

D3D11Renderer::~D3D11Renderer() {
    shutdown();
}

RendererResult D3D11Renderer::initialize(
    HWND window,
    std::uint32_t width,
    std::uint32_t height,
    const std::filesystem::path& shaderPath) {
    shutdown();

    DXGI_SWAP_CHAIN_DESC swapDescription{};
    swapDescription.BufferDesc.Width = std::max(width, 1u);
    swapDescription.BufferDesc.Height = std::max(height, 1u);
    swapDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapDescription.BufferDesc.RefreshRate.Numerator = 0;
    swapDescription.BufferDesc.RefreshRate.Denominator = 1;
    swapDescription.SampleDesc.Count = 1;
    swapDescription.SampleDesc.Quality = 0;
    swapDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDescription.BufferCount = 2;
    swapDescription.OutputWindow = window;
    swapDescription.Windowed = TRUE;
    swapDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const HRESULT deviceResult = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &swapDescription,
        swapChain_.GetAddressOf(),
        device_.GetAddressOf(),
        nullptr,
        context_.GetAddressOf());

    if (FAILED(deviceResult)) {
        return RendererResult::failure(hresultError("D3D11CreateDeviceAndSwapChain", deviceResult));
    }

    RendererResult result = createGeometryResources();
    if (!result) {
        shutdown();
        return result;
    }

    result = createShaderResources(shaderPath);
    if (!result) {
        shutdown();
        return result;
    }

    result = createPipelineStates();
    if (!result) {
        shutdown();
        return result;
    }

    result = createSizeDependentResources(width, height);
    if (!result) {
        shutdown();
        return result;
    }

    return RendererResult::success();
}

RendererResult D3D11Renderer::createSizeDependentResources(
    std::uint32_t width,
    std::uint32_t height) {
    width_ = width;
    height_ = height;

    if (width == 0 || height == 0) {
        return RendererResult::success();
    }

    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    const HRESULT bufferResult = swapChain_->GetBuffer(
        0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
    if (FAILED(bufferResult)) {
        return RendererResult::failure(hresultError("IDXGISwapChain::GetBuffer", bufferResult));
    }

    const HRESULT targetResult = device_->CreateRenderTargetView(
        backBuffer.Get(), nullptr, renderTargetView_.GetAddressOf());
    if (FAILED(targetResult)) {
        return RendererResult::failure(hresultError("ID3D11Device::CreateRenderTargetView", targetResult));
    }

    D3D11_TEXTURE2D_DESC depthDescription{};
    depthDescription.Width = width;
    depthDescription.Height = height;
    depthDescription.MipLevels = 1;
    depthDescription.ArraySize = 1;
    depthDescription.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDescription.SampleDesc.Count = 1;
    depthDescription.SampleDesc.Quality = 0;
    depthDescription.Usage = D3D11_USAGE_DEFAULT;
    depthDescription.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    const HRESULT depthTextureResult = device_->CreateTexture2D(
        &depthDescription, nullptr, depthTexture_.GetAddressOf());
    if (FAILED(depthTextureResult)) {
        return RendererResult::failure(hresultError("ID3D11Device::CreateTexture2D(depth)", depthTextureResult));
    }

    const HRESULT depthViewResult = device_->CreateDepthStencilView(
        depthTexture_.Get(), nullptr, depthStencilView_.GetAddressOf());
    if (FAILED(depthViewResult)) {
        return RendererResult::failure(hresultError("ID3D11Device::CreateDepthStencilView", depthViewResult));
    }

    viewport_.TopLeftX = 0.0f;
    viewport_.TopLeftY = 0.0f;
    viewport_.Width = static_cast<float>(width);
    viewport_.Height = static_cast<float>(height);
    viewport_.MinDepth = 0.0f;
    viewport_.MaxDepth = 1.0f;
    return RendererResult::success();
}

RendererResult D3D11Renderer::resize(std::uint32_t width, std::uint32_t height) {
    if (device_ == nullptr || swapChain_ == nullptr) {
        return RendererResult::failure("resize called before renderer initialization");
    }

    if (context_ != nullptr) {
        context_->OMSetRenderTargets(0, nullptr, nullptr);
    }
    renderTargetView_.Reset();
    depthStencilView_.Reset();
    depthTexture_.Reset();
    width_ = width;
    height_ = height;

    if (width == 0 || height == 0) {
        return RendererResult::success();
    }

    const HRESULT resizeResult = swapChain_->ResizeBuffers(
        0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(resizeResult)) {
        return RendererResult::failure(hresultError("IDXGISwapChain::ResizeBuffers", resizeResult));
    }

    return createSizeDependentResources(width, height);
}

RendererResult D3D11Renderer::createGeometryResources() {
    const std::array<Vertex, 8> vertices{{
        {{-0.5f, -0.5f, -0.5f}},
        {{-0.5f,  0.5f, -0.5f}},
        {{ 0.5f,  0.5f, -0.5f}},
        {{ 0.5f, -0.5f, -0.5f}},
        {{-0.5f, -0.5f,  0.5f}},
        {{-0.5f,  0.5f,  0.5f}},
        {{ 0.5f,  0.5f,  0.5f}},
        {{ 0.5f, -0.5f,  0.5f}},
    }};

    const std::array<std::uint16_t, kCubeIndexCount> indices{{
        0, 1, 2, 0, 2, 3,
        4, 7, 6, 4, 6, 5,
        4, 5, 1, 4, 1, 0,
        3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2,
        4, 0, 3, 4, 3, 7,
    }};

    D3D11_BUFFER_DESC vertexDescription{};
    vertexDescription.ByteWidth = static_cast<UINT>(sizeof(vertices));
    vertexDescription.Usage = D3D11_USAGE_IMMUTABLE;
    vertexDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vertexData{};
    vertexData.pSysMem = vertices.data();
    const HRESULT vertexResult = device_->CreateBuffer(
        &vertexDescription, &vertexData, cubeVertexBuffer_.GetAddressOf());
    if (FAILED(vertexResult)) {
        return RendererResult::failure(hresultError("ID3D11Device::CreateBuffer(cube vertex)", vertexResult));
    }

    D3D11_BUFFER_DESC indexDescription{};
    indexDescription.ByteWidth = static_cast<UINT>(sizeof(indices));
    indexDescription.Usage = D3D11_USAGE_IMMUTABLE;
    indexDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA indexData{};
    indexData.pSysMem = indices.data();
    const HRESULT indexResult = device_->CreateBuffer(
        &indexDescription, &indexData, cubeIndexBuffer_.GetAddressOf());
    if (FAILED(indexResult)) {
        return RendererResult::failure(hresultError("ID3D11Device::CreateBuffer(cube index)", indexResult));
    }

    D3D11_BUFFER_DESC instanceDescription{};
    instanceDescription.ByteWidth = static_cast<UINT>(sizeof(InstanceData) * kMaxInstancesPerDraw);
    instanceDescription.Usage = D3D11_USAGE_DYNAMIC;
    instanceDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    instanceDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    const HRESULT instanceResult = device_->CreateBuffer(
        &instanceDescription, nullptr, instanceBuffer_.GetAddressOf());
    if (FAILED(instanceResult)) {
        return RendererResult::failure(hresultError("ID3D11Device::CreateBuffer(instance)", instanceResult));
    }

    D3D11_BUFFER_DESC cameraDescription{};
    cameraDescription.ByteWidth = static_cast<UINT>(sizeof(CameraConstants));
    cameraDescription.Usage = D3D11_USAGE_DYNAMIC;
    cameraDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cameraDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    const HRESULT cameraResult = device_->CreateBuffer(
        &cameraDescription, nullptr, cameraConstantBuffer_.GetAddressOf());
    if (FAILED(cameraResult)) {
        return RendererResult::failure(hresultError("ID3D11Device::CreateBuffer(camera constants)", cameraResult));
    }

    return RendererResult::success();
}

RendererResult D3D11Renderer::createShaderResources(const std::filesystem::path& shaderPath) {
    Microsoft::WRL::ComPtr<ID3DBlob> vertexBytecode;
    RendererResult result = compileShader(shaderPath, "VSMain", "vs_5_0", vertexBytecode);
    if (!result) {
        return result;
    }

    Microsoft::WRL::ComPtr<ID3DBlob> pixelBytecode;
    result = compileShader(shaderPath, "PSMain", "ps_5_0", pixelBytecode);
    if (!result) {
        return result;
    }

    HRESULT createResult = device_->CreateVertexShader(
        vertexBytecode->GetBufferPointer(),
        vertexBytecode->GetBufferSize(),
        nullptr,
        vertexShader_.GetAddressOf());
    if (FAILED(createResult)) {
        return RendererResult::failure(hresultError("ID3D11Device::CreateVertexShader", createResult));
    }

    createResult = device_->CreatePixelShader(
        pixelBytecode->GetBufferPointer(),
        pixelBytecode->GetBufferSize(),
        nullptr,
        pixelShader_.GetAddressOf());
    if (FAILED(createResult)) {
        return RendererResult::failure(hresultError("ID3D11Device::CreatePixelShader", createResult));
    }

    const std::array<D3D11_INPUT_ELEMENT_DESC, 6> layout{{
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, static_cast<UINT>(offsetof(InstanceData, world0)), D3D11_INPUT_PER_INSTANCE_DATA, 1},
        {"WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, static_cast<UINT>(offsetof(InstanceData, world1)), D3D11_INPUT_PER_INSTANCE_DATA, 1},
        {"WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, static_cast<UINT>(offsetof(InstanceData, world2)), D3D11_INPUT_PER_INSTANCE_DATA, 1},
        {"WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, static_cast<UINT>(offsetof(InstanceData, world3)), D3D11_INPUT_PER_INSTANCE_DATA, 1},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, static_cast<UINT>(offsetof(InstanceData, color)), D3D11_INPUT_PER_INSTANCE_DATA, 1},
    }};

    createResult = device_->CreateInputLayout(
        layout.data(),
        static_cast<UINT>(layout.size()),
        vertexBytecode->GetBufferPointer(),
        vertexBytecode->GetBufferSize(),
        inputLayout_.GetAddressOf());
    if (FAILED(createResult)) {
        return RendererResult::failure(hresultError("ID3D11Device::CreateInputLayout", createResult));
    }

    return RendererResult::success();
}

RendererResult D3D11Renderer::createPipelineStates() {
    D3D11_RASTERIZER_DESC solidDescription{};
    solidDescription.FillMode = D3D11_FILL_SOLID;
    solidDescription.CullMode = D3D11_CULL_BACK;
    solidDescription.DepthClipEnable = TRUE;
    HRESULT result = device_->CreateRasterizerState(
        &solidDescription, solidRasterizer_.GetAddressOf());
    if (FAILED(result)) {
        return RendererResult::failure(hresultError("ID3D11Device::CreateRasterizerState(solid)", result));
    }

    D3D11_RASTERIZER_DESC wireDescription = solidDescription;
    wireDescription.FillMode = D3D11_FILL_WIREFRAME;
    wireDescription.CullMode = D3D11_CULL_NONE;
    result = device_->CreateRasterizerState(
        &wireDescription, wireframeRasterizer_.GetAddressOf());
    if (FAILED(result)) {
        return RendererResult::failure(hresultError("ID3D11Device::CreateRasterizerState(wireframe)", result));
    }

    D3D11_BLEND_DESC opaqueDescription{};
    opaqueDescription.RenderTarget[0].BlendEnable = FALSE;
    opaqueDescription.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    result = device_->CreateBlendState(&opaqueDescription, opaqueBlend_.GetAddressOf());
    if (FAILED(result)) {
        return RendererResult::failure(hresultError("ID3D11Device::CreateBlendState(opaque)", result));
    }

    D3D11_BLEND_DESC alphaDescription = opaqueDescription;
    alphaDescription.RenderTarget[0].BlendEnable = TRUE;
    alphaDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    alphaDescription.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    alphaDescription.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    alphaDescription.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    alphaDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    alphaDescription.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    result = device_->CreateBlendState(&alphaDescription, alphaBlend_.GetAddressOf());
    if (FAILED(result)) {
        return RendererResult::failure(hresultError("ID3D11Device::CreateBlendState(alpha)", result));
    }

    D3D11_DEPTH_STENCIL_DESC depthWriteDescription{};
    depthWriteDescription.DepthEnable = TRUE;
    depthWriteDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthWriteDescription.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    result = device_->CreateDepthStencilState(
        &depthWriteDescription, depthWriteState_.GetAddressOf());
    if (FAILED(result)) {
        return RendererResult::failure(hresultError("ID3D11Device::CreateDepthStencilState(write)", result));
    }

    D3D11_DEPTH_STENCIL_DESC depthReadDescription = depthWriteDescription;
    depthReadDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    result = device_->CreateDepthStencilState(
        &depthReadDescription, depthReadState_.GetAddressOf());
    if (FAILED(result)) {
        return RendererResult::failure(hresultError("ID3D11Device::CreateDepthStencilState(read)", result));
    }

    return RendererResult::success();
}

RendererResult D3D11Renderer::drawBatch(
    const std::vector<ComposedBox>& boxes,
    bool wireframe,
    bool alphaBlend) {
    if (boxes.empty()) {
        return RendererResult::success();
    }

    context_->RSSetState(wireframe ? wireframeRasterizer_.Get() : solidRasterizer_.Get());
    context_->OMSetBlendState(
        alphaBlend ? alphaBlend_.Get() : opaqueBlend_.Get(), nullptr, 0xFFFFFFFFu);
    context_->OMSetDepthStencilState(
        alphaBlend ? depthReadState_.Get() : depthWriteState_.Get(), 0);

    ID3D11Buffer* vertexBuffers[] = {cubeVertexBuffer_.Get(), instanceBuffer_.Get()};
    const UINT strides[] = {
        static_cast<UINT>(sizeof(Vertex)),
        static_cast<UINT>(sizeof(InstanceData)),
    };
    const UINT offsets[] = {0, 0};
    context_->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);

    std::size_t batchOffset = 0;
    while (batchOffset < boxes.size()) {
        const std::size_t count = std::min(kMaxInstancesPerDraw, boxes.size() - batchOffset);
        D3D11_MAPPED_SUBRESOURCE mapped{};
        const HRESULT mapResult = context_->Map(
            instanceBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(mapResult)) {
            return RendererResult::failure(hresultError("ID3D11DeviceContext::Map(instance)", mapResult));
        }

        auto* destination = static_cast<InstanceData*>(mapped.pData);
        for (std::size_t index = 0; index < count; ++index) {
            const ComposedBox& box = boxes[batchOffset + index];
            float sizeY = box.item.size.y;
            float centerY = box.item.center.y;
            if (box.cutaway) {
                const float baseY = centerY - sizeY * 0.5f;
                sizeY *= kCutawayHeightFactor;
                centerY = baseY + sizeY * 0.5f;
            }

            const DirectX::XMMATRIX world = DirectX::XMMatrixMultiply(
                DirectX::XMMatrixScaling(box.item.size.x, sizeY, box.item.size.z),
                DirectX::XMMatrixTranslation(box.item.center.x, centerY, box.item.center.z));
            DirectX::XMFLOAT4X4 matrix{};
            DirectX::XMStoreFloat4x4(&matrix, world);

            destination[index].world0 = DirectX::XMFLOAT4(matrix._11, matrix._12, matrix._13, matrix._14);
            destination[index].world1 = DirectX::XMFLOAT4(matrix._21, matrix._22, matrix._23, matrix._24);
            destination[index].world2 = DirectX::XMFLOAT4(matrix._31, matrix._32, matrix._33, matrix._34);
            destination[index].world3 = DirectX::XMFLOAT4(matrix._41, matrix._42, matrix._43, matrix._44);
            destination[index].color = DirectX::XMFLOAT4(
                box.item.color.r,
                box.item.color.g,
                box.item.color.b,
                box.item.color.a);
        }

        context_->Unmap(instanceBuffer_.Get(), 0);
        context_->DrawIndexedInstanced(
            kCubeIndexCount,
            static_cast<UINT>(count),
            0,
            0,
            0);
        batchOffset += count;
    }

    return RendererResult::success();
}

RendererResult D3D11Renderer::render(const ComposedScene& scene, const OrthoCamera& camera) {
    if (device_ == nullptr || context_ == nullptr || swapChain_ == nullptr) {
        return RendererResult::failure("render called before renderer initialization");
    }
    if (width_ == 0 || height_ == 0) {
        return RendererResult::success();
    }

    D3D11_MAPPED_SUBRESOURCE mapped{};
    const HRESULT mapResult = context_->Map(
        cameraConstantBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(mapResult)) {
        return RendererResult::failure(hresultError("ID3D11DeviceContext::Map(camera)", mapResult));
    }
    auto* constants = static_cast<CameraConstants*>(mapped.pData);
    DirectX::XMStoreFloat4x4(&constants->viewProjection, camera.viewProjectionMatrix());
    context_->Unmap(cameraConstantBuffer_.Get(), 0);

    constexpr float clearColor[4] = {0.075f, 0.085f, 0.10f, 1.0f};
    context_->ClearRenderTargetView(renderTargetView_.Get(), clearColor);
    context_->ClearDepthStencilView(
        depthStencilView_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    ID3D11RenderTargetView* renderTarget = renderTargetView_.Get();
    context_->OMSetRenderTargets(1, &renderTarget, depthStencilView_.Get());
    context_->RSSetViewports(1, &viewport_);
    context_->IASetInputLayout(inputLayout_.Get());
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->IASetIndexBuffer(cubeIndexBuffer_.Get(), DXGI_FORMAT_R16_UINT, 0);
    context_->VSSetShader(vertexShader_.Get(), nullptr, 0);
    context_->PSSetShader(pixelShader_.Get(), nullptr, 0);
    ID3D11Buffer* cameraBuffer = cameraConstantBuffer_.Get();
    context_->VSSetConstantBuffers(0, 1, &cameraBuffer);

    RendererResult result = drawBatch(scene.opaque, false, false);
    if (!result) {
        return result;
    }
    result = drawBatch(scene.translucent, false, true);
    if (!result) {
        return result;
    }
    result = drawBatch(scene.wireframe, true, true);
    if (!result) {
        return result;
    }

    const HRESULT presentResult = swapChain_->Present(1, 0);
    if (presentResult == DXGI_ERROR_DEVICE_REMOVED || presentResult == DXGI_ERROR_DEVICE_RESET) {
        const HRESULT reason = device_->GetDeviceRemovedReason();
        return RendererResult::failure(
            hresultError("IDXGISwapChain::Present(device lost)", reason));
    }
    if (FAILED(presentResult)) {
        return RendererResult::failure(hresultError("IDXGISwapChain::Present", presentResult));
    }

    return RendererResult::success();
}

void D3D11Renderer::shutdown() noexcept {
    if (context_ != nullptr) {
        context_->ClearState();
        context_->Flush();
    }

    depthReadState_.Reset();
    depthWriteState_.Reset();
    alphaBlend_.Reset();
    opaqueBlend_.Reset();
    wireframeRasterizer_.Reset();
    solidRasterizer_.Reset();
    inputLayout_.Reset();
    pixelShader_.Reset();
    vertexShader_.Reset();
    cameraConstantBuffer_.Reset();
    instanceBuffer_.Reset();
    cubeIndexBuffer_.Reset();
    cubeVertexBuffer_.Reset();
    depthStencilView_.Reset();
    depthTexture_.Reset();
    renderTargetView_.Reset();
    swapChain_.Reset();
    context_.Reset();
    device_.Reset();
    width_ = 0;
    height_ = 0;
    viewport_ = {};
}

}  // namespace hh::renderer
