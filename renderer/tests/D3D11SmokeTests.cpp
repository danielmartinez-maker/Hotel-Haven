#include "TestFramework.h"

#include "d3d11/D3D11Renderer.h"
#include "hh/assets/Hasset.h"
#include "hh/renderer/RuntimeAssetRegistry.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace {

void appendU32(std::vector<std::byte>& bytes, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32u; shift += 8u) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffu));
    }
}

template <typename T>
void appendValue(std::vector<std::byte>& bytes, const T& value) {
    const auto* first = reinterpret_cast<const std::byte*>(&value);
    bytes.insert(bytes.end(), first, first + sizeof(T));
}

std::vector<std::byte> makeTriangleGlb() {
    std::vector<std::byte> binary;
    const std::array<float, 9> positions{
        -0.5f, 0.0f, 0.0f,
         0.5f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f,
    };
    const std::array<float, 9> normals{
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
    };
    for (const float value : positions) appendValue(binary, value);
    const std::size_t normalOffset = binary.size();
    for (const float value : normals) appendValue(binary, value);
    const std::size_t indexOffset = binary.size();
    const std::array<std::uint16_t, 3> indices{0u, 1u, 2u};
    for (const auto value : indices) appendValue(binary, value);
    while ((binary.size() % 4u) != 0u) binary.push_back(std::byte{0});

    std::string json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"buffers\":[{\"byteLength\":" + std::to_string(binary.size()) + "}],"
        "\"bufferViews\":["
          "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
          "{\"buffer\":0,\"byteOffset\":" + std::to_string(normalOffset) + ",\"byteLength\":36},"
          "{\"buffer\":0,\"byteOffset\":" + std::to_string(indexOffset) + ",\"byteLength\":6}"
        "],"
        "\"accessors\":["
          "{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
          "{\"bufferView\":1,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
          "{\"bufferView\":2,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}"
        "],"
        "\"materials\":[{\"pbrMetallicRoughness\":{\"baseColorFactor\":[0.8,0.5,0.2,1.0]}}],"
        "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0,\"NORMAL\":1},\"indices\":2,\"material\":0}]}],"
        "\"nodes\":[{\"mesh\":0}],"
        "\"scenes\":[{\"nodes\":[0]}],\"scene\":0"
        "}";
    while ((json.size() % 4u) != 0u) json.push_back(' ');

    const auto jsonLength = static_cast<std::uint32_t>(json.size());
    const auto binaryLength = static_cast<std::uint32_t>(binary.size());
    std::vector<std::byte> glb;
    appendU32(glb, 0x46546c67u);
    appendU32(glb, 2u);
    appendU32(glb, 12u + 8u + jsonLength + 8u + binaryLength);
    appendU32(glb, jsonLength);
    appendU32(glb, 0x4e4f534au);
    for (const char c : json) glb.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
    appendU32(glb, binaryLength);
    appendU32(glb, 0x004e4942u);
    glb.insert(glb.end(), binary.begin(), binary.end());
    return glb;
}

std::vector<std::byte> makeTriangleHasset() {
    hh::assets::HassetDocument document;
    document.type = hh::assets::AssetType::StaticMesh;
    document.asset_id = "HH_A001";
    document.fingerprint = "warp-smoke";
    document.source_path = "Art/Exports/HH_A001.glb";
    document.sidecar_path = "Art/Exports/HH_A001.asset.json";
    document.payload = makeTriangleGlb();
    return hh::assets::serialize_hasset(document);
}

LRESULT CALLBACK smokeWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    return DefWindowProcW(window, message, wParam, lParam);
}

class SmokeWindow {
public:
    SmokeWindow() {
        instance_ = GetModuleHandleW(nullptr);
        className_ = L"HotelHavenRendererSmokeWindow";
        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = smokeWindowProc;
        windowClass.hInstance = instance_;
        windowClass.lpszClassName = className_;
        atom_ = RegisterClassW(&windowClass);
        if (atom_ == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return;
        }
        window_ = CreateWindowExW(
            0, className_, L"", WS_OVERLAPPEDWINDOW,
            0, 0, 64, 64, nullptr, nullptr, instance_, nullptr);
    }

    ~SmokeWindow() {
        if (window_ != nullptr) DestroyWindow(window_);
        if (atom_ != 0) UnregisterClassW(className_, instance_);
    }

    [[nodiscard]] HWND get() const noexcept { return window_; }

private:
    HINSTANCE instance_{};
    LPCWSTR className_{};
    ATOM atom_{};
    HWND window_{};
};

std::filesystem::path shaderPath(std::string_view fileName) {
    return std::filesystem::path(__FILE__).parent_path().parent_path() /
        "shaders" / fileName;
}

}  // namespace

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

    const std::filesystem::path boxShaderPath = shaderPath("InstancedBox.hlsl");

    Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
    Microsoft::WRL::ComPtr<ID3DBlob> vertexErrors;
    const HRESULT vertexResult = D3DCompileFromFile(
        boxShaderPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "VSMain", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0,
        vertexShader.GetAddressOf(), vertexErrors.GetAddressOf());
    EXPECT_TRUE(SUCCEEDED(vertexResult));
    EXPECT_TRUE(vertexShader != nullptr);

    Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;
    Microsoft::WRL::ComPtr<ID3DBlob> pixelErrors;
    const HRESULT pixelResult = D3DCompileFromFile(
        boxShaderPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "PSMain", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0,
        pixelShader.GetAddressOf(), pixelErrors.GetAddressOf());
    EXPECT_TRUE(SUCCEEDED(pixelResult));
    EXPECT_TRUE(pixelShader != nullptr);
}

TEST_CASE("D3D11 WARP renderer caches and submits a cooked mesh") {
    using namespace hh::renderer;

    SmokeWindow window;
    EXPECT_TRUE(window.get() != nullptr);

    RuntimeAssetRegistry registry;
    const AssetHandle handle = registry.addHasset(makeTriangleHasset());

    D3D11Renderer renderer(&registry);
    const RendererResult initializeResult = renderer.initialize(
        window.get(), 64u, 64u, shaderPath("InstancedBox.hlsl"), true);
    EXPECT_TRUE(initializeResult);

    OrthoCamera camera;
    camera.setAspectRatio(1.0f);
    camera.setOrthoHeight(10.0f);

    ComposedScene scene;
    MeshRenderItem item{
        handle,
        MeshTransform{Vec3{0.0f, 0.0f, 0.0f}, Vec3{1.0f, 1.0f, 1.0f}, 0.0f},
        Color{1.0f, 1.0f, 1.0f, 1.0f},
        0,
        RenderCategory::Object,
        Aabb{{-0.5f, 0.0f, -0.1f}, {0.5f, 1.0f, 0.1f}},
        false
    };
    scene.opaqueMeshes.push_back(ComposedMesh{item, false});

    const RendererResult renderResult = renderer.render(scene, camera);
    EXPECT_TRUE(renderResult);
    const RendererStats stats = renderer.stats();
    EXPECT_EQ(stats.cachedMeshes, 1u);
    EXPECT_EQ(stats.meshDrawCalls, 1u);

    renderer.shutdown();
}
