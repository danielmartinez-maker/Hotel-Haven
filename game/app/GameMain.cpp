#include "Client.h"
#include "hh/renderer/SceneComposer.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <thread>
#include <windowsx.h>

namespace hh::client {
using namespace hh::game;
using namespace hh::renderer;
namespace {
std::optional<Position> pick(const Client &c, int x, int y) {
  RECT r{};
  GetClientRect(c.viewport, &r);
  const float w = static_cast<float>(r.right), h = static_cast<float>(r.bottom);
  if (w <= 0 || h <= 0)
    return {};
  using namespace DirectX;
  auto unproject = [&](float depth) {
    XMFLOAT3 p{};
    XMStoreFloat3(
        &p, XMVector3Unproject(XMVectorSet(static_cast<float>(x),
                                           static_cast<float>(y), depth, 1),
                               0, 0, w, h, 0, 1, c.camera.projectionMatrix(),
                               c.camera.viewMatrix(), XMMatrixIdentity()));
    return p;
  };
  const auto near = unproject(0), far = unproject(1);
  const float dy = far.y - near.y;
  if (std::abs(dy) < 1e-6f)
    return {};
  const float t = (static_cast<float>(c.floor) * 3.2f - near.y) / dy;
  if (t < 0 || t > 1)
    return {};
  const int tx = static_cast<int>(std::floor(near.x + (far.x - near.x) * t)),
            ty = static_cast<int>(std::floor(near.z + (far.z - near.z) * t));
  if (tx < 0 || ty < 0 || tx >= c.snapshot.width || ty >= c.snapshot.height)
    return {};
  return Position{c.floor, tx, ty};
}
std::string readFile(const std::filesystem::path &path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream)
    throw std::runtime_error("Cannot open " + path.string());
  stream.seekg(0, std::ios::end);
  if (stream.tellg() > 64 * 1024 * 1024)
    throw std::runtime_error("Save is larger than the supported 64 MiB limit");
  stream.seekg(0);
  return {std::istreambuf_iterator<char>(stream), {}};
}
CommandResult applyBuildTool(Simulation &simulation, Tool tool,
                             Position position, std::size_t roomCount) {
  if (tool == Tool::Bedroom) {
    RoomBlueprint r;
    r.name =
        "Room " + std::to_string((position.floor + 1) * 100 + roomCount + 1);
    r.floor = position.floor;
    r.x = position.x;
    r.y = position.y;
    r.width = 6;
    r.height = 6;
    r.door = {position.floor, position.x + 2, position.y + 5};
    r.nightlyRate = 120;
    return simulation.buildFurnishedRoom(r);
  }
  constexpr std::array<TileKind, 13> kinds = {
      TileKind::Empty,     TileKind::Empty,        TileKind::Floor,
      TileKind::Wall,      TileKind::Door,         TileKind::Entrance,
      TileKind::FrontDesk, TileKind::SupplyCloset, TileKind::Stairs,
      TileKind::Empty,     TileKind::Bathroom,     TileKind::StaffRoom,
      TileKind::Lobby};
  return simulation.buildTile(position, kinds[static_cast<std::size_t>(tool)]);
}
void captureClient(HWND window, const std::filesystem::path &path) {
  RECT r{};
  GetClientRect(window, &r);
  const int w = r.right, h = r.bottom;
  if (w <= 0 || h <= 0)
    return;
  HDC screen = GetDC(window), memory = CreateCompatibleDC(screen);
  HBITMAP bitmap = CreateCompatibleBitmap(screen, w, h);
  HGDIOBJ old = SelectObject(memory, bitmap);
  const BOOL copied = BitBlt(memory, 0, 0, w, h, screen, 0, 0, SRCCOPY);
  SelectObject(memory, old);
  BITMAPINFO info{};
  info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  info.bmiHeader.biWidth = w;
  info.bmiHeader.biHeight = h;
  info.bmiHeader.biPlanes = 1;
  info.bmiHeader.biBitCount = 32;
  info.bmiHeader.biCompression = BI_RGB;
  std::vector<unsigned char> pixels(static_cast<std::size_t>(w) * h * 4);
  const int lines = GetDIBits(memory, bitmap, 0, static_cast<UINT>(h),
                              pixels.data(), &info, DIB_RGB_COLORS);
  DeleteObject(bitmap);
  DeleteDC(memory);
  ReleaseDC(window, screen);
  if (!copied || lines != h)
    return;
  BITMAPFILEHEADER header{};
  header.bfType = 0x4d42;
  header.bfOffBits = sizeof(header) + sizeof(BITMAPINFOHEADER);
  header.bfSize = header.bfOffBits + static_cast<DWORD>(pixels.size());
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<const char *>(&header), sizeof(header));
  out.write(reinterpret_cast<const char *>(&info.bmiHeader),
            sizeof(BITMAPINFOHEADER));
  out.write(reinterpret_cast<const char *>(pixels.data()),
            static_cast<std::streamsize>(pixels.size()));
}
LRESULT CALLBACK procedure(HWND window, UINT message, WPARAM wp, LPARAM lp) {
  auto *c =
      reinterpret_cast<Client *>(GetWindowLongPtrW(window, GWLP_USERDATA));
  if (message == WM_NCCREATE) {
    c = static_cast<Client *>(
        reinterpret_cast<CREATESTRUCTW *>(lp)->lpCreateParams);
    SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(c));
    if ((reinterpret_cast<CREATESTRUCTW *>(lp)->style & WS_CHILD) != 0)
      c->viewport = window;
    else
      c->window = window;
  }
  if (!c)
    return DefWindowProcW(window, message, wp, lp);
  const bool view = window == c->viewport;
  try {
    switch (message) {
    case WM_GETMINMAXINFO:
      if (!view) {
        auto *info = reinterpret_cast<MINMAXINFO *>(lp);
        info->ptMinTrackSize = {1200, 850};
        return 0;
      }
      break;
    case WM_CLOSE:
      if (!view) {
        if (!c->smoke) {
          const int choice =
              MessageBoxW(window, L"Save the campaign before closing?",
                          L"Hotel Haven", MB_YESNOCANCEL | MB_ICONQUESTION);
          if (choice == IDCANCEL)
            return 0;
          if (choice == IDYES && !c->save())
            return 0;
        }
        c->running = false;
        DestroyWindow(window);
      }
      return 0;
    case WM_DESTROY:
      if (!view)
        PostQuitMessage(0);
      return 0;
    case WM_SIZE:
      if (!view) {
        c->width = LOWORD(lp);
        c->height = HIWORD(lp);
        if (c->viewport && c->width > 0 && c->height > 0)
          c->layout();
      }
      return 0;
    case WM_ERASEBKGND:
      return 1;
    case WM_PAINT: {
      PAINTSTRUCT ps{};
      HDC dc = BeginPaint(window, &ps);
      if (!view)
        c->paint(dc);
      EndPaint(window, &ps);
      return 0;
    }
    case WM_KEYDOWN:
      if (wp < 256) {
        c->keys[wp] = true;
        if ((lp & (1LL << 30)) == 0)
          c->key(static_cast<int>(wp));
      }
      return 0;
    case WM_KEYUP:
      if (wp < 256)
        c->keys[wp] = false;
      return 0;
    case WM_KILLFOCUS:
      c->keys.fill(false);
      return 0;
    case WM_LBUTTONDOWN:
      if (view) {
        SetFocus(window);
        c->mapClick(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
      } else
        c->click(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
      return 0;
    case WM_MOUSEMOVE:
      if (view)
        c->hover(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
      return 0;
    case WM_MOUSEWHEEL: {
      const float steps =
          static_cast<float>(GET_WHEEL_DELTA_WPARAM(wp)) / WHEEL_DELTA;
      c->camera.setOrthoHeight(std::clamp(
          c->camera.orthoHeight() * std::pow(.85f, steps), 8.f, 150.f));
      return 0;
    }
    }
  } catch (const std::exception &error) {
    c->fatalError = error.what();
    c->running = false;
    return 0;
  }
  return DefWindowProcW(window, message, wp, lp);
}
} // namespace
Client::Client() : simulation(Simulation::tutorial(20260907)) {
  normal = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
  small = CreateFontW(-13, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                      CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
  title = CreateFontW(-23, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                      CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Georgia");
  number = CreateFontW(-24, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
  snapshot = simulation.view();
  camera.setTarget({static_cast<float>(snapshot.width) * .5f, 0,
                    static_cast<float>(snapshot.height) * .5f});
  camera.setYawDegrees(45);
  camera.setOrthoHeight(30);
}
Client::~Client() {
  DeleteObject(normal);
  DeleteObject(small);
  DeleteObject(title);
  DeleteObject(number);
}
void Client::refresh() {
  snapshot = simulation.view();
  if (window)
    InvalidateRect(window, nullptr, FALSE);
}
void Client::result(const CommandResult &r) {
  notice = wide(r.message);
  if (notice.empty())
    notice = r.ok ? L"Change applied." : L"This change could not be applied.";
  refresh();
}
void Client::layout() {
  const int vw = std::max(1, width - SidebarWidth),
            vh = std::max(1, height - HeaderHeight - FooterHeight);
  MoveWindow(viewport, 0, HeaderHeight, vw, vh, TRUE);
  camera.setAspectRatio(static_cast<float>(vw) / static_cast<float>(vh));
  if (initialized) {
    const auto r = renderer.resize(static_cast<std::uint32_t>(vw),
                                   static_cast<std::uint32_t>(vh));
    if (!r)
      throw std::runtime_error(r.error);
  }
  InvalidateRect(window, nullptr, FALSE);
}
void Client::click(int x, int y) {
  for (const auto &b : buttons)
    if (x >= b.rect.left && x < b.rect.right && y >= b.rect.top &&
        y < b.rect.bottom) {
      auto fn = b.action;
      fn();
      refresh();
      return;
    }
}
void Client::hover(int x, int y) {
  const auto p = pick(*this, x, y);
  if (p && p->x == hoverX && p->y == hoverY && previewTool == tool)
    return;
  hoverX = p ? p->x : -1;
  hoverY = p ? p->y : -1;
  previewTool = tool;
  if (p && tool != Tool::Inspect) {
    auto preview = simulation;
    const auto check = applyBuildTool(preview, tool, *p, snapshot.rooms.size());
    previewValid = check.ok;
    notice =
        check.ok ? L"Placement preview · click to build" : wide(check.message);
    InvalidateRect(window, nullptr, FALSE);
  }
}
void Client::mapClick(int x, int y) {
  const auto pos = pick(*this, x, y);
  if (!pos)
    return;
  if (tool == Tool::Inspect) {
    selected = 0;
    for (const auto &r : snapshot.rooms)
      if (r.floor == floor && pos->x >= r.x && pos->x < r.x + r.width &&
          pos->y >= r.y && pos->y < r.y + r.height) {
        selected = r.id;
        page = Page::Rooms;
        tabScroll = 0;
        break;
      }
    refresh();
    return;
  }
  const auto out =
      applyBuildTool(simulation, tool, *pos, snapshot.rooms.size());
  if (out && tool == Tool::Bedroom)
    selected = out.id;
  result(out);
  hoverX = hoverY = -1;
}
void Client::changeFloor(int requested) {
  floor = std::clamp(requested, 0, snapshot.floors - 1);
  auto target = camera.target();
  target.y = static_cast<float>(floor) * 3.2f;
  camera.setTarget(target);
  hoverX = hoverY = -1;
}
void Client::key(int k) {
  if (k == VK_SPACE) {
    if (speed) {
      priorSpeed = speed;
      speed = 0;
    } else
      speed = priorSpeed;
  }
  if (k == 'Q')
    camera.rotateSnapped(-1);
  if (k == 'E')
    camera.rotateSnapped(1);
  if (k == VK_PRIOR)
    changeFloor(floor + 1);
  if (k == VK_NEXT)
    changeFloor(floor - 1);
  if (k == VK_F5)
    save();
  if (k == VK_F9)
    load();
  if (k == VK_ESCAPE) {
    tool = Tool::Inspect;
    selected = 0;
  }
  if (k == 'C')
    context = !context;
  refresh();
}
bool Client::save() {
  try {
    std::filesystem::create_directories(savePath.parent_path());
    const auto tmp = savePath.wstring() + L".tmp";
    {
      std::ofstream out(std::filesystem::path(tmp),
                        std::ios::binary | std::ios::trunc);
      out << simulation.save();
      out.flush();
      if (!out)
        throw std::runtime_error("Cannot write save file");
    }
    if (!MoveFileExW(tmp.c_str(), savePath.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
      throw std::runtime_error(
          "Cannot replace save file; previous save preserved");
    notice = L"Campaign saved. F9 restores this exact state.";
  } catch (const std::exception &e) {
    notice = L"Save failed: " + wide(e.what());
    refresh();
    return false;
  }
  refresh();
  return true;
}
void Client::load() {
  try {
    auto restored = Simulation::load(readFile(savePath));
    simulation = std::move(restored);
    pendingSimulationSeconds = 0;
    speed = 0;
    selected = 0;
    tabScroll = 0;
    refresh();
    changeFloor(std::min(floor, snapshot.floors - 1));
    notice = L"Campaign restored and paused.";
  } catch (const std::exception &e) {
    notice = L"Load failed: " + wide(e.what());
  }
  refresh();
}
void Client::newCampaign() {
  if (MessageBoxW(
          window,
          L"Start a new campaign? Save first to retain the current campaign.",
          L"New campaign", MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;
  auto campaign = Simulation::tutorial(20260907);
  const auto definitions = directory / L"data" / L"balance.json";
  if (std::filesystem::exists(definitions)) {
    const auto result = campaign.loadDefinitions(readFile(definitions));
    if (!result) {
      notice = wide(result.message);
      refresh();
      return;
    }
  }
  simulation = std::move(campaign);
  pendingSimulationSeconds = 0;
  selected = 0;
  speed = 0;
  floor = 0;
  page = Page::Guide;
  refresh();
  camera.setTarget({static_cast<float>(snapshot.width) * .5f, 0,
                    static_cast<float>(snapshot.height) * .5f});
}
} // namespace hh::client

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine,
                    int show) {
  using namespace hh::client;
  try {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    Client c;
    c.smoke =
        std::wstring(commandLine).find(L"--smoke-test") != std::wstring::npos;
    std::array<wchar_t, 32768> path{};
    const DWORD n = GetModuleFileNameW(nullptr, path.data(),
                                       static_cast<DWORD>(path.size()));
    if (!n || n >= path.size())
      throw std::runtime_error("Cannot resolve game directory");
    c.directory = std::filesystem::path(path.data()).parent_path();
    const auto definitions = c.directory / L"data" / L"balance.json";
    if (std::filesystem::exists(definitions)) {
      const auto result = c.simulation.loadDefinitions(readFile(definitions));
      if (!result)
        throw std::runtime_error(result.message);
    }
    const DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", path.data(),
                                              static_cast<DWORD>(path.size()));
    c.savePath =
        (len > 0 && len < path.size() ? std::filesystem::path(path.data())
                                      : c.directory) /
        L"HotelHaven" / L"campaign.hhsave";
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = procedure;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"HotelHavenGame";
    if (!RegisterClassExW(&wc))
      throw std::runtime_error("Cannot register game window");
    c.window = CreateWindowExW(0, wc.lpszClassName, L"Hotel Haven",
                               WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                               CW_USEDEFAULT, CW_USEDEFAULT, c.width, c.height,
                               nullptr, nullptr, instance, &c);
    if (!c.window)
      throw std::runtime_error("Cannot create game window");
    c.viewport = CreateWindowExW(
        0, wc.lpszClassName, L"Hotel", WS_CHILD | WS_VISIBLE, 0, HeaderHeight,
        c.width - SidebarWidth, c.height - HeaderHeight - FooterHeight,
        c.window, nullptr, instance, &c);
    if (!c.viewport)
      throw std::runtime_error("Cannot create hotel viewport");
    c.layout();
    RECT rect{};
    GetClientRect(c.viewport, &rect);
    const auto init = c.renderer.initialize(
        c.viewport, static_cast<std::uint32_t>(rect.right),
        static_cast<std::uint32_t>(rect.bottom),
        c.directory / L"shaders" / L"InstancedBox.hlsl", c.smoke);
    if (!init)
      throw std::runtime_error(init.error);
    c.initialized = true;
    ShowWindow(c.window, show);
    UpdateWindow(c.window);
    c.refresh();
    auto previous = std::chrono::steady_clock::now();
    double refreshTime = 0;
    int frames = 0;
    hh::renderer::SceneComposer composer;
    while (c.running) {
      MSG msg{};
      while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
          c.running = false;
          break;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
      }
      if (!c.running)
        break;
      const auto now = std::chrono::steady_clock::now();
      const double dt = std::clamp(
          std::chrono::duration<double>(now - previous).count(), 0., .25);
      previous = now;
      if (IsIconic(c.window)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        continue;
      }
      auto target = c.camera.target();
      const float amount =
          static_cast<float>(dt) * c.camera.orthoHeight() * .7f;
      const float yaw = DirectX::XMConvertToRadians(c.camera.yawDegrees());
      const float x = static_cast<float>(c.keys['D']) -
                      static_cast<float>(c.keys['A']),
                  z = static_cast<float>(c.keys['W']) -
                      static_cast<float>(c.keys['S']);
      target.x += (x * std::cos(yaw) + z * std::sin(yaw)) * amount;
      target.z += (-x * std::sin(yaw) + z * std::cos(yaw)) * amount;
      c.camera.setTarget(target);
      if (c.speed) {
        c.pendingSimulationSeconds += dt * 60 * c.speed;
        const auto seconds =
            static_cast<std::int64_t>(std::floor(c.pendingSimulationSeconds));
        if (seconds > 0) {
          c.simulation.step(static_cast<double>(seconds));
          c.snapshot = c.simulation.view();
          c.pendingSimulationSeconds -= static_cast<double>(seconds);
        }
      }
      refreshTime += dt;
      if (refreshTime >= .1 || frames == 0) {
        c.refresh();
        refreshTime = 0;
      }
      const auto scene =
          worldScene(c.snapshot, {c.floor, c.selected, c.overlay, c.hoverX,
                                  c.hoverY, c.tool == Tool::Bedroom ? 6.f : 1.f,
                                  c.tool != Tool::Inspect, c.previewValid});
      const auto frame = composer.compose(
          scene,
          c.context ? hh::renderer::FloorContextMode::AdjacentContext
                    : hh::renderer::FloorContextMode::Normal,
          c.wallMode, c.camera.worldPosition());
      const auto draw = c.renderer.render(frame, c.camera);
      if (!draw)
        throw std::runtime_error(draw.error);
      ++frames;
      if (c.smoke && frames == 10)
        captureClient(c.window, c.directory / L"smoke-guide.bmp");
      if (c.smoke && frames == 20) {
        c.page = Page::Rooms;
        if (!c.snapshot.rooms.empty())
          c.selected = c.snapshot.rooms.front().id;
        c.refresh();
        UpdateWindow(c.window);
      }
      if (c.smoke && frames >= 30) {
        RECT viewSize{};
        GetClientRect(c.viewport, &viewSize);
        if (viewSize.right != c.width - SidebarWidth ||
            viewSize.bottom != c.height - HeaderHeight - FooterHeight)
          throw std::runtime_error(
              "Viewport dimensions do not match client layout");
        captureClient(c.window, c.directory / L"smoke-hotel.bmp");
        auto state = c.simulation.save();
        auto restored = hh::game::Simulation::load(state);
        if (restored.save() != state)
          throw std::runtime_error("Client save roundtrip mismatch");
        c.running = false;
      }
    }
    c.renderer.shutdown();
    if (!c.fatalError.empty())
      throw std::runtime_error(c.fatalError);
    return 0;
  } catch (const std::exception &e) {
    OutputDebugStringA(e.what());
    if (std::wstring(commandLine).find(L"--smoke-test") == std::wstring::npos)
      MessageBoxW(nullptr, wide(e.what()).c_str(),
                  L"Hotel Haven could not continue", MB_OK | MB_ICONERROR);
    return 1;
  }
}
