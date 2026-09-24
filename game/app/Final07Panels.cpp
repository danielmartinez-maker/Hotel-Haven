#include "Client.h"
#include "LiveBuildCatalog.h"
#include "hh/frontend/EconomyDashboard.h"
#include "hh/frontend/OverlayModel.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace hh::client {
namespace {
using hh::frontend::AlertSeverity;
using hh::frontend::InspectorKind;
using hh::frontend::OperationArea;
using hh::frontend::OverlayId;
using hh::frontend::UiCommand;
using hh::frontend::UiCommandType;
using hh::frontend::UiEntitySnapshot;

constexpr COLORREF Bg = RGB(232, 229, 217);
constexpr COLORREF Panel = RGB(250, 247, 238);
constexpr COLORREF Ink = RGB(34, 52, 48);
constexpr COLORREF Muted = RGB(101, 112, 98);
constexpr COLORREF Accent = RGB(42, 105, 92);
constexpr COLORREF AccentSoft = RGB(224, 234, 226);
constexpr COLORREF Warning = RGB(150, 102, 31);
constexpr COLORREF WarningSoft = RGB(244, 235, 210);
constexpr COLORREF Critical = RGB(145, 55, 50);
constexpr COLORREF CriticalSoft = RGB(245, 224, 220);
constexpr COLORREF Line = RGB(207, 208, 192);

enum class ButtonTone {
  Normal,
  Warning,
  Critical,
};
constexpr COLORREF Chrome = RGB(30, 46, 42);
constexpr COLORREF ChromeRaised = RGB(39, 58, 53);
constexpr COLORREF ChromeText = RGB(248, 244, 234);
constexpr COLORREF ChromeMuted = RGB(186, 198, 187);
constexpr COLORREF ChromeLine = RGB(74, 91, 84);

void fill(HDC dc, RECT rect, COLORREF color) {
  HBRUSH brush = CreateSolidBrush(color);
  FillRect(dc, &rect, brush);
  DeleteObject(brush);
}

void frame(HDC dc, RECT rect, COLORREF color) {
  HBRUSH brush = CreateSolidBrush(color);
  FrameRect(dc, &rect, brush);
  DeleteObject(brush);
}

void drawText(HDC dc, HFONT font, const std::wstring &value, int x, int y,
              int width, int height, COLORREF color = Ink,
              UINT format = DT_LEFT | DT_WORDBREAK) {
  SelectObject(dc, font);
  SetTextColor(dc, color);
  SetBkMode(dc, TRANSPARENT);
  RECT rect{x, y, x + width, y + height};
  DrawTextW(dc, value.c_str(), -1, &rect, format | DT_NOPREFIX);
}

std::wstring permille(int value) {
  std::wostringstream out;
  out << std::fixed << std::setprecision(1) << static_cast<double>(value) / 10.0
      << L"%";
  return out.str();
}

std::wstring alertSeverity(AlertSeverity severity) {
  switch (severity) {
  case AlertSeverity::Info: return L"INFO";
  case AlertSeverity::Warning: return L"WARNING";
  case AlertSeverity::Critical: return L"CRITICAL";
  }
  return L"INFO";
}

COLORREF alertColor(AlertSeverity severity) {
  switch (severity) {
  case AlertSeverity::Info: return Accent;
  case AlertSeverity::Warning: return Warning;
  case AlertSeverity::Critical: return Critical;
  }
  return Accent;
}

std::wstring pageLabel(Page page) {
  constexpr std::array<const wchar_t *, 12> labels{
      L"Build",      L"Rooms",      L"Staff",      L"Guests",
      L"Supplies",   L"Operations", L"Finance",    L"Alerts",
      L"Objectives", L"Overlays",   L"Settings",   L"Guide"};
  return labels[static_cast<std::size_t>(page)];
}

std::wstring navPageLabel(Page page, bool compact) {
  if (compact && page == Page::Operations)
    return L"Ops";
  return pageLabel(page);
}

bool belongs(Page page, InspectorKind kind) {
  switch (page) {
  case Page::Rooms: return kind == InspectorKind::Room;
  case Page::Guests: return kind == InspectorKind::Guest;
  case Page::Staff:
    return kind == InspectorKind::Employee || kind == InspectorKind::Department;
  case Page::Supplies:
    return kind == InspectorKind::Inventory || kind == InspectorKind::Task;
  case Page::Operations:
    return kind == InspectorKind::Task || kind == InspectorKind::ServiceVenue ||
           kind == InspectorKind::BuildJob || kind == InspectorKind::Elevator ||
           kind == InspectorKind::BuildingSystem;
  case Page::Build:
  case Page::Finance:
  case Page::Alerts:
  case Page::Objectives:
  case Page::Overlays:
  case Page::Settings:
  case Page::Guide: return false;
  }
  return false;
}

std::vector<const UiEntitySnapshot *> pageEntities(const Client &client, Page page) {
  std::vector<const UiEntitySnapshot *> result;
  for (const auto &entity : client.ui.snapshot().entities)
    if (belongs(page, entity.kind))
      result.push_back(&entity);
  return result;
}

std::wstring overlayName(OverlayId id) {
  if (id == OverlayId::None)
    return L"Natural view";
  const auto descriptors = hh::frontend::OverlayModel::requiredDescriptors();
  const auto found = std::find_if(descriptors.begin(), descriptors.end(),
                                  [id](const auto &value) { return value.id == id; });
  return found == descriptors.end() ? L"Overlay" : wide(found->label);
}

std::wstring keyName(int keyCode) {
  if ((keyCode >= '0' && keyCode <= '9') ||
      (keyCode >= 'A' && keyCode <= 'Z'))
    return std::wstring(1, static_cast<wchar_t>(keyCode));
  if (keyCode >= VK_F1 && keyCode <= VK_F24)
    return L"F" + std::to_wstring(keyCode - VK_F1 + 1);
  switch (keyCode) {
  case VK_UP: return L"Up";
  case VK_DOWN: return L"Down";
  case VK_LEFT: return L"Left";
  case VK_RIGHT: return L"Right";
  case VK_RETURN: return L"Enter";
  case VK_ESCAPE: return L"Esc";
  case VK_SPACE: return L"Space";
  case VK_TAB: return L"Tab";
  case VK_BACK: return L"Backspace";
  case VK_DELETE: return L"Delete";
  case VK_HOME: return L"Home";
  case VK_END: return L"End";
  case VK_PRIOR: return L"Page Up";
  case VK_NEXT: return L"Page Down";
  case VK_OEM_MINUS: return L"-";
  case VK_OEM_PLUS: return L"+";
  default: return L"Key " + std::to_wstring(keyCode);
  }
}

hh::frontend::OperationsFilter operationFilter(int filterIndex) {
  hh::frontend::OperationsFilter filter;
  if (filterIndex > 0)
    filter.area = static_cast<OperationArea>(filterIndex - 1);
  return filter;
}

std::wstring operationSortLabel(hh::frontend::OperationSort sort) {
  using hh::frontend::OperationSort;
  switch (sort) {
  case OperationSort::SchedulerOrder: return L"Scheduler";
  case OperationSort::PriorityHighFirst: return L"Priority";
  case OperationSort::OldestFirst: return L"Oldest";
  }
  return L"Scheduler";
}

std::size_t fieldSectionRows(
    const std::vector<hh::frontend::FieldSnapshot> &values) {
  return values.empty() ? 0 : values.size() + 1;
}

std::size_t financePageRows(const Client &client) {
  const auto &economy = client.economyDashboard.snapshot();
  switch (client.financeView) {
  case FinanceView::Overview:
    return 10;
  case FinanceView::Revenue:
    return fieldSectionRows(economy.departmentContribution) +
           fieldSectionRows(economy.bookingPace) +
           fieldSectionRows(economy.cancellationAndNoShow) +
           fieldSectionRows(economy.channelMix) +
           fieldSectionRows(economy.futureRateCalendar);
  case FinanceView::Market:
    return fieldSectionRows(economy.competitors) +
           fieldSectionRows(economy.demandBySegment);
  case FinanceView::Controls:
    return 0;
  case FinanceView::Risk:
    return fieldSectionRows(economy.debtSchedule) +
           economy.financingDiagnostics.size() +
           fieldSectionRows(economy.campaigns) +
           fieldSectionRows(economy.contracts);
  }
  return 0;
}

int pageScrollStep(const Client &client) {
  if (client.page == Page::Build) {
    return computeFinal07PanelLayout(
               client.width, client.height, client.uiSettings.scalePercent())
        .buildColumns;
  }
  if (client.page == Page::Settings &&
      client.settingsView == SettingsView::Controls)
    return 2;
  return 1;
}

std::size_t pageRows(const Client &client) {
  switch (client.page) {
  case Page::Rooms:
  case Page::Guests:
  case Page::Staff:
  case Page::Supplies: return pageEntities(client, client.page).size();
  case Page::Operations:
    return client.operationsDashboard.filteredCount(
        operationFilter(client.operationsFilter));
  case Page::Alerts: return client.alertCenter.active().size();
  case Page::Objectives: return client.objectiveUi.items().size();
  case Page::Overlays:
    return hh::frontend::OverlayModel::requiredDescriptors().size();
  case Page::Build: {
    std::size_t count = 0;
    for (const auto &item : client.ui.snapshot().buildCatalog) {
      const bool categoryMatches =
          client.buildCategoryFilter.empty() ||
          item.category == client.buildCategoryFilter;
      count += liveBuildTool(item.id).has_value() && categoryMatches;
    }
    return count;
  }
  case Page::Finance: return financePageRows(client);
  case Page::Settings:
    return client.settingsView == SettingsView::Controls
               ? hh::frontend::EditableKeyBindings.size()
               : 0;
  case Page::Guide: return 0;
  }
  return 0;
}
} // namespace

std::wstring wide(const std::string &source) {
  if (source.empty())
    return {};
  const int size = MultiByteToWideChar(CP_UTF8, 0, source.data(),
                                       static_cast<int>(source.size()), nullptr, 0);
  std::wstring result(static_cast<std::size_t>(size), L' ');
  MultiByteToWideChar(CP_UTF8, 0, source.data(), static_cast<int>(source.size()),
                      result.data(), size);
  return result;
}

std::wstring money(std::int64_t cents) {
  return wide(hh::frontend::EconomyDashboard::formatMoney(cents));
}

std::wstring compactMoney(std::int64_t cents) {
  const bool negative = cents < 0;
  const std::uint64_t magnitude =
      negative ? static_cast<std::uint64_t>(-(cents + 1)) + 1
               : static_cast<std::uint64_t>(cents);
  const double dollars = static_cast<double>(magnitude) / 100.0;

  double value = dollars;
  const wchar_t *suffix = L"";
  if (magnitude >= 100000000ULL) {
    value = dollars / 1000000.0;
    suffix = L"M";
  } else if (magnitude >= 100000ULL) {
    value = dollars / 1000.0;
    suffix = L"K";
  } else {
    return money(cents);
  }

  std::wostringstream out;
  if (negative)
    out << L"-";
  out << L"$" << std::fixed << std::setprecision(value >= 100.0 ? 0 : 1)
      << value << suffix;
  return out.str();
}

std::wstring roomStatus(hh::game::RoomStatus status) {
  switch (status) {
  case hh::game::RoomStatus::VacantReady: return L"Ready to sell";
  case hh::game::RoomStatus::Reserved: return L"Reserved";
  case hh::game::RoomStatus::Occupied: return L"Occupied";
  case hh::game::RoomStatus::VacantDirty: return L"Needs cleaning";
  case hh::game::RoomStatus::Cleaning: return L"Cleaning";
  case hh::game::RoomStatus::OutOfOrder: return L"Out of service";
  case hh::game::RoomStatus::Incomplete: return L"Incomplete";
  }
  return L"Unknown";
}

std::wstring personState(hh::game::PersonState state) {
  switch (state) {
  case hh::game::PersonState::OffDuty: return L"Off duty";
  case hh::game::PersonState::Idle: return L"Idle";
  case hh::game::PersonState::Traveling: return L"Traveling";
  case hh::game::PersonState::Working: return L"Working";
  case hh::game::PersonState::Waiting: return L"Waiting";
  case hh::game::PersonState::Sleeping: return L"Sleeping";
  case hh::game::PersonState::CheckedOut: return L"Checked out";
  }
  return L"Unknown";
}

void Client::hoverUi(int x, int y) {
  uiSettings.setInputModality(hh::frontend::InputModality::Mouse);
  int nextHover = -1;
  for (std::size_t index = 0; index < buttons.size(); ++index) {
    const auto &rect = buttons[index].rect;
    if (x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom) {
      nextHover = static_cast<int>(index);
      break;
    }
  }

  if (nextHover == hoveredButton)
    return;

  hoveredButton = nextHover;
  SetCursor(LoadCursorW(nullptr, hoveredButton >= 0 ? IDC_HAND : IDC_ARROW));
  if (window)
    InvalidateRect(window, nullptr, FALSE);
}

void Client::scrollPanel(int delta) {
  if (delta == 0)
    return;

  const std::size_t count = pageRows(*this);
  if (count == 0)
    return;

  const int maximum =
      count == 0 ? 0 : static_cast<int>(count - 1);
  const int step = pageScrollStep(*this);
  const int next =
      std::clamp(tabScroll + delta * step, 0, maximum);
  if (next == tabScroll)
    return;

  tabScroll = next;
  hoveredButton = -1;
  SetCursor(LoadCursorW(nullptr, IDC_ARROW));
  if (window)
    InvalidateRect(window, nullptr, FALSE);
}

void Client::paint(HDC output) {
  objectiveUi.update(ui.snapshot().objectives);
  const std::size_t currentPageRows = pageRows(*this);
  if (currentPageRows == 0) {
    tabScroll = 0;
  } else {
    tabScroll = std::clamp(
        tabScroll, 0, static_cast<int>(currentPageRows - 1));
  }

  HDC dc = CreateCompatibleDC(output);
  HBITMAP bitmap = CreateCompatibleBitmap(output, width, height);
  HGDIOBJ previous = SelectObject(dc, bitmap);
  fill(dc, {0, 0, width, height}, Bg);
  buttons.clear();

  const int uiScale = uiSettings.scalePercent();
  const auto panelLayout = computeFinal07PanelLayout(width, height, uiScale);
  const auto px = [uiScale](int logicalPixels) {
    return final07ScalePixel(logicalPixels, uiScale);
  };
  const int densityScale = panelLayout.densityScalePercent;
  const auto vpx = [densityScale](int logicalPixels) {
    return final07ScalePixel(logicalPixels, densityScale);
  };

  fill(dc, {0, 0, width, HeaderHeight}, Chrome);
  fill(dc, {0, height - FooterHeight, width, height}, Chrome);

  auto button = [&](int x, int y, int w, int h, std::wstring value,
                    std::function<void()> action, bool active = false,
                    ButtonTone tone = ButtonTone::Normal) {
    RECT rect{x, y, x + w, y + h};
    const int buttonIndex = static_cast<int>(buttons.size());
    const bool hovered = buttonIndex == hoveredButton;
    const bool chromeButton =
        rect.bottom <= HeaderHeight || rect.top >= height - FooterHeight;
    const bool warningTone = tone == ButtonTone::Warning;
    const bool criticalTone = tone == ButtonTone::Critical;
    const COLORREF toneAccent =
        criticalTone ? Critical : warningTone ? Warning : Accent;
    const COLORREF softTone =
        criticalTone ? CriticalSoft : warningTone ? WarningSoft : AccentSoft;
    const COLORREF idleFill =
        chromeButton ? ChromeRaised
                     : (warningTone || criticalTone ? softTone : Panel);
    const COLORREF hoverFill =
        chromeButton ? ChromeLine : softTone;
    const COLORREF idleBorder =
        warningTone || criticalTone
            ? toneAccent
            : (chromeButton ? ChromeLine : Line);
    fill(dc, rect, active ? toneAccent : (hovered ? hoverFill : idleFill));
    frame(dc, rect, active || hovered ? toneAccent : idleBorder);
    if (active) {
      const int markerWidth = std::max(2, px(3));
      fill(dc, {rect.left, rect.top, rect.left + markerWidth, rect.bottom},
           chromeButton ? ChromeText : Panel);
    }
    const int horizontalPadding = px(5);
    const int buttonTextHeight = std::max(px(13), vpx(16));
    const int verticalPadding = std::max(1, (h - buttonTextHeight) / 2);
    drawText(dc, small, value, x + horizontalPadding, y + verticalPadding,
             std::max(1, w - horizontalPadding * 2),
             std::max(1, h - verticalPadding),
             active || chromeButton ? ChromeText : Ink,
             DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    buttons.push_back({rect, std::move(value), std::move(action), active});
  };

  const auto &gameUi = ui.snapshot();
  const auto &hud = gameUi.hud;

  const int actionGap = px(8);
  const int actionRight = px(22);
  const int alertWidth = px(88);
  const int saveWidth = px(76);
  const int loadWidth = px(76);
  const int actionHeight = px(34);
  const int actionY = px(19);
  const int loadX = width - actionRight - loadWidth;
  const int saveX = loadX - actionGap - saveWidth;
  const int alertsX = saveX - actionGap - alertWidth;

  const int metricLeft = px(235);
  const int metricRight = std::max(metricLeft + px(4), alertsX - px(12));
  const int metricGap = px(8);
  const int metricWidth =
      std::max(1, (metricRight - metricLeft - metricGap * 3) / 4);
  const int brandWidth = std::max(px(120), metricLeft - px(44));

  drawText(dc, title, L"HOTEL HAVEN", px(22), px(14), brandWidth, px(30),
           ChromeText);
  drawText(dc, small, L"PROPERTY MANAGEMENT", px(23), px(47), brandWidth,
           px(18), ChromeMuted);

  auto metric = [&](int index, const std::wstring &labelText,
                    const std::wstring &valueText, bool compact = false,
                    std::function<void()> action = {}) {
    const int x = metricLeft + index * (metricWidth + metricGap);
    RECT surface{x, px(10), x + metricWidth, HeaderHeight - px(24)};
    const int metricButtonIndex =
        action ? static_cast<int>(buttons.size()) : -1;
    const bool hovered =
        metricButtonIndex >= 0 && metricButtonIndex == hoveredButton;
    fill(dc, surface, hovered ? ChromeLine : ChromeRaised);
    frame(dc, surface, hovered ? Accent : ChromeLine);
    drawText(dc, small, labelText, x + px(8), px(15),
             std::max(1, metricWidth - px(16)), px(18), ChromeMuted,
             DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
    HFONT valueFont = compact || metricWidth < px(145) ? normal : number;
    drawText(dc, valueFont, valueText, x + px(8), px(37),
             std::max(1, metricWidth - px(16)), px(26), ChromeText,
             DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
    if (action)
      buttons.push_back(
          {surface, labelText + L" · " + valueText, std::move(action), false});
  };

  std::wostringstream clock;
  clock << std::setw(2) << std::setfill(L'0') << hud.hour << L":" << std::setw(2)
        << hud.minute;
  const bool compactHud = metricWidth < px(130);
  metric(0, L"CASH",
         compactHud ? compactMoney(hud.cashCents) : money(hud.cashCents),
         compactHud, [this] {
           page = Page::Finance;
           financeView = FinanceView::Overview;
           tabScroll = 0;
         });
  metric(1, L"OCCUPANCY", permille(hud.occupancyPermille), false, [this] {
    page = Page::Finance;
    financeView = FinanceView::Overview;
    tabScroll = 0;
  });
  metric(2, compactHud ? L"SAT / REP" : L"SATISFACTION / REP",
         permille(hud.satisfactionPermille) + L" / " +
             permille(hud.reputationPermille),
         true, [this] {
           page = Page::Guests;
           tabScroll = 0;
         });
  metric(3, L"DAY " + std::to_wstring(hud.day + 1), clock.str(), false);

  int criticalAlertCount = 0;
  int warningAlertCount = 0;
  for (const auto &alert : alertCenter.active()) {
    criticalAlertCount += alert.severity == AlertSeverity::Critical;
    warningAlertCount += alert.severity == AlertSeverity::Warning;
  }
  const std::wstring alertLabel =
      criticalAlertCount > 0
          ? L"Critical " + std::to_wstring(criticalAlertCount)
          : warningAlertCount > 0
                ? L"Warnings " + std::to_wstring(warningAlertCount)
                : L"Alerts " + std::to_wstring(hud.alertCount);
  const ButtonTone alertTone =
      criticalAlertCount > 0
          ? ButtonTone::Critical
          : warningAlertCount > 0 ? ButtonTone::Warning : ButtonTone::Normal;
  button(alertsX, actionY, alertWidth, actionHeight, alertLabel,
         [this] {
           page = Page::Alerts;
           tabScroll = 0;
         },
         page == Page::Alerts, alertTone);
  button(saveX, actionY, saveWidth, actionHeight, L"Save [F5]", [this] {
    const auto result = ui.dispatchUiCommand(UiCommand{UiCommandType::SaveGame});
    if (!result.message.empty())
      notice = wide(result.message);
  });
  button(loadX, actionY, loadWidth, actionHeight, L"Load [F9]", [this] {
    const auto result = ui.dispatchUiCommand(UiCommand{UiCommandType::LoadGame});
    if (!result.message.empty())
      notice = wide(result.message);
  });

  const int sidebarX = width - SidebarWidth;
  fill(dc, {sidebarX, HeaderHeight, width, height - FooterHeight}, Panel);
  fill(dc, {sidebarX, HeaderHeight, sidebarX + 1, height - FooterHeight}, Line);

  const int navMargin = px(12);
  const int navGap = px(4);
  const int navRowGap = vpx(5);
  const int navHeight = vpx(30);
  const int navWidth =
      std::max(1, (SidebarWidth - navMargin * 2 - navGap * 3) / 4);
  for (int index = 0; index < 12; ++index) {
    const auto target = static_cast<Page>(index);
    const int row = index / 4;
    const int column = index % 4;
    button(sidebarX + navMargin + column * (navWidth + navGap),
           HeaderHeight + vpx(12) + row * (navHeight + navRowGap), navWidth,
           navHeight, navPageLabel(target, navWidth < px(90)),
           [this, target] {
             page = target;
             tabScroll = 0;
           },
           page == target);
  }

  const int left = sidebarX + px(20);
  const int panelWidth = panelLayout.contentWidthPixels;
  const int controlGap = px(12);
  const int halfControlWidth = std::max(1, (panelWidth - controlGap) / 2);
  int y = panelLayout.contentTopPixels;
  const int bottom = panelLayout.contentBottomPixels;

  auto heading = [&](const std::wstring &value) {
    drawText(dc, small, L"MANAGEMENT  /  " + pageLabel(page), left, y,
             panelWidth, std::max(px(13), vpx(18)), Muted,
             DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
    y += std::max(px(13), vpx(18));
    drawText(dc, title, value, left, y, panelWidth,
             std::max(px(24), vpx(32)));
    y += std::max(px(28), vpx(43));
  };
  auto paragraph = [&](const std::wstring &value, int h = 48,
                       COLORREF color = Ink) {
    const int paragraphHeight = std::max(px(18), vpx(h));
    const int advance = paragraphHeight + vpx(9);
    if (y + paragraphHeight > bottom) {
      y = bottom;
      return;
    }
    drawText(dc, normal, value, left, y, panelWidth, paragraphHeight, color);
    y = std::min(bottom, y + advance);
  };
  auto label = [&](const std::wstring &name, const std::wstring &value) {
    const int keyWidth = std::min(px(174), std::max(px(110), panelWidth / 2));
    const int labelHeight = std::max(px(16), vpx(24));
    const int advance = std::max(px(18), vpx(28));
    if (y + labelHeight > bottom) {
      y = bottom;
      return;
    }
    drawText(dc, small, name, left, y, keyWidth, labelHeight, Muted,
             DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
    drawText(dc, normal, value, left + keyWidth, y,
             std::max(1, panelWidth - keyWidth), labelHeight, Ink,
             DT_RIGHT | DT_SINGLELINE | DT_END_ELLIPSIS);
    y = std::min(bottom, y + advance);
  };
  auto separator = [&] {
    if (y >= bottom)
      return;
    fill(dc, {left, y, left + panelWidth, y + 1}, Line);
    y = std::min(bottom, y + vpx(15));
  };
  auto fullButton = [&](std::wstring value, std::function<void()> action,
                        bool active = false,
                        ButtonTone tone = ButtonTone::Normal) {
    const int controlHeight = std::max(px(24), vpx(32));
    const int advance = std::max(px(28), vpx(39));
    if (y + controlHeight > bottom) {
      y = bottom;
      return;
    }
    button(left, y, panelWidth, controlHeight, std::move(value),
           std::move(action), active, tone);
    y = std::min(bottom, y + advance);
  };

  if (page == Page::Build) {
    heading(L"Build & construction");
    if (bottom - y >= vpx(420)) {
      paragraph(L"Choose a construction item, move it over the hotel to check placement and cost, then click to build.", 64);
    } else if (bottom - y >= vpx(300)) {
      paragraph(L"Select an item, validate it over the hotel, then click to place.", 36, Muted);
    }

    struct VisibleBuildTool {
      Tool tool;
      const hh::frontend::BuildCatalogItem *item;
    };
    std::vector<std::string> buildCategories;
    std::vector<VisibleBuildTool> visibleTools;
    visibleTools.reserve(gameUi.buildCatalog.size());
    for (const auto &item : gameUi.buildCatalog) {
      const auto mappedTool = liveBuildTool(item.id);
      if (!mappedTool)
        continue;
      if (std::find(buildCategories.begin(), buildCategories.end(),
                    item.category) == buildCategories.end())
        buildCategories.push_back(item.category);
      if (buildCategoryFilter.empty() || item.category == buildCategoryFilter)
        visibleTools.push_back({*mappedTool, &item});
    }

    const bool filterStillExists =
        buildCategoryFilter.empty() ||
        std::find(buildCategories.begin(), buildCategories.end(),
                  buildCategoryFilter) != buildCategories.end();
    if (!filterStillExists) {
      buildCategoryFilter.clear();
      tabScroll = 0;
      visibleTools.clear();
      for (const auto &item : gameUi.buildCatalog) {
        const auto mappedTool = liveBuildTool(item.id);
        if (mappedTool)
          visibleTools.push_back({*mappedTool, &item});
      }
    }

    const int categoryButtonHeight = std::max(px(22), vpx(30));
    button(left, y, panelWidth, categoryButtonHeight,
           L"Browse · " +
               (buildCategoryFilter.empty() ? std::wstring(L"All")
                                            : wide(buildCategoryFilter)),
           [this, buildCategories] {
             if (buildCategories.empty()) {
               buildCategoryFilter.clear();
               tabScroll = 0;
               return;
             }
             if (buildCategoryFilter.empty()) {
               buildCategoryFilter = buildCategories.front();
             } else {
               const auto current =
                   std::find(buildCategories.begin(), buildCategories.end(),
                             buildCategoryFilter);
               if (current == buildCategories.end() ||
                   std::next(current) == buildCategories.end())
                 buildCategoryFilter.clear();
               else
                 buildCategoryFilter = *std::next(current);
             }
             tabScroll = 0;
             tool = Tool::Inspect;
             buildPreview = {};
             previewValid = false;
             notice = buildCategoryFilter.empty()
                          ? L"Showing all construction items."
                          : L"Build category changed.";
           },
           !buildCategoryFilter.empty());
    y += std::max(px(28), vpx(39));

    const int buildRowHeight = vpx(35);
    const int buildButtonHeight = std::max(px(22), vpx(30));
    const int buildColumns = panelLayout.buildColumns;
    const int buildColumnWidth =
        std::max(1, (panelWidth - controlGap * (buildColumns - 1)) /
                         buildColumns);
    const int buildDetailReserve =
        buildPreview.requestId != 0
            ? vpx(150)
            : (tool == Tool::Inspect ? vpx(70) : vpx(105));
    const int gridHeight =
        std::max(buildRowHeight, bottom - buildDetailReserve - y);
    const int visibleRows = std::max(1, gridHeight / buildRowHeight);
    const std::size_t visibleToolSlots = static_cast<std::size_t>(
        std::max(1, visibleRows * buildColumns - 1));
    const std::size_t buildStart =
        std::min(static_cast<std::size_t>(std::max(0, tabScroll)),
                 visibleTools.size());
    const std::size_t displayedTools =
        std::min(visibleToolSlots, visibleTools.size() - buildStart);

    button(left, y, buildColumnWidth, buildButtonHeight, L"Inspect", [this] {
      tool = Tool::Inspect;
      buildPreview = {};
      previewValid = false;
      refreshUi();
      notice = L"Inspection tool selected.";
    }, tool == Tool::Inspect);

    for (std::size_t localIndex = 0; localIndex < displayedTools; ++localIndex) {
      const std::size_t slot = localIndex + 1;
      const int row = static_cast<int>(slot / static_cast<std::size_t>(buildColumns));
      const int column =
          static_cast<int>(slot % static_cast<std::size_t>(buildColumns));
      const auto &visible = visibleTools[buildStart + localIndex];
      const auto mappedTool = visible.tool;
      const auto *item = visible.item;
      button(left + column * (buildColumnWidth + controlGap),
             y + row * buildRowHeight, buildColumnWidth, buildButtonHeight,
             wide(item->name),
             [this, mappedTool] {
               tool = mappedTool;
               buildPreview = {};
               previewValid = false;
               refreshUi();
               notice = L"Move over the world to validate placement.";
             },
             tool == mappedTool);
    }

    const std::size_t slotCount = displayedTools + 1;
    const int usedRows = std::max(
        1, static_cast<int>((slotCount + static_cast<std::size_t>(buildColumns) - 1) /
                            static_cast<std::size_t>(buildColumns)));
    y += usedRows * buildRowHeight;

    if (visibleTools.size() > visibleToolSlots) {
      const std::size_t step = visibleToolSlots;
      button(left, y, halfControlWidth, std::max(px(22), vpx(30)),
             L"Previous tools", [this, step] {
               tabScroll = std::max(0, tabScroll - static_cast<int>(step));
             });
      button(left + halfControlWidth + controlGap, y, halfControlWidth,
             std::max(px(22), vpx(30)), L"More tools",
             [this, step, total = visibleTools.size()] {
               const int maximumStart =
                   total <= step ? 0 : static_cast<int>(total - step);
               tabScroll =
                   std::min(maximumStart, tabScroll + static_cast<int>(step));
             });
      y += std::max(px(28), vpx(39));
    }
    separator();

    const hh::frontend::BuildCatalogItem *selectedBuildItem = nullptr;
    if (tool != Tool::Inspect) {
      for (const auto &item : gameUi.buildCatalog) {
        const auto mappedTool = liveBuildTool(item.id);
        if (mappedTool && *mappedTool == tool) {
          selectedBuildItem = &item;
          break;
        }
      }
    }
    if (selectedBuildItem != nullptr) {
      label(L"Category", wide(selectedBuildItem->category));
      label(L"Base build cost", money(selectedBuildItem->costCents));
    }

    if (buildPreview.requestId == 0) {
      paragraph(tool == Tool::Inspect
                    ? L"Select a construction item to see its catalog cost and validate placement."
                    : L"Move the selected construction item over the hotel to validate placement.",
                52, Muted);
    } else {
      label(L"Preview", buildPreview.valid ? L"VALID" : L"REJECTED");
      if (!buildPreview.reasonCode.empty()) label(L"Reason", wide(buildPreview.reasonCode));
      if (!buildPreview.reasonText.empty()) paragraph(wide(buildPreview.reasonText), 40, buildPreview.valid ? Accent : Critical);
      if (y + vpx(38) < bottom)
        fullButton(L"Cancel construction", [this] {
          const auto result =
              ui.dispatchUiCommand(UiCommand{UiCommandType::BuildCancel});
          if (!result.message.empty())
            notice = wide(result.message);
        }, false, ButtonTone::Warning);
    }
  } else if (page == Page::Rooms || page == Page::Guests || page == Page::Staff ||
             page == Page::Supplies) {
    heading(page == Page::Rooms ? L"Room inspectors" : page == Page::Guests ? L"Guest inspectors" :
            page == Page::Staff ? L"Staff & departments" : L"Inventory & tasks");
    const auto matching = pageEntities(*this, page);
    const auto selectedEntity = std::find_if(gameUi.entities.begin(), gameUi.entities.end(),
        [this](const auto &entity) { return entity.id == selected; });
    if (selectedEntity != gameUi.entities.end() &&
        belongs(page, selectedEntity->kind)) {
      const int inspectorListReserve =
          matching.size() > 1 ? vpx(105) : vpx(45);
      const int detailBottom =
          std::max(y + vpx(56), bottom - inspectorListReserve);

      paragraph(wide(selectedEntity->title), 28);
      const bool departmentInspector =
          page == Page::Staff &&
          selectedEntity->kind == InspectorKind::Department;
      if (departmentInspector && y + vpx(38) < detailBottom) {
        paragraph(L"MANAGEMENT", 20, Muted);
      }
      if (departmentInspector && y + vpx(38) < detailBottom) {
        const auto departments = simulation.departments();
        const auto department = std::find_if(
            departments.begin(), departments.end(),
            [&](const auto &value) {
              return value.name == selectedEntity->title;
            });
        if (department != departments.end()) {
          const auto departmentId = department->id;
          if (department->managerId != 0 &&
              y + vpx(38) < detailBottom) {
            fullButton(L"Clear current manager", [this, departmentId] {
              const auto result = ui.dispatchUiCommand(
                  UiCommand{UiCommandType::AssignDepartmentManager, 0,
                            static_cast<std::int64_t>(departmentId)});
              notice =
                  result.message.empty()
                      ? (result.ok ? L"Department manager cleared."
                                   : L"Manager change rejected.")
                      : wide(result.message);
              refreshUi();
            });
          }
          for (const auto candidateId : department->directReports) {
            if (y + vpx(38) >= detailBottom)
              break;
            const auto person = std::find_if(
                snapshot.people.begin(), snapshot.people.end(),
                [&](const auto &value) { return value.id == candidateId; });
            const auto candidateName =
                person == snapshot.people.end()
                    ? std::to_wstring(candidateId)
                    : wide(person->name);
            fullButton(
                L"Assign " + candidateName + L" as manager",
                [this, departmentId, candidateId] {
                  const auto result = ui.dispatchUiCommand(
                      UiCommand{UiCommandType::AssignDepartmentManager,
                                candidateId,
                                static_cast<std::int64_t>(departmentId)});
                  notice =
                      result.message.empty()
                          ? (result.ok ? L"Department manager assigned."
                                       : L"Manager change rejected.")
                          : wide(result.message);
                  refreshUi();
                });
          }
        }
      }      for (const auto &field : selectedEntity->fields) {
        if (y + vpx(28) >= detailBottom)
          break;
        label(wide(field.label), wide(field.value));
      }
      for (const auto &diagnostic : selectedEntity->diagnostics) {
        if (y + vpx(42) >= detailBottom)
          break;
        paragraph(L"WHY · " + wide(diagnostic.code) + L" · " +
                      wide(diagnostic.message),
                  36, Warning);
      }



      if (y < detailBottom)
        separator();
    } else {
      paragraph(L"Select an item below to view its current status and any blockers.",
                48);
    }
    for (std::size_t index = static_cast<std::size_t>(tabScroll);
         index < matching.size() && y + vpx(38) < bottom; ++index) {
      const auto *entity = matching[index];
      fullButton(wide(entity->title), [this, id = entity->id] {
        const auto result = ui.dispatchUiCommand(UiCommand{UiCommandType::OpenInspector, id});
        if (!result.message.empty()) notice = wide(result.message);
      }, selected == entity->id);
    }
    if (matching.empty()) paragraph(L"Nothing in this category is available yet.", 54, Muted);
  } else if (page == Page::Operations) {
    heading(L"Operations command center");
    const auto &ops = operationsDashboard.snapshot();
    const bool compactOperations =
        panelLayout.contentHeightPixels < vpx(520);

    if (compactOperations) {
      label(L"Staff active / scheduled",
            std::to_wstring(ops.activeStaff) + L" / " +
                std::to_wstring(ops.scheduledStaff));
      label(L"HK / Eng / Service",
            std::to_wstring(ops.housekeepingBacklog) + L" / " +
                std::to_wstring(ops.engineeringOpenOrders) + L" / " +
                permille(ops.serviceLevelPermille));
    } else {
      label(L"Scheduled / active",
            std::to_wstring(ops.scheduledStaff) + L" / " +
                std::to_wstring(ops.activeStaff));
      label(L"Fatigued / break",
            std::to_wstring(ops.fatiguedStaff) + L" / " +
                std::to_wstring(ops.onBreakStaff));
      label(L"HK backlog", std::to_wstring(ops.housekeepingBacklog));
      label(L"Clean linen", std::to_wstring(ops.cleanLinenUnits));
      label(L"Incoming / blocked",
            std::to_wstring(ops.incomingOrders) + L" / " +
                std::to_wstring(ops.blockedInventoryMoves));
      label(L"Engineering", std::to_wstring(ops.engineeringOpenOrders));
      label(L"Room service / F&B",
            std::to_wstring(ops.roomServiceQueue) + L" / " +
                std::to_wstring(ops.foodTickets));
      label(L"Events", std::to_wstring(ops.eventCount));
      label(L"Amenity capacity",
            std::to_wstring(ops.amenityCapacityUsed) + L" / " +
                std::to_wstring(ops.amenityCapacityTotal));
      label(L"Service level", permille(ops.serviceLevelPermille));
    }
    separator();

    constexpr std::array<const wchar_t *, 9> filterNames{
        L"All", L"Staffing", L"Housekeeping", L"Laundry", L"Logistics",
        L"Engineering", L"F&B", L"Amenities", L"Events"};
    const int operationControlHeight = std::max(px(24), vpx(32));
    button(left, y, halfControlWidth, operationControlHeight,
           L"Filter · " +
               std::wstring(filterNames[static_cast<std::size_t>(operationsFilter)]),
           [this] {
             operationsFilter = (operationsFilter + 1) % 9;
             tabScroll = 0;
           },
           operationsFilter != 0);
    button(left + halfControlWidth + controlGap, y, halfControlWidth,
           operationControlHeight,
           L"Sort · " + operationSortLabel(operationsSort),
           [this] {
             using hh::frontend::OperationSort;
             operationsSort =
                 operationsSort == OperationSort::SchedulerOrder
                     ? OperationSort::PriorityHighFirst
                     : operationsSort == OperationSort::PriorityHighFirst
                           ? OperationSort::OldestFirst
                           : OperationSort::SchedulerOrder;
             tabScroll = 0;
           },
           operationsSort != hh::frontend::OperationSort::SchedulerOrder);
    y += std::max(px(28), vpx(39));
    separator();

    auto drawOperation = [&](const auto &row) {
      if (compactOperations) {
        if (y + vpx(52) >= bottom)
          return false;
        std::wstring summary = wide(row.name) + L" · " + wide(row.state);
        if (!row.reasonCode.empty())
          summary += L" · " + wide(row.reasonCode);
        paragraph(summary, 36, row.reasonCode.empty() ? Ink : Warning);
        separator();
        return true;
      }

      if (y + vpx(82) >= bottom)
        return false;
      paragraph(wide(row.name) + L" · " + wide(row.state), 24);
      if (!row.reasonCode.empty())
        paragraph(L"WHY · " + wide(row.reasonCode), 24, Warning);
      separator();
      return true;
    };

    const auto rows = operationsDashboard.filteredWindow(
        operationFilter(operationsFilter), operationsSort,
        static_cast<std::size_t>(tabScroll), 128);
    bool anyRows = false;
    for (const auto &row : rows) {
      if (!drawOperation(row))
        break;
      anyRows = true;
    }
    if (!anyRows)
      paragraph(L"No active operational rows for this filter.", 36, Muted);
  } else if (page == Page::Finance) {
    heading(L"Revenue & finance");
    const auto &economy = gameUi.economy;
    const auto &kpi = economy.kpis;
    constexpr std::array<const wchar_t *, 5> financeLabels{
        L"Overview", L"Revenue", L"Market", L"Controls", L"Risk"};
    const int financeGap = px(5);
    const int financeWidth =
        std::max(1, (panelWidth - financeGap * 4) /
                         static_cast<int>(financeLabels.size()));
    for (std::size_t index = 0; index < financeLabels.size(); ++index) {
      button(left + static_cast<int>(index) * (financeWidth + financeGap), y,
             financeWidth, std::max(px(22), vpx(29)), financeLabels[index],
             [this, index] {
               financeView = static_cast<FinanceView>(index);
               tabScroll = 0;
             },
             static_cast<std::size_t>(financeView) == index);
    }
    y += std::max(px(28), vpx(39));
    separator();

    std::size_t financeCursor = 0;
    auto financeParagraph = [&](const std::wstring &value, int h,
                                COLORREF color) {
      const std::size_t row = financeCursor++;
      if (row < static_cast<std::size_t>(std::max(0, tabScroll)))
        return;
      if (y + vpx(h) >= bottom)
        return;
      paragraph(value, h, color);
    };
    auto financeLabel = [&](const std::wstring &name,
                            const std::wstring &value) {
      const std::size_t row = financeCursor++;
      if (row < static_cast<std::size_t>(std::max(0, tabScroll)))
        return;
      if (y + vpx(28) >= bottom)
        return;
      label(name, value);
    };
    auto fields = [&](const std::wstring &name, const auto &values) {
      if (values.empty())
        return;
      financeParagraph(name, 22, Muted);
      for (const auto &field : values)
        financeLabel(wide(field.label), wide(field.value));
    };
    auto dispatchFinance = [&](auto command, const std::wstring &unavailable) {
      if (!command) {
        notice = unavailable;
        return;
      }
      const auto result = ui.dispatchUiCommand(*command);
      economyDashboard.applyCommandResult(result);
      notice = result.message.empty()
                   ? (result.ok ? L"Finance change applied." : L"Finance change rejected.")
                   : wide(result.message);
      refreshUi();
    };

    switch (financeView) {
    case FinanceView::Overview:
      financeLabel(L"Occupancy today", permille(kpi.todayOccupancyPermille));
      financeLabel(L"7d / 30d",
                   permille(kpi.sevenDayOccupancyPermille) + L" / " +
                       permille(kpi.thirtyDayOccupancyPermille));
      financeLabel(L"ADR", money(kpi.adrCents));
      financeLabel(L"RevPAR / TRevPAR",
                   money(kpi.revParCents) + L" / " + money(kpi.trevParCents));
      financeLabel(L"GOP", money(kpi.gopCents));
      financeLabel(L"Room / total rev",
                   money(kpi.roomRevenueCents) + L" / " +
                       money(kpi.totalRevenueCents));
      financeLabel(L"Labor / utilities",
                   money(kpi.laborCostCents) + L" / " +
                       money(kpi.utilitiesCostCents));
      financeLabel(L"F&B cost", money(kpi.foodCostCents));
      financeLabel(L"Cash", money(kpi.cashCents));
      financeLabel(L"Cash runway",
                   std::to_wstring(kpi.cashRunwayDays) + L" days");
      break;
    case FinanceView::Revenue:
      fields(L"Department contribution", economy.departmentContribution);
      fields(L"Booking pace", economy.bookingPace);
      fields(L"Cancellations / no-shows", economy.cancellationAndNoShow);
      fields(L"Channel mix", economy.channelMix);
      fields(L"Future rates", economy.futureRateCalendar);
      if (economy.departmentContribution.empty() && economy.bookingPace.empty() &&
          economy.futureRateCalendar.empty())
        paragraph(L"No revenue diagnostics are available yet.", 44, Muted);
      break;
    case FinanceView::Market:
      fields(L"Competitors", economy.competitors);
      fields(L"Demand by segment", economy.demandBySegment);
      if (economy.competitors.empty() && economy.demandBySegment.empty())
        paragraph(L"No market comparison data is available yet.", 44, Muted);
      break;
    case FinanceView::Controls: {
      const bool compactFinanceControls =
          panelLayout.contentHeightPixels < vpx(480);
      const int financeControlTabHeight = std::max(px(24), vpx(32));
      button(left, y, halfControlWidth, financeControlTabHeight, L"Pricing",
             [this] {
               financeControlView = FinanceControlView::Pricing;
               tabScroll = 0;
             },
             financeControlView == FinanceControlView::Pricing);
      button(left + halfControlWidth + controlGap, y, halfControlWidth,
             financeControlTabHeight, L"Overbooking", [this] {
               financeControlView = FinanceControlView::Overbooking;
               tabScroll = 0;
             },
             financeControlView == FinanceControlView::Overbooking);
      y += std::max(px(28), vpx(39));
      separator();

      if (!compactFinanceControls) {
        paragraph(
            financeControlView == FinanceControlView::Pricing
                ? L"Adjust the active room-rate rule. Changes use the existing pricing policy."
                : L"Adjust the standard-room overbooking allowance. Other policy fields are read-only.",
            44, Muted);
      }

      if (financeControlView == FinanceControlView::Pricing) {
        if (economy.pricingRules.empty()) {
          paragraph(L"No pricing rule is available to adjust.", 34, Muted);
          break;
        }

        financeRuleIndex =
            std::min(financeRuleIndex, economy.pricingRules.size() - 1);
        const auto &rule = economy.pricingRules[financeRuleIndex];
        const std::wstring ruleSelector =
            compactFinanceControls
                ? L"Rule " + std::to_wstring(financeRuleIndex + 1) + L" / " +
                      std::to_wstring(economy.pricingRules.size()) + L" · " +
                      wide(rule.roomCategory) + L" · " + money(rule.rateCents)
                : L"Pricing rule " + std::to_wstring(financeRuleIndex + 1) +
                      L" / " + std::to_wstring(economy.pricingRules.size());
        fullButton(ruleSelector, [this, count = economy.pricingRules.size()] {
          financeRuleIndex = (financeRuleIndex + 1) % count;
        });

        if (!compactFinanceControls) {
          label(L"Category", wide(rule.roomCategory));
          label(L"Days", std::to_wstring(rule.startDay) + L"–" +
                             std::to_wstring(rule.endDay));
          label(L"Rate", money(rule.rateCents));
        }

        if (y + vpx(38) < bottom) {
          const int controlHeight = std::max(px(24), vpx(32));
          button(left, y, halfControlWidth, controlHeight, L"Rate − $5",
                 [this, dispatchFinance] {
                   dispatchFinance(
                       economyDashboard.adjustPricingRuleCommand(
                           financeRuleIndex, -500),
                       L"This pricing rule cannot be reduced safely.");
                 });
          button(left + halfControlWidth + controlGap, y, halfControlWidth,
                 controlHeight, L"Rate + $5", [this, dispatchFinance] {
                   dispatchFinance(
                       economyDashboard.adjustPricingRuleCommand(
                           financeRuleIndex, 500),
                       L"This pricing rule cannot be increased safely.");
                 });
          y += std::max(px(28), vpx(39));
        }
      } else {
        if (economy.overbookingPolicies.empty()) {
          paragraph(L"No overbooking policy is available.", 34, Muted);
          break;
        }

        financeOverbookingIndex = std::min(
            financeOverbookingIndex, economy.overbookingPolicies.size() - 1);
        const auto &policy =
            economy.overbookingPolicies[financeOverbookingIndex];
        const std::wstring policySelector =
            compactFinanceControls
                ? L"Policy " + std::to_wstring(financeOverbookingIndex + 1) +
                      L" / " +
                      std::to_wstring(economy.overbookingPolicies.size()) +
                      L" · " + wide(policy.roomCategory) + L" · +" +
                      std::to_wstring(policy.allowance)
                : L"Overbooking " +
                      std::to_wstring(financeOverbookingIndex + 1) + L" / " +
                      std::to_wstring(economy.overbookingPolicies.size());
        fullButton(
            policySelector,
            [this, count = economy.overbookingPolicies.size()] {
              financeOverbookingIndex =
                  (financeOverbookingIndex + 1) % count;
            });

        if (!compactFinanceControls) {
          label(L"Category", wide(policy.roomCategory));
          label(L"Allowance", std::to_wstring(policy.allowance));
          label(L"Relocation", money(policy.relocationCompensationCents));
          label(L"Days", std::to_wstring(policy.startDay) + L"–" +
                             std::to_wstring(policy.endDay));
        }

        if (policy.roomCategory == "standard" && y + vpx(38) < bottom) {
          const int controlHeight = std::max(px(24), vpx(32));
          button(left, y, halfControlWidth, controlHeight, L"Allowance − 1",
                 [this, dispatchFinance] {
                   dispatchFinance(
                       economyDashboard.adjustOverbookingCommand(
                           financeOverbookingIndex, -1),
                       L"Overbooking allowance cannot be reduced further.");
                 });
          button(left + halfControlWidth + controlGap, y, halfControlWidth,
                 controlHeight, L"Allowance + 1",
                 [this, dispatchFinance] {
                   dispatchFinance(
                       economyDashboard.adjustOverbookingCommand(
                           financeOverbookingIndex, 1),
                       L"Overbooking allowance cannot be increased safely.");
                 });
          y += std::max(px(28), vpx(39));
        } else if (policy.roomCategory != "standard") {
          paragraph(
              L"This policy is read-only here. Choose the standard-room policy to change its allowance.",
              compactFinanceControls ? 28 : 44, Muted);
        }
      }
      break;
    }
    case FinanceView::Risk:
      fields(L"Debt schedule", economy.debtSchedule);
      for (const auto &diagnostic : economy.financingDiagnostics)
        financeParagraph(L"WHY · " + wide(diagnostic.code) + L" · " +
                             wide(diagnostic.message),
                         36, Warning);
      fields(L"Active campaigns", economy.campaigns);
      fields(L"Accepted contracts", economy.contracts);
      if (economy.debtSchedule.empty() && economy.financingDiagnostics.empty() &&
          economy.campaigns.empty() && economy.contracts.empty())
        paragraph(L"No financing or commercial-risk data is available yet.", 44,
                  Muted);
      break;
    }
  } else if (page == Page::Alerts) {
    heading(L"Alerts & why chains");
    label(L"Active", std::to_wstring(alertCenter.active().size()));
    label(L"Resolved", std::to_wstring(alertCenter.resolvedHistory().size())); separator();
    if (selectedAlertId != 0) {
      const auto chain = alertCenter.causalChain(selectedAlertId);
      const int alertListReserve =
          alertCenter.active().size() > 1 ? vpx(110) : vpx(45);
      const int chainBottom =
          std::max(y + vpx(48), bottom - alertListReserve);
      for (const auto &alert : chain) {
        if (y + vpx(38) >= chainBottom)
          break;
        paragraph(alertSeverity(alert.severity) + L" · " +
                      wide(alert.reasonCode) + L" · " + wide(alert.message),
                  34, alertColor(alert.severity));
      }
      if (const auto navigation =
              alertCenter.navigationFor(selectedAlertId);
          navigation && y + vpx(38) < chainBottom) {
        fullButton(L"Focus source", [this, command = *navigation] {
          const auto result = ui.dispatchUiCommand(command);
          if (!result.message.empty())
            notice = wide(result.message);
        });
      }
      if (y < chainBottom)
        separator();
    }
    const auto &alerts = alertCenter.active();
    for (std::size_t index = static_cast<std::size_t>(tabScroll); index < alerts.size() && y + vpx(82) < bottom; ++index) {
      const auto &alert = alerts[index];
      paragraph(alertSeverity(alert.severity) + L" · " + wide(alert.reasonCode), 22, alertColor(alert.severity));
      paragraph(wide(alert.message), 34);
      fullButton(L"Show why chain", [this, id = alert.id] { selectedAlertId = id; tabScroll = 0; }, selectedAlertId == alert.id); separator();
    }
    if (alerts.empty()) {
      paragraph(L"ALL CLEAR", 24, Accent);
      paragraph(L"No active alerts require attention.", 36, Muted);
    }
  } else if (page == Page::Objectives) {
    heading(L"Objectives & guidance");
    fullButton(objectiveUi.minimized() ? L"Expand guidance" : L"Minimize guidance", [this] { objectiveUi.setMinimized(!objectiveUi.minimized()); });
    if (!objectiveUi.minimized()) {
      const auto &items = objectiveUi.items();
      for (std::size_t index = static_cast<std::size_t>(tabScroll); index < items.size() && y + vpx(92) < bottom; ++index) {
        const auto &objective = items[index];
        paragraph((objective.complete ? L"COMPLETE · " : L"ACTIVE · ") + wide(objective.title), 24, objective.complete ? Accent : Ink);
        label(L"Progress", std::to_wstring(objective.current) + L" / " + std::to_wstring(objective.target));
        if (!objective.reasonCode.empty()) paragraph(L"WHY · " + wide(objective.reasonCode) + L" · " + wide(objective.reasonText), 34, Warning);
        if (objective.dismissible && y + vpx(38) < bottom) fullButton(L"Dismiss guidance", [this, id = objective.id] { if (!objectiveUi.dismiss(id)) notice = L"Guidance cannot be dismissed."; });
        separator();
      }
      if (items.empty()) {
        paragraph(L"NO ACTIVE GUIDANCE", 24, Accent);
        paragraph(L"The hotel has no scenario objectives requiring attention.", 40, Muted);
      }
    }
  } else if (page == Page::Overlays) {
    heading(L"Management overlays");
    paragraph(L"Choose an overlay to inspect hotel conditions. Each view shows its legend and units; unavailable data is labeled.", 56);
    const auto descriptors = hh::frontend::OverlayModel::requiredDescriptors();
    for (std::size_t index = static_cast<std::size_t>(tabScroll); index < descriptors.size() && y + vpx(40) < bottom; ++index) {
      const auto &descriptor = descriptors[index];
      const auto data = std::find_if(gameUi.overlays.begin(), gameUi.overlays.end(), [&descriptor](const auto &view) { return view.id == descriptor.id; });
      const bool available = data != gameUi.overlays.end();
      fullButton((available ? L"" : L"[NO DATA] ") + wide(descriptor.label), [this, id = descriptor.id] {
        const auto result = ui.dispatchUiCommand(UiCommand{UiCommandType::SetOverlay, 0, static_cast<std::int64_t>(id)});
        notice = result.message.empty() ? (result.ok ? L"Overlay selected." : L"Overlay unavailable.") : wide(result.message);
      }, managementOverlay == descriptor.id);
      if (managementOverlay == descriptor.id && y + vpx(70) < bottom) {
        label(L"Legend", wide(descriptor.lowSemantic) + L" → " + wide(descriptor.highSemantic));
        label(L"Range / unit", std::to_wstring(descriptor.minValue) + L"–" + std::to_wstring(descriptor.maxValue) + L" " + wide(descriptor.unit));
        if (available && selected != 0) {
          const auto sample = std::find_if(data->samples.begin(), data->samples.end(), [this](const auto &value) { return value.entityId == selected; });
          if (sample != data->samples.end()) label(L"Selected exact", wide(sample->exactText) + L" · " + wide(sample->semanticText));
        }
      }
    }
  } else if (page == Page::Settings) {
    heading(L"UI & accessibility");

    const int settingsTabHeight = std::max(px(24), vpx(32));
    button(left, y, halfControlWidth, settingsTabHeight, L"Display", [this] {
      settingsView = SettingsView::Display;
      tabScroll = 0;
      keyBindingEditor.cancel();
    }, settingsView == SettingsView::Display);
    button(left + halfControlWidth + controlGap, y, halfControlWidth,
           settingsTabHeight, L"Controls", [this] {
             settingsView = SettingsView::Controls;
             tabScroll = 0;
           }, settingsView == SettingsView::Controls);
    y += std::max(px(28), vpx(39));
    separator();

    if (settingsView == SettingsView::Display) {
      label(L"UI scale", std::to_wstring(uiSettings.scalePercent()) + L"%");
      const int scaleButtonGap = px(5);
      const int scaleButtonWidth =
          std::max(1, (panelWidth -
                       scaleButtonGap *
                           (static_cast<int>(Final07UiScales.size()) - 1)) /
                          static_cast<int>(Final07UiScales.size()));
      const int scaleButtonHeight = std::max(px(22), vpx(30));
      for (std::size_t index = 0; index < Final07UiScales.size(); ++index) {
        const int value = Final07UiScales[index];
        button(left + static_cast<int>(index) *
                          (scaleButtonWidth + scaleButtonGap),
               y, scaleButtonWidth, scaleButtonHeight,
               std::to_wstring(value) + L"%", [this, value] {
                 if (!uiSettings.setScalePercent(value)) {
                   notice = L"Unsupported UI scale.";
                   return;
                 }
                 notice =
                     saveUiPreferences()
                         ? L"UI scale preference updated."
                         : L"UI scale updated for this session; preference file could not be saved.";
               },
               uiSettings.scalePercent() == value);
      }
      y += std::max(px(28), vpx(39));

      fullButton(
          uiSettings.reducedMotion() ? L"Reduced motion: ON"
                                     : L"Reduced motion: OFF",
          [this] {
            uiSettings.setReducedMotion(!uiSettings.reducedMotion());
            const bool persisted = saveUiPreferences();
            if (!persisted) {
              notice = L"Reduced-motion preference changed for this session; preference file could not be saved.";
            } else {
              notice = uiSettings.reducedMotion()
                           ? L"Reduced motion enabled."
                           : L"Reduced motion disabled.";
            }
          },
          uiSettings.reducedMotion());

      if (y + vpx(28) < bottom)
        label(L"Visible focus",
              uiSettings.visibleFocusRequired() ? L"Required"
                                                : L"Mouse modality");
      if (y + vpx(48) < bottom)
        paragraph(
            L"Color is never the only cue. Text keeps extra room for longer labels. These options affect presentation only.",
            40, Muted);
    } else {
      const bool compactControls =
          panelLayout.contentHeightPixels < vpx(430);
      if (!compactControls) {
        paragraph(
            keyBindingEditor.capturing()
                ? L"Press the replacement key, or click the highlighted action to cancel."
                : L"Choose an action, then press its replacement key.",
            34, keyBindingEditor.capturing() ? Warning : Muted);
      } else if (keyBindingEditor.capturing()) {
        paragraph(L"Press a replacement key.", 24, Warning);
      }

      fullButton(L"Reset keyboard bindings", [this] {
        keyBindingEditor.cancel();
        uiSettings.resetKeyboardBindings();
        notice =
            saveUiPreferences()
                ? L"Keyboard bindings reset to defaults."
                : L"Keyboard bindings reset for this session; preference file could not be saved.";
      });

      const auto pendingBinding = keyBindingEditor.pendingAction();
      const int bindingGap = px(12);
      const int bindingWidth = (panelWidth - bindingGap) / 2;
      const int bindingHeight = std::max(px(22), vpx(30));
      const int bindingRowHeight = vpx(35);
      const std::size_t bindingStart = std::min(
          static_cast<std::size_t>(std::max(0, tabScroll)),
          hh::frontend::EditableKeyBindings.size());

      for (std::size_t index = bindingStart;
           index < hh::frontend::EditableKeyBindings.size(); ++index) {
        const std::size_t localIndex = index - bindingStart;
        const int row = static_cast<int>(localIndex / 2);
        const int column = static_cast<int>(localIndex % 2);
        const int buttonY = y + row * bindingRowHeight;
        if (buttonY + bindingHeight >= bottom)
          break;

        const auto &descriptor = hh::frontend::EditableKeyBindings[index];
        const bool capturing =
            pendingBinding && *pendingBinding == descriptor.action;
        std::wstring bindingLabel =
            wide(std::string(descriptor.label)) + L" · " +
            keyName(uiSettings.keyboardBinding(descriptor.action));
        if (capturing)
          bindingLabel = L"PRESS · " + bindingLabel;

        button(left + column * (bindingWidth + bindingGap), buttonY,
               bindingWidth, bindingHeight, std::move(bindingLabel),
               [this, action = descriptor.action] {
                 const auto pending = keyBindingEditor.pendingAction();
                 if (pending && *pending == action) {
                   keyBindingEditor.cancel();
                   notice = L"Keyboard rebinding cancelled.";
                   return;
                 }
                 keyBindingEditor.begin(action);
                 notice = L"Press a key for the selected action.";
               },
               capturing);
      }
    }
  } else {
    heading(L"Your first hotel");
    paragraph(L"Inspect the hotel, follow Operations and Alerts, then use Finance and Overlays to understand why service succeeds or fails.", 58);
    const std::array<const wchar_t *, 5> steps{
        L"1   Inspect a room and read exact readiness diagnostics.",
        L"2   Start at 1× and watch arrivals, queues and alerts.",
        L"3   Open Operations for housekeeping, logistics and service.",
        L"4   Overlays always show legends; unavailable data is labeled.",
        L"5   Track Finance and Objectives before expanding."};
    for (const auto *step : steps) { if (y + vpx(52) >= bottom - vpx(70)) break; paragraph(step, 44); }
    if (y + vpx(38) < bottom) fullButton(L"Start operating · 1×", [this] {
      const auto result = ui.dispatchUiCommand(UiCommand{UiCommandType::SetSimulationSpeed, 0, 1});
      if (!result.message.empty()) notice = wide(result.message); page = Page::Rooms;
    });
    if (y + vpx(38) < bottom)
      fullButton(L"New starter campaign", [this] { newCampaign(); }, false,
                 ButtonTone::Warning);
  }

  if (page != Page::Build && page != Page::Guide) {
    const std::size_t count = pageRows(*this);
    if (count > 0) {
      const int scrollStep = pageScrollStep(*this);
      button(left, height - FooterHeight - vpx(42), halfControlWidth,
             std::max(px(20), vpx(28)), L"Previous",
             [this, scrollStep] {
               tabScroll = std::max(0, tabScroll - scrollStep);
             });
      button(left + halfControlWidth + controlGap,
             height - FooterHeight - vpx(42), halfControlWidth,
             std::max(px(20), vpx(28)), L"Next",
             [this, count, scrollStep] {
               const int maximum = static_cast<int>(count - 1);
               tabScroll =
                   std::min(maximum, tabScroll + scrollStep);
             });
    }
  }

  const std::size_t scrollCount = pageRows(*this);
  if (scrollCount > 6) {
    const int railX = width - px(8);
    const int railTop = panelLayout.contentTopPixels;
    const int railBottom = panelLayout.contentBottomPixels;
    const int railHeight = std::max(1, railBottom - railTop);
    const int thumbHeight = std::min(railHeight, std::max(vpx(24), railHeight / 6));
    const int maximumScroll = static_cast<int>(scrollCount - 1);
    const int clampedScroll = std::clamp(tabScroll, 0, maximumScroll);
    const int thumbTravel = std::max(0, railHeight - thumbHeight);
    const int thumbY =
        railTop + (maximumScroll == 0
                       ? 0
                       : thumbTravel * clampedScroll / maximumScroll);
    fill(dc, {railX, railTop, railX + std::max(1, px(1)), railBottom}, Line);
    fill(dc, {railX - px(1), thumbY, railX + px(2), thumbY + thumbHeight},
         Accent);
  }

  const int footerY = height - FooterHeight + px(9);
  const int footerControlHeight = std::max(px(22), vpx(31));
  const int footerGap = px(5);
  int footerX = px(15);
  const int speedWidth = px(47);
  for (std::size_t index = 0; index < Final07SpeedButtons.size(); ++index) {
    const int value = Final07SpeedButtons[index];
    button(footerX, footerY, speedWidth, footerControlHeight,
           value == 0 ? L"Pause" : std::to_wstring(value) + L"×",
           [this, value] {
             const auto result = ui.dispatchUiCommand(
                 UiCommand{UiCommandType::SetSimulationSpeed, 0, value});
             if (!result.message.empty())
               notice = wide(result.message);
             refreshUi();
           },
           speed == value);
    footerX += speedWidth + footerGap;
  }

  footerX += px(19);
  const int floorButtonWidth = px(35);
  const int floorLabelWidth = px(65);
  button(footerX, footerY, floorButtonWidth, footerControlHeight, L"−",
         [this] { changeFloor(floor - 1); });
  footerX += floorButtonWidth + px(7);
  drawText(dc, small, L"Floor " + std::to_wstring(hud.activeFloor), footerX,
           footerY + vpx(7), floorLabelWidth, std::max(px(16), vpx(24)), ChromeText,
           DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
  footerX += floorLabelWidth + px(7);
  button(footerX, footerY, floorButtonWidth, footerControlHeight, L"+",
         [this] { changeFloor(floor + 1); });
  footerX += floorButtonWidth + px(17);

  const int wallsWidth = px(102);
  button(footerX, footerY, wallsWidth, footerControlHeight, L"Walls", [this] {
    wallMode = static_cast<hh::renderer::WallRenderMode>(
        (static_cast<int>(wallMode) + 1) % 3);
    refreshUi();
  });
  footerX += wallsWidth + px(8);

  const int overlayWidth = px(103);
  button(footerX, footerY, overlayWidth, footerControlHeight, L"Overlays [O]",
         [this] {
           page = Page::Overlays;
           tabScroll = 0;
         });
  footerX += overlayWidth + px(14);

  drawText(dc, small,
           overlayName(managementOverlay) + L" · " + wide(hud.currentTool) +
               (hud.cutaway ? L" · cutaway" : L" · full walls"),
           footerX, footerY + vpx(7),
           std::max(1, width - footerX - px(20)), std::max(px(15), vpx(22)), ChromeMuted,
           DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);

  drawText(dc, small, notice, px(22), HeaderHeight - px(20),
           std::max(1, sidebarX - px(44)), px(18), ChromeMuted,
           DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);

  focusedButton =
      final07ValidatedControlIndex(focusedButton, buttons.size());
  hoveredButton =
      final07ValidatedControlIndex(hoveredButton, buttons.size());
  if (hoveredButton < 0)
    SetCursor(LoadCursorW(nullptr, IDC_ARROW));

  BitBlt(output, 0, 0, width, HeaderHeight, dc, 0, 0, SRCCOPY);
  BitBlt(output, sidebarX, HeaderHeight, SidebarWidth, height - HeaderHeight - FooterHeight,
         dc, sidebarX, HeaderHeight, SRCCOPY);
  BitBlt(output, 0, height - FooterHeight, width, FooterHeight, dc, 0,
         height - FooterHeight, SRCCOPY);
  SelectObject(dc, previous);
  DeleteObject(bitmap);
  DeleteDC(dc);
}
} // namespace hh::client
