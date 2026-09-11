#include "Client.h"
#include "hh/frontend/EconomyDashboard.h"
#include "hh/frontend/OverlayModel.h"
#include <algorithm>
#include <array>
#include <iomanip>
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

constexpr COLORREF Bg = RGB(239, 235, 223);
constexpr COLORREF Panel = RGB(251, 248, 239);
constexpr COLORREF Ink = RGB(36, 56, 52);
constexpr COLORREF Muted = RGB(113, 121, 105);
constexpr COLORREF Accent = RGB(42, 105, 92);
constexpr COLORREF Warning = RGB(150, 102, 31);
constexpr COLORREF Critical = RGB(145, 55, 50);
constexpr COLORREF Line = RGB(216, 216, 200);

void fill(HDC dc, RECT rect, COLORREF color) {
  HBRUSH brush = CreateSolidBrush(color);
  FillRect(dc, &rect, brush);
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
      L"Build", L"Rooms", L"Staff", L"Guests", L"Supplies", L"Ops",
      L"Finance", L"Alerts", L"Objectives", L"Overlays", L"Settings", L"Guide"};
  return labels[static_cast<std::size_t>(page)];
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

std::size_t pageRows(const Client &client) {
  switch (client.page) {
  case Page::Rooms:
  case Page::Guests:
  case Page::Staff:
  case Page::Supplies: return pageEntities(client, client.page).size();
  case Page::Operations:
    if (client.operationsFilter <= 0)
      return client.operationsDashboard.snapshot().rows.size();
    return client.operationsDashboard.filteredCount(
        static_cast<OperationArea>(client.operationsFilter - 1));
  case Page::Alerts: return client.alertCenter.active().size();
  case Page::Objectives: return client.objectiveUi.items().size();
  case Page::Overlays:
    return hh::frontend::OverlayModel::requiredDescriptors().size();
  case Page::Build:
  case Page::Finance:
  case Page::Settings:
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

void Client::paint(HDC output) {
  objectiveUi.update(ui.snapshot().objectives);
  HDC dc = CreateCompatibleDC(output);
  HBITMAP bitmap = CreateCompatibleBitmap(output, width, height);
  HGDIOBJ previous = SelectObject(dc, bitmap);
  fill(dc, {0, 0, width, height}, Bg);
  buttons.clear();

  auto button = [&](int x, int y, int w, int h, std::wstring value,
                    std::function<void()> action, bool active = false) {
    RECT rect{x, y, x + w, y + h};
    fill(dc, rect, active ? Accent : Panel);
    drawText(dc, small, value, x + 5, y + 7, w - 10, h - 8,
             active ? RGB(250, 247, 236) : Ink, DT_CENTER | DT_SINGLELINE);
    buttons.push_back({rect, std::move(value), std::move(action), active});
  };

  const auto &gameUi = ui.snapshot();
  const auto &hud = gameUi.hud;
  drawText(dc, title, L"HOTEL HAVEN", 22, 14, 205, 30);
  drawText(dc, small, L"PROPERTY MANAGEMENT", 23, 49, 205, 20, Muted);
  drawText(dc, small, L"CASH", 245, 15, 90, 18, Muted);
  drawText(dc, number, money(hud.cashCents), 245, 36, 160, 30);
  drawText(dc, small, L"OCCUPANCY", 412, 15, 112, 18, Muted);
  drawText(dc, number, permille(hud.occupancyPermille), 412, 36, 115, 30);
  drawText(dc, small, L"SAT / REP", 535, 15, 120, 18, Muted);
  drawText(dc, normal, permille(hud.satisfactionPermille) + L" / " +
                         permille(hud.reputationPermille),
           535, 39, 160, 26);
  drawText(dc, small, L"DAY " + std::to_wstring(hud.day + 1), 704, 15, 95, 18,
           Muted);
  std::wostringstream clock;
  clock << std::setw(2) << std::setfill(L'0') << hud.hour << L":" << std::setw(2)
        << hud.minute;
  drawText(dc, number, clock.str(), 704, 36, 98, 30);

  button(width - 278, 19, 88, 34,
         L"Alerts " + std::to_wstring(hud.alertCount),
         [this] { page = Page::Alerts; tabScroll = 0; });
  button(width - 182, 19, 76, 34, L"Save [F5]", [this] {
    const auto result = ui.dispatchUiCommand(UiCommand{UiCommandType::SaveGame});
    if (!result.message.empty()) notice = wide(result.message);
  });
  button(width - 98, 19, 76, 34, L"Load [F9]", [this] {
    const auto result = ui.dispatchUiCommand(UiCommand{UiCommandType::LoadGame});
    if (!result.message.empty()) notice = wide(result.message);
  });

  const int sidebarX = width - SidebarWidth;
  fill(dc, {sidebarX, HeaderHeight, width, height - FooterHeight}, Panel);
  for (int index = 0; index < 12; ++index) {
    const auto target = static_cast<Page>(index);
    const int row = index / 4;
    const int column = index % 4;
    button(sidebarX + 12 + column * 83, HeaderHeight + 12 + row * 35, 79, 30,
           pageLabel(target), [this, target] { page = target; tabScroll = 0; },
           page == target);
  }

  const int left = sidebarX + 20;
  const int panelWidth = SidebarWidth - 40;
  int y = HeaderHeight + 126;
  const int bottom = height - FooterHeight - 52;
  auto heading = [&](const std::wstring &value) {
    drawText(dc, title, value, left, y, panelWidth, 32); y += 43;
  };
  auto paragraph = [&](const std::wstring &value, int h = 48,
                       COLORREF color = Ink) {
    drawText(dc, normal, value, left, y, panelWidth, h, color); y += h + 9;
  };
  auto label = [&](const std::wstring &name, const std::wstring &value) {
    drawText(dc, small, name, left, y, 174, 24, Muted);
    drawText(dc, normal, value, left + 174, y, panelWidth - 174, 24, Ink,
             DT_RIGHT | DT_SINGLELINE | DT_END_ELLIPSIS); y += 28;
  };
  auto separator = [&] { fill(dc, {left, y, left + panelWidth, y + 1}, Line); y += 15; };
  auto fullButton = [&](std::wstring value, std::function<void()> action,
                        bool active = false) {
    button(left, y, panelWidth, 32, std::move(value), std::move(action), active); y += 39;
  };

  if (page == Page::Build) {
    heading(L"Build & construction");
    paragraph(L"Move a selected tool over the hotel to request authoritative placement validation. Click confirms only the latest valid preview.", 64);
    const std::array<const wchar_t *, 13> tools{
        L"Inspect", L"Guest room", L"Floor", L"Wall", L"Door", L"Entrance",
        L"Reception desk", L"Supply closet", L"Stairs", L"Remove tile",
        L"Bathroom", L"Staff room", L"Lobby"};
    for (int index = 0; index < 13; ++index) {
      const int row = index / 2, column = index % 2;
      button(left + column * 163, y + row * 35, 151, 30, tools[index],
             [this, index] {
               tool = static_cast<Tool>(index); buildPreview = {}; previewValid = false;
               refreshUi(); notice = L"Move over the world to validate placement.";
             }, static_cast<int>(tool) == index);
    }
    y += 7 * 35; separator();
    if (buildPreview.requestId == 0) {
      paragraph(L"No preview yet. Cost/material/labor details stay unavailable until FINAL-01 exposes them through its authoritative preview seam.", 62, Muted);
    } else {
      label(L"Preview", buildPreview.valid ? L"VALID" : L"REJECTED");
      if (!buildPreview.reasonCode.empty()) label(L"Reason", wide(buildPreview.reasonCode));
      if (!buildPreview.reasonText.empty()) paragraph(wide(buildPreview.reasonText), 40, buildPreview.valid ? Accent : Critical);
      if (y + 38 < bottom) fullButton(L"Cancel construction", [this] {
        const auto result = ui.dispatchUiCommand(UiCommand{UiCommandType::BuildCancel});
        if (!result.message.empty()) notice = wide(result.message);
      });
    }
  } else if (page == Page::Rooms || page == Page::Guests || page == Page::Staff ||
             page == Page::Supplies) {
    heading(page == Page::Rooms ? L"Room inspectors" : page == Page::Guests ? L"Guest inspectors" :
            page == Page::Staff ? L"Staff & departments" : L"Inventory & tasks");
    const auto matching = pageEntities(*this, page);
    const auto selectedEntity = std::find_if(gameUi.entities.begin(), gameUi.entities.end(),
        [this](const auto &entity) { return entity.id == selected; });
    if (selectedEntity != gameUi.entities.end() && belongs(page, selectedEntity->kind)) {
      paragraph(wide(selectedEntity->title), 28);
      for (const auto &field : selectedEntity->fields) {
        if (y + 28 >= bottom) break;
        label(wide(field.label), wide(field.value));
      }
      for (const auto &diagnostic : selectedEntity->diagnostics) {
        if (y + 42 >= bottom) break;
        paragraph(L"WHY · " + wide(diagnostic.code) + L" · " + wide(diagnostic.message), 36, Warning);
      }
      separator();
    } else paragraph(L"Select an item below to inspect authoritative fields and reason codes.", 48);
    for (std::size_t index = static_cast<std::size_t>(tabScroll);
         index < matching.size() && y + 38 < bottom; ++index) {
      const auto *entity = matching[index];
      fullButton(wide(entity->title), [this, id = entity->id] {
        const auto result = ui.dispatchUiCommand(UiCommand{UiCommandType::OpenInspector, id});
        if (!result.message.empty()) notice = wide(result.message);
      }, selected == entity->id);
    }
    if (matching.empty()) paragraph(L"No matching authoritative entities are exposed on this integration branch.", 54, Muted);
  } else if (page == Page::Operations) {
    heading(L"Operations command center");
    const auto &ops = operationsDashboard.snapshot();
    label(L"Scheduled / active", std::to_wstring(ops.scheduledStaff) + L" / " + std::to_wstring(ops.activeStaff));
    label(L"Fatigued / break", std::to_wstring(ops.fatiguedStaff) + L" / " + std::to_wstring(ops.onBreakStaff));
    label(L"HK backlog", std::to_wstring(ops.housekeepingBacklog));
    label(L"Clean linen", std::to_wstring(ops.cleanLinenUnits));
    label(L"Incoming / blocked", std::to_wstring(ops.incomingOrders) + L" / " + std::to_wstring(ops.blockedInventoryMoves));
    label(L"Engineering", std::to_wstring(ops.engineeringOpenOrders));
    label(L"Room service / F&B", std::to_wstring(ops.roomServiceQueue) + L" / " + std::to_wstring(ops.foodTickets));
    label(L"Events", std::to_wstring(ops.eventCount));
    label(L"Amenity capacity", std::to_wstring(ops.amenityCapacityUsed) + L" / " + std::to_wstring(ops.amenityCapacityTotal));
    label(L"Service level", permille(ops.serviceLevelPermille)); separator();
    constexpr std::array<const wchar_t *, 9> filterNames{
        L"All", L"Staffing", L"Housekeeping", L"Laundry", L"Logistics",
        L"Engineering", L"F&B", L"Amenities", L"Events"};
    fullButton(L"Filter · " + std::wstring(filterNames[static_cast<std::size_t>(operationsFilter)]),
               [this] { operationsFilter = (operationsFilter + 1) % 9; tabScroll = 0; },
               operationsFilter != 0);
    separator();
    auto drawOperation = [&](const auto &row) {
      if (y + 58 >= bottom)
        return false;
      paragraph(wide(row.name) + L" · " + wide(row.state), 24);
      if (!row.reasonCode.empty()) paragraph(L"WHY · " + wide(row.reasonCode), 24, Warning);
      separator();
      return true;
    };
    bool anyRows = false;
    if (operationsFilter == 0) {
      const auto rows = operationsDashboard.window(static_cast<std::size_t>(tabScroll), 128);
      for (const auto &row : rows) {
        if (!drawOperation(row)) break;
        anyRows = true;
      }
    } else {
      const auto rows = operationsDashboard.filteredWindow(
          static_cast<OperationArea>(operationsFilter - 1),
          static_cast<std::size_t>(tabScroll), 128);
      for (const auto &row : rows) {
        if (!drawOperation(row)) break;
        anyRows = true;
      }
    }
    if (!anyRows) paragraph(L"No active operational rows for this filter.", 36, Muted);
  } else if (page == Page::Finance) {
    heading(L"Revenue & finance");
    const auto &economy = gameUi.economy;
    const auto &kpi = economy.kpis;
    constexpr std::array<const wchar_t *, 5> financeLabels{
        L"Overview", L"Revenue", L"Market", L"Controls", L"Risk"};
    for (std::size_t index = 0; index < financeLabels.size(); ++index) {
      button(left + static_cast<int>(index) * 63, y, 58, 29, financeLabels[index],
             [this, index] { financeView = static_cast<FinanceView>(index); tabScroll = 0; },
             static_cast<std::size_t>(financeView) == index);
    }
    y += 39;
    separator();

    auto fields = [&](const std::wstring &name, const auto &values) {
      if (values.empty() || y + 40 >= bottom) return;
      paragraph(name, 22, Muted);
      for (const auto &field : values) {
        if (y + 28 >= bottom) break;
        label(wide(field.label), wide(field.value));
      }
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
      label(L"Occupancy today", permille(kpi.todayOccupancyPermille));
      label(L"7d / 30d", permille(kpi.sevenDayOccupancyPermille) + L" / " + permille(kpi.thirtyDayOccupancyPermille));
      label(L"ADR", money(kpi.adrCents));
      label(L"RevPAR / TRevPAR", money(kpi.revParCents) + L" / " + money(kpi.trevParCents));
      label(L"GOP", money(kpi.gopCents));
      label(L"Room / total rev", money(kpi.roomRevenueCents) + L" / " + money(kpi.totalRevenueCents));
      label(L"Labor / utilities", money(kpi.laborCostCents) + L" / " + money(kpi.utilitiesCostCents));
      label(L"F&B cost", money(kpi.foodCostCents));
      label(L"Cash", money(kpi.cashCents));
      label(L"Cash runway", std::to_wstring(kpi.cashRunwayDays) + L" days");
      break;
    case FinanceView::Revenue:
      fields(L"Department contribution", economy.departmentContribution);
      fields(L"Booking pace", economy.bookingPace);
      fields(L"Cancellations / no-shows", economy.cancellationAndNoShow);
      fields(L"Channel mix", economy.channelMix);
      fields(L"Future rates", economy.futureRateCalendar);
      if (economy.departmentContribution.empty() && economy.bookingPace.empty() &&
          economy.futureRateCalendar.empty())
        paragraph(L"No authoritative revenue diagnostics are available yet.", 44, Muted);
      break;
    case FinanceView::Market:
      fields(L"Competitors", economy.competitors);
      fields(L"Demand by segment", economy.demandBySegment);
      if (economy.competitors.empty() && economy.demandBySegment.empty())
        paragraph(L"No authoritative market comparison data is available yet.", 44, Muted);
      break;
    case FinanceView::Controls:
      paragraph(L"Controls modify only existing FINAL-06 pricing rules and the standard overbooking policy. FINAL-07 does not manufacture offers or recovery policy fields.", 66, Muted);
      if (economy.pricingRules.empty()) {
        paragraph(L"No authoritative pricing rule exists to adjust.", 34, Muted);
      } else {
        financeRuleIndex = std::min(financeRuleIndex, economy.pricingRules.size() - 1);
        const auto &rule = economy.pricingRules[financeRuleIndex];
        fullButton(L"Pricing rule " + std::to_wstring(financeRuleIndex + 1) + L" / " +
                       std::to_wstring(economy.pricingRules.size()),
                   [this, count = economy.pricingRules.size()] {
                     financeRuleIndex = (financeRuleIndex + 1) % count;
                   });
        label(L"Category", wide(rule.roomCategory));
        label(L"Days", std::to_wstring(rule.startDay) + L"–" + std::to_wstring(rule.endDay));
        label(L"Rate", money(rule.rateCents));
        if (y + 38 < bottom) {
          button(left, y, 151, 32, L"Rate − $5", [this] {
            dispatchFinance(economyDashboard.adjustPricingRuleCommand(financeRuleIndex, -500),
                            L"This pricing rule cannot be reduced safely.");
          });
          button(left + 163, y, 151, 32, L"Rate + $5", [this] {
            dispatchFinance(economyDashboard.adjustPricingRuleCommand(financeRuleIndex, 500),
                            L"This pricing rule cannot be increased safely.");
          });
          y += 39;
        }
      }
      separator();
      if (economy.overbookingPolicies.empty()) {
        paragraph(L"No authoritative overbooking policy is available.", 34, Muted);
      } else {
        financeOverbookingIndex = std::min(financeOverbookingIndex, economy.overbookingPolicies.size() - 1);
        const auto &policy = economy.overbookingPolicies[financeOverbookingIndex];
        fullButton(L"Overbooking " + std::to_wstring(financeOverbookingIndex + 1) + L" / " +
                       std::to_wstring(economy.overbookingPolicies.size()),
                   [this, count = economy.overbookingPolicies.size()] {
                     financeOverbookingIndex = (financeOverbookingIndex + 1) % count;
                   });
        label(L"Category", wide(policy.roomCategory));
        label(L"Allowance", std::to_wstring(policy.allowance));
        label(L"Relocation", money(policy.relocationCompensationCents));
        label(L"Days", std::to_wstring(policy.startDay) + L"–" + std::to_wstring(policy.endDay));
        if (policy.roomCategory == "standard" && y + 38 < bottom) {
          button(left, y, 151, 32, L"Allowance − 1", [this] {
            dispatchFinance(economyDashboard.adjustOverbookingCommand(financeOverbookingIndex, -1),
                            L"Overbooking allowance cannot be reduced further.");
          });
          button(left + 163, y, 151, 32, L"Allowance + 1", [this] {
            dispatchFinance(economyDashboard.adjustOverbookingCommand(financeOverbookingIndex, 1),
                            L"Overbooking allowance cannot be increased safely.");
          });
          y += 39;
        } else if (policy.roomCategory != "standard") {
          paragraph(L"This policy is read-only because the FINAL-06 application adapter currently owns only the standard-room overbooking command.", 54, Muted);
        }
      }
      break;
    case FinanceView::Risk:
      fields(L"Debt schedule", economy.debtSchedule);
      for (const auto &diagnostic : economy.financingDiagnostics) {
        if (y + 42 >= bottom) break;
        paragraph(L"WHY · " + wide(diagnostic.code) + L" · " + wide(diagnostic.message), 36, Warning);
      }
      fields(L"Active campaigns", economy.campaigns);
      fields(L"Accepted contracts", economy.contracts);
      if (economy.debtSchedule.empty() && economy.financingDiagnostics.empty() &&
          economy.campaigns.empty() && economy.contracts.empty())
        paragraph(L"No authoritative financing or commercial-risk state is available.", 44, Muted);
      break;
    }
  } else if (page == Page::Alerts) {
    heading(L"Alerts & why chains");
    label(L"Active", std::to_wstring(alertCenter.active().size()));
    label(L"Resolved", std::to_wstring(alertCenter.resolvedHistory().size())); separator();
    if (selectedAlertId != 0) {
      const auto chain = alertCenter.causalChain(selectedAlertId);
      for (const auto &alert : chain) {
        if (y + 38 >= bottom) break;
        paragraph(alertSeverity(alert.severity) + L" · " + wide(alert.reasonCode) + L" · " + wide(alert.message), 34, alertColor(alert.severity));
      }
      if (const auto navigation = alertCenter.navigationFor(selectedAlertId); navigation && y + 38 < bottom)
        fullButton(L"Focus source", [this, command = *navigation] { const auto result = ui.dispatchUiCommand(command); if (!result.message.empty()) notice = wide(result.message); });
      separator();
    }
    const auto &alerts = alertCenter.active();
    for (std::size_t index = static_cast<std::size_t>(tabScroll); index < alerts.size() && y + 82 < bottom; ++index) {
      const auto &alert = alerts[index];
      paragraph(alertSeverity(alert.severity) + L" · " + wide(alert.reasonCode), 22, alertColor(alert.severity));
      paragraph(wide(alert.message), 34);
      fullButton(L"Show why chain", [this, id = alert.id] { selectedAlertId = id; tabScroll = 0; }, selectedAlertId == alert.id); separator();
    }
    if (alerts.empty()) paragraph(L"No active alerts.", 36, Muted);
  } else if (page == Page::Objectives) {
    heading(L"Objectives & guidance");
    fullButton(objectiveUi.minimized() ? L"Expand guidance" : L"Minimize guidance", [this] { objectiveUi.setMinimized(!objectiveUi.minimized()); });
    if (!objectiveUi.minimized()) {
      const auto &items = objectiveUi.items();
      for (std::size_t index = static_cast<std::size_t>(tabScroll); index < items.size() && y + 92 < bottom; ++index) {
        const auto &objective = items[index];
        paragraph((objective.complete ? L"COMPLETE · " : L"ACTIVE · ") + wide(objective.title), 24, objective.complete ? Accent : Ink);
        label(L"Progress", std::to_wstring(objective.current) + L" / " + std::to_wstring(objective.target));
        if (!objective.reasonCode.empty()) paragraph(L"WHY · " + wide(objective.reasonCode) + L" · " + wide(objective.reasonText), 34, Warning);
        if (objective.dismissible && y + 38 < bottom) fullButton(L"Dismiss guidance", [this, id = objective.id] { if (!objectiveUi.dismiss(id)) notice = L"Guidance cannot be dismissed."; });
        separator();
      }
      if (items.empty()) paragraph(L"No authoritative scenario objectives are exposed by the current application layer.", 54, Muted);
    }
  } else if (page == Page::Overlays) {
    heading(L"Management overlays");
    paragraph(L"All 18 required overlays are listed with text legends and exact units. Missing simulation data is never inferred.", 56);
    const auto descriptors = hh::frontend::OverlayModel::requiredDescriptors();
    for (std::size_t index = static_cast<std::size_t>(tabScroll); index < descriptors.size() && y + 40 < bottom; ++index) {
      const auto &descriptor = descriptors[index];
      const auto data = std::find_if(gameUi.overlays.begin(), gameUi.overlays.end(), [&descriptor](const auto &view) { return view.id == descriptor.id; });
      const bool available = data != gameUi.overlays.end();
      fullButton((available ? L"" : L"[NO DATA] ") + wide(descriptor.label), [this, id = descriptor.id] {
        const auto result = ui.dispatchUiCommand(UiCommand{UiCommandType::SetOverlay, 0, static_cast<std::int64_t>(id)});
        notice = result.message.empty() ? (result.ok ? L"Overlay selected." : L"Overlay unavailable.") : wide(result.message);
      }, managementOverlay == descriptor.id);
      if (managementOverlay == descriptor.id && y + 70 < bottom) {
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
    label(L"UI scale", std::to_wstring(uiSettings.scalePercent()) + L"%");
    for (std::size_t index = 0; index < Final07UiScales.size(); ++index) {
      const int value = Final07UiScales[index]; const int row = static_cast<int>(index / 3), column = static_cast<int>(index % 3);
      button(left + column * 106, y + row * 35, 98, 30, std::to_wstring(value) + L"%", [this, value] {
        notice = uiSettings.setScalePercent(value) ? L"UI scale preference updated." : L"Unsupported UI scale.";
      }, uiSettings.scalePercent() == value);
    }
    y += 78;
    fullButton(uiSettings.reducedMotion() ? L"Reduced motion: ON" : L"Reduced motion: OFF", [this] { uiSettings.setReducedMotion(!uiSettings.reducedMotion()); }, uiSettings.reducedMotion());
    label(L"Visible focus", uiSettings.visibleFocusRequired() ? L"Required" : L"Mouse modality");
    label(L"Color-only information", L"Never"); label(L"Localization reserve", L"140% string expansion");
    paragraph(L"Scaling/focus/reduced-motion preferences are presentation state only and never alter simulation authority.", 54, Muted);
  } else {
    heading(L"Your first hotel");
    paragraph(L"Inspect the hotel, follow Operations and Alerts, then use Finance and Overlays to understand why service succeeds or fails.", 58);
    const std::array<const wchar_t *, 5> steps{
        L"1   Inspect a room and read exact readiness diagnostics.",
        L"2   Start at 1× and watch arrivals, queues and alerts.",
        L"3   Open Operations for housekeeping, logistics and service.",
        L"4   Overlays always show legends; unavailable data is labeled.",
        L"5   Track Finance and Objectives before expanding."};
    for (const auto *step : steps) { if (y + 52 >= bottom - 70) break; paragraph(step, 44); }
    if (y + 38 < bottom) fullButton(L"Start operating · 1×", [this] {
      const auto result = ui.dispatchUiCommand(UiCommand{UiCommandType::SetSimulationSpeed, 0, 1});
      if (!result.message.empty()) notice = wide(result.message); page = Page::Rooms;
    });
    if (y + 38 < bottom) fullButton(L"New starter campaign", [this] { newCampaign(); });
  }

  if (page != Page::Build && page != Page::Finance && page != Page::Settings && page != Page::Guide) {
    const std::size_t count = pageRows(*this);
    button(left, height - FooterHeight - 42, 151, 28, L"Previous", [this] { tabScroll = std::max(0, tabScroll - 1); });
    button(left + 163, height - FooterHeight - 42, 151, 28, L"Next", [this, count] {
      const int maximum = count == 0 ? 0 : static_cast<int>(count - 1); tabScroll = std::min(maximum, tabScroll + 1);
    });
  }

  const int footerY = height - FooterHeight + 9;
  for (std::size_t index = 0; index < Final07SpeedButtons.size(); ++index) {
    const int value = Final07SpeedButtons[index];
    button(15 + static_cast<int>(index) * 52, footerY, 47, 31,
           value == 0 ? L"Pause" : std::to_wstring(value) + L"×", [this, value] {
      const auto result = ui.dispatchUiCommand(UiCommand{UiCommandType::SetSimulationSpeed, 0, value});
      if (!result.message.empty()) notice = wide(result.message); refreshUi();
    }, speed == value);
  }
  button(291, footerY, 35, 31, L"−", [this] { changeFloor(floor - 1); });
  drawText(dc, small, L"Floor " + std::to_wstring(hud.activeFloor), 333, footerY + 7, 65, 24);
  button(401, footerY, 35, 31, L"+", [this] { changeFloor(floor + 1); });
  button(453, footerY, 102, 31, L"Walls", [this] {
    wallMode = static_cast<hh::renderer::WallRenderMode>((static_cast<int>(wallMode) + 1) % 3); refreshUi();
  });
  button(563, footerY, 103, 31, L"Overlays [O]", [this] { page = Page::Overlays; tabScroll = 0; });
  drawText(dc, small, overlayName(managementOverlay) + L" · " + wide(hud.currentTool) +
                         (hud.cutaway ? L" · cutaway" : L" · full walls"),
           680, footerY + 7, width - 700, 22, Muted, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
  drawText(dc, small, notice, 22, HeaderHeight - 19, sidebarX - 40, 19, Accent,
           DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);

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
