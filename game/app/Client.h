#pragma once
#include "SoundtrackPlayer.h"
#include "WorldView.h"
#include "d3d11/D3D11Renderer.h"
#include "hh/game/Simulation.h"
#include "hh/renderer/Camera.h"
#include "hh/renderer/RenderScene.h"
#include <array>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>
#include <windows.h>

namespace hh::client {
constexpr int HeaderHeight = 88, FooterHeight = 58, SidebarWidth = 356;
enum class Page { Build, Rooms, Staff, Guests, Supplies, Finance, Guide };
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
struct Client {
  hh::game::Simulation simulation;
  hh::game::SimulationView snapshot;
  HWND window{}, viewport{};
  hh::renderer::D3D11Renderer renderer;
  hh::renderer::OrthoCamera camera;
  hh::audio::SoundtrackPlayer soundtrack;
  std::filesystem::path directory, savePath;
  std::vector<Button> buttons;
  std::array<bool, 256> keys{};
  int width = 1500, height = 960, floor = 0, speed = 0, priorSpeed = 1,
      tabScroll = 0;
  int hoverX = -1, hoverY = -1;
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
  HFONT normal{}, smallFont{}, title{}, number{};
  Client();
  ~Client();
  void refresh();
  void layout();
  void paint(HDC);
  void click(int, int);
  void mapClick(int, int);
  void hover(int, int);
  void key(int);
  void changeFloor(int);
  bool save();
  void load();
  void result(const hh::game::CommandResult &);
  void newCampaign();
};
std::wstring wide(const std::string &);
std::wstring money(std::int64_t);
std::wstring roomStatus(hh::game::RoomStatus);
std::wstring personState(hh::game::PersonState);
} // namespace hh::client
