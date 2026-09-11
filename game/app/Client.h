#pragma once
#include "GameUiBridge.h"
#include "GameUiCommandRouter.h"
#include "InGameUiContract.h"
#include "RuntimeWorldAssets.h"
#include "d3d11/D3D11Renderer.h"
#include "hh/frontend/AlertCenter.h"
#include "hh/frontend/EconomyDashboard.h"
#include "hh/frontend/GameHudController.h"
#include "hh/frontend/GameUiRuntime.h"
#include "hh/frontend/ObjectiveUi.h"
#include "hh/frontend/OperationsDashboard.h"
#include "hh/frontend/UiSettings.h"
#include "hh/game/SimulationEconomyBridge.h"
#include "hh/renderer/Camera.h"
#include "hh/renderer/RenderScene.h"
#include <array>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>
#include <windows.h>

#ifdef small
#undef small
#endif
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif

namespace hh::client {
using Simulation = hh::game::SimulationEconomyBridge;

inline int HeaderHeight = 88;
inline int FooterHeight = 58;
inline int SidebarWidth = 356;
inline void applyClientUiScale(int scalePercent) noexcept {
  HeaderHeight = final07ScalePixel(88, scalePercent);
  FooterHeight = final07ScalePixel(58, scalePercent);
  SidebarWidth = final07ScalePixel(356, scalePercent);
}

enum class Tool {
  Inspect,
  Bedroom,
  Floor,
  Wall,
  Door,
  Entrance,
  Desk,
  Closet,
  Stairs,
  Erase,
  Bathroom,
  StaffRoom,
  Lobby
};
struct Button {
  RECT rect{};
  std::wstring label;
  std::function<void()> action;
  bool active{};
};
class ClientRenderer final : public hh::renderer::D3D11Renderer {
public:
  explicit ClientRenderer(hh::renderer::RuntimeAssetRegistry &registry) noexcept
      : registry_(&registry) {}
  [[nodiscard]] hh::renderer::RendererResult
  initialize(HWND window, std::uint32_t width, std::uint32_t height,
             const std::filesystem::path &shaderPath,
             bool softwareDevice = false) {
    auto result = hh::renderer::D3D11Renderer::initialize(
        window, width, height, shaderPath, softwareDevice);
    if (result)
      setAssetRegistry(registry_);
    return result;
  }
private:
  hh::renderer::RuntimeAssetRegistry *registry_{};
};
struct Client {
  Simulation simulation;
  hh::game::SimulationView snapshot;
  hh::frontend::GameHudModel hudModel;
  hh::frontend::GameHudController hudController;
  hh::frontend::GameUiRuntime ui;
  hh::frontend::AlertCenter alertCenter{200};
  hh::frontend::ObjectiveUi objectiveUi;
  hh::frontend::OperationsDashboard operationsDashboard;
  hh::frontend::EconomyDashboard economyDashboard;
  hh::frontend::UiSettings uiSettings;
  hh::frontend::BuildPlacementPreview buildPreview;
  bool uiConfigured{};
  std::uint64_t previewRequestSerial{};
  std::uint64_t selectedAlertId{};
  std::size_t financeRuleIndex{};
  std::size_t financeOverbookingIndex{};
  hh::frontend::OverlayId managementOverlay{hh::frontend::OverlayId::None};
  HWND window{}, viewport{};
  hh::renderer::RuntimeAssetRegistry assetRegistry;
  ClientRenderer renderer{assetRegistry};
  WorldAssetSet worldAssets;
  hh::renderer::OrthoCamera camera;
  std::filesystem::path directory, savePath;
  std::vector<Button> buttons;
  std::array<bool, 256> keys{};
  unsigned short gamepadButtons{};
  int width = 1500, height = 960, floor = 0, speed = 0, priorSpeed = 1,
      tabScroll = 0, operationsFilter = 0;
  int hoverX = -1, hoverY = -1;
  int focusedButton = -1;
  double pendingSimulationSeconds = 0;
  hh::game::EntityId selected{};
  Page page = Page::Guide;
  Tool tool = Tool::Inspect;
  Tool previewTool = Tool::Inspect;
  bool previewValid = false;
  Overlay overlay = Overlay::Natural;
  hh::renderer::WallRenderMode wallMode = hh::renderer::WallRenderMode::Cutaway;
  bool context = false, running = true, smoke = false, initialized = false;
  std::string fatalError;
  std::wstring notice = L"Welcome. Open the guide to begin your first hotel.";
  HFONT normal{}, small{}, title{}, number{};
  Client();
  ~Client();
  void refresh();
  void refreshUi();
  void layout();
  void paint(HDC);
  void click(int, int);
  void mapClick(int, int);
  void hover(int, int);
  void key(int);
  void changeFloor(int);
  bool save();
  bool load();
  void result(const hh::game::CommandResult &);
  void newCampaign();
  [[nodiscard]] hh::frontend::UiCommandResult
  dispatchUiCommand(const hh::frontend::UiCommand &command);
};
[[nodiscard]] std::string toolName(Tool tool);
[[nodiscard]] hh::game::CommandResult
applyBuildTool(Simulation &simulation, Tool tool,
               hh::game::Position position, std::size_t roomCount);
std::wstring wide(const std::string &);
std::wstring money(std::int64_t);
std::wstring roomStatus(hh::game::RoomStatus);
std::wstring personState(hh::game::PersonState);
} // namespace hh::client
