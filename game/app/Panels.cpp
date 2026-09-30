#include "Client.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>
namespace hh::client {
using namespace hh::game;
namespace {
constexpr COLORREF Bg = RGB(239, 235, 223), Panel = RGB(251, 248, 239),
                   Ink = RGB(36, 56, 52), Muted = RGB(113, 121, 105),
                   Accent = RGB(42, 105, 92), Line = RGB(216, 216, 200);
void fill(HDC dc, RECT r, COLORREF c) {
  HBRUSH b = CreateSolidBrush(c);
  FillRect(dc, &r, b);
  DeleteObject(b);
}
void text(HDC dc, HFONT font, std::wstring value, int x, int y, int w, int h,
          COLORREF color = Ink, UINT format = DT_LEFT | DT_WORDBREAK) {
  SelectObject(dc, font);
  SetTextColor(dc, color);
  SetBkMode(dc, TRANSPARENT);
  RECT r{x, y, x + w, y + h};
  DrawTextW(dc, value.c_str(), -1, &r, format | DT_NOPREFIX);
}
std::wstring pct(double v) {
  return std::to_wstring(static_cast<int>(std::round(v))) + L"%";
}
std::wstring role(PersonKind k) {
  switch (k) {
  case PersonKind::Receptionist:
    return L"Reception";
  case PersonKind::Housekeeper:
    return L"Housekeeping";
  case PersonKind::Maintenance:
    return L"Maintenance";
  default:
    return L"Guest";
  }
}
std::wstring taskName(TaskKind k) {
  switch (k) {
  case TaskKind::CheckIn:
    return L"Check-in";
  case TaskKind::CheckOut:
    return L"Check-out";
  case TaskKind::Turnover:
    return L"Room turnover";
  case TaskKind::Restock:
    return L"Supply pickup";
  case TaskKind::Repair:
    return L"Maintenance";
  }
  return L"Task";
}
std::wstring guestArchetypeName(GuestArchetype archetype) {
  switch (archetype) {
  case GuestArchetype::BudgetLeisure: return L"Budget leisure";
  case GuestArchetype::Backpacker: return L"Backpacker";
  case GuestArchetype::BusinessTraveler: return L"Business traveler";
  case GuestArchetype::ExecutiveBusiness: return L"Executive business";
  case GuestArchetype::CoupleLeisure: return L"Couple leisure";
  case GuestArchetype::FamilyLeisure: return L"Family leisure";
  case GuestArchetype::LuxuryLeisure: return L"Luxury leisure";
  case GuestArchetype::ConferenceDelegate: return L"Conference delegate";
  case GuestArchetype::GroupTourTraveler: return L"Group tour";
  case GuestArchetype::AirportTransitTraveler: return L"Airport transit";
  case GuestArchetype::WellnessTraveler: return L"Wellness traveler";
  case GuestArchetype::VipCelebrity: return L"VIP";
  case GuestArchetype::CriticReviewer: return L"Critic reviewer";
  case GuestArchetype::Count: break;
  }
  return L"Guest";
}
std::wstring guestLifecycleName(GuestLifecycleState state) {
  switch (state) {
  case GuestLifecycleState::Prospective: return L"Prospective";
  case GuestLifecycleState::Reserved: return L"Reserved";
  case GuestLifecycleState::TravelingToHotel: return L"Traveling to hotel";
  case GuestLifecycleState::Arriving: return L"Arriving";
  case GuestLifecycleState::AwaitingCheckIn: return L"Waiting for check-in";
  case GuestLifecycleState::CheckedIn: return L"Checked in";
  case GuestLifecycleState::InStay: return L"In stay";
  case GuestLifecycleState::PreparingCheckout: return L"Preparing checkout";
  case GuestLifecycleState::AwaitingCheckout: return L"Waiting for checkout";
  case GuestLifecycleState::Departing: return L"Departing";
  case GuestLifecycleState::CompletedStay: return L"Completed stay";
  case GuestLifecycleState::Cancelled: return L"Cancelled";
  case GuestLifecycleState::NoShow: return L"No show";
  case GuestLifecycleState::WalkedRelocated: return L"Relocated";
  case GuestLifecycleState::Count: break;
  }
  return L"Unknown";
}
std::wstring guestGoalName(GuestGoal goal) {
  switch (goal) {
  case GuestGoal::ReachHotel: return L"Reach hotel";
  case GuestGoal::CheckIn: return L"Check in";
  case GuestGoal::ReachRoom: return L"Reach room";
  case GuestGoal::Sleep: return L"Sleep";
  case GuestGoal::Eat: return L"Eat";
  case GuestGoal::Drink: return L"Drink";
  case GuestGoal::Bathe: return L"Bathe";
  case GuestGoal::Work: return L"Work";
  case GuestGoal::Exercise: return L"Exercise";
  case GuestGoal::Swim: return L"Swim";
  case GuestGoal::Socialize: return L"Socialize";
  case GuestGoal::Relax: return L"Relax";
  case GuestGoal::AttendEvent: return L"Attend event";
  case GuestGoal::RequestService: return L"Request service";
  case GuestGoal::ResolveComplaint: return L"Resolve complaint";
  case GuestGoal::Checkout: return L"Check out";
  case GuestGoal::LeaveHotel: return L"Leave hotel";
  case GuestGoal::Count: break;
  }
  return L"No current goal";
}
std::wstring guestCategoryName(GuestCategory category) {
  switch (category) {
  case GuestCategory::Room: return L"Room";
  case GuestCategory::Cleanliness: return L"Cleanliness";
  case GuestCategory::Service: return L"Service";
  case GuestCategory::Food: return L"Food";
  case GuestCategory::Amenities: return L"Amenities";
  case GuestCategory::Quiet: return L"Quiet";
  case GuestCategory::Convenience: return L"Convenience";
  case GuestCategory::Value: return L"Value";
  case GuestCategory::ArrivalDeparture: return L"Arrival / departure";
  case GuestCategory::Count: break;
  }
  return L"Category";
}
std::wstring decimal(double value) {
  std::wostringstream out;
  out << std::fixed << std::setprecision(1) << value;
  return out.str();
}
} // namespace
std::wstring wide(const std::string &s) {
  if (s.empty())
    return {};
  int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                              nullptr, 0);
  std::wstring out(static_cast<std::size_t>(n), L' ');
  MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                      out.data(), n);
  return out;
}
std::wstring money(std::int64_t cents) {
  std::wostringstream out;
  out << L"$" << std::fixed << std::setprecision(2)
      << static_cast<double>(cents) / 100;
  return out.str();
}
std::wstring roomStatus(RoomStatus st) {
  switch (st) {
  case RoomStatus::VacantReady:
    return L"Ready to sell";
  case RoomStatus::Reserved:
    return L"Reserved";
  case RoomStatus::Occupied:
    return L"Occupied";
  case RoomStatus::VacantDirty:
    return L"Needs cleaning";
  case RoomStatus::Cleaning:
    return L"Cleaning in progress";
  case RoomStatus::OutOfOrder:
    return L"Out of service";
  default:
    return L"Incomplete";
  }
}
std::wstring personState(PersonState st) {
  switch (st) {
  case PersonState::OffDuty:
    return L"Off duty";
  case PersonState::Traveling:
    return L"Walking";
  case PersonState::Working:
    return L"Working";
  case PersonState::Waiting:
    return L"Waiting";
  case PersonState::Sleeping:
    return L"Sleeping";
  case PersonState::CheckedOut:
    return L"Departed";
  default:
    return L"Available";
  }
}
void Client::paint(HDC output) {
  HDC dc = CreateCompatibleDC(output);
  HBITMAP bitmap = CreateCompatibleBitmap(output, width, height);
  HGDIOBJ old = SelectObject(dc, bitmap);
  fill(dc, {0, 0, width, height}, Bg);
  buttons.clear();
  auto button = [&](int x, int y, int w, int h, std::wstring label,
                    std::function<void()> action, bool active = false) {
    RECT r{x, y, x + w, y + h};
    fill(dc, r, active ? Accent : Panel);
    text(dc, small, label, x + 5, y + 7, w - 10, h - 8,
         active ? RGB(250, 247, 236) : Ink, DT_CENTER | DT_SINGLELINE);
    buttons.push_back({r, std::move(label), std::move(action), active});
  };
  text(dc, title, L"HOTEL HAVEN", 22, 14, 290, 30);
  text(dc, small, L"PROPERTY MANAGEMENT", 23, 49, 220, 20, Muted);
  text(dc, small, L"AVAILABLE CASH", 306, 15, 175, 20, Muted);
  text(dc, number, money(snapshot.economy.cashCents), 306, 36, 185, 31);
  text(dc, small, L"OCCUPANCY", 505, 15, 120, 20, Muted);
  text(dc, number, pct(snapshot.economy.occupancy * 100), 505, 36, 120, 31);
  text(dc, small, L"REPUTATION", 643, 15, 140, 20, Muted);
  text(dc, number, pct(snapshot.economy.reputation), 643, 36, 130, 31);
  text(dc, small, L"DAY " + std::to_wstring(snapshot.day + 1), 795, 15, 100, 20,
       Muted);
  const int minute = static_cast<int>((snapshot.elapsedSeconds / 60) % 60);
  std::wostringstream clock;
  clock << std::setw(2) << std::setfill(L'0') << snapshot.hour << L":"
        << std::setw(2) << minute;
  text(dc, number, clock.str(), 795, 36, 100, 31);
  const int sx = width - SidebarWidth;
  button(width - 174, 19, 72, 34, L"Save [F5]", [this] { save(); });
  button(width - 94, 19, 72, 34, L"Load [F9]", [this] { load(); });
  fill(dc, {sx, HeaderHeight, width, height - FooterHeight}, Panel);
  const std::array<std::wstring, 7> names = {L"Build",  L"Rooms",    L"Staff",
                                             L"Guests", L"Supplies", L"Finance",
                                             L"Guide"};
  for (int i = 0; i < 7; ++i) {
    const int row = i / 4, col = i % 4;
    button(
        sx + 12 + col * 83, HeaderHeight + 12 + row * 35, 79, 30,
        names[static_cast<std::size_t>(i)],
        [this, i] {
          page = static_cast<Page>(i);
          tabScroll = 0;
        },
        static_cast<int>(page) == i);
  }
  const int left = sx + 20, pw = SidebarWidth - 40;
  int y = HeaderHeight + 96;
  auto heading = [&](std::wstring str) {
    text(dc, title, str, left, y, pw, 32);
    y += 43;
  };
  auto paragraph = [&](std::wstring str, int h = 48) {
    text(dc, normal, str, left, y, pw, h);
    y += h + 9;
  };
  auto label = [&](std::wstring key, std::wstring value) {
    text(dc, small, key, left, y, 190, 24, Muted);
    text(dc, normal, value, left + 190, y, pw - 190, 25, Ink,
         DT_RIGHT | DT_SINGLELINE);
    y += 28;
  };
  auto separator = [&] {
    fill(dc, {left, y, left + pw, y + 1}, Line);
    y += 15;
  };
  auto fullButton = [&](std::wstring label, std::function<void()> fn,
                        bool active = false) {
    button(left, y, pw, 32, std::move(label), std::move(fn), active);
    y += 39;
  };
  const int bottom = height - FooterHeight - 52;
  if (page == Page::Build) {
    heading(L"Build your property");
    paragraph(L"Choose a tool, then click the hotel. Rooms include furniture "
              L"and a bathroom. Connect the door to the entrance.",
              64);
    const std::array<std::wstring, 13> labels = {L"Inspect / select",
                                                 L"Room · 6 × 6",
                                                 L"Corridor / floor",
                                                 L"Wall",
                                                 L"Door",
                                                 L"Guest entrance",
                                                 L"Reception desk",
                                                 L"Supply closet",
                                                 L"Stairs",
                                                 L"Remove tile",
                                                 L"Bathroom tile",
                                                 L"Staff room tile",
                                                 L"Lobby tile"};
    for (int i = 0; i < 13; ++i) {
      button(
          left + (i % 2) * 163, y + (i / 2) * 35, 151, 30,
          labels[static_cast<std::size_t>(i)],
          [this, i] {
            tool = static_cast<Tool>(i);
            notice = L"Click a tile to use the selected tool.";
          },
          static_cast<int>(tool) == i);
    }
    y += 7 * 35;
    if (y + 55 < bottom)
      paragraph(L"Stairs connect matching coordinates on adjacent floors. "
                L"Invalid construction shows its reason below.",
                56);
  } else if (page == Page::Rooms) {
    heading(L"Rooms & service");
    const auto room =
        std::find_if(snapshot.rooms.begin(), snapshot.rooms.end(),
                     [&](const auto &r) { return r.id == selected; });
    if (room != snapshot.rooms.end()) {
      const auto r = *room;
      paragraph(wide(r.name) + L" · floor " + std::to_wstring(r.floor), 27);
      label(L"Status", roomStatus(r.status));
      label(L"Cleanliness", pct(r.cleanliness));
      label(L"Condition", pct(r.condition));
      label(L"Nightly rate", money(r.nightlyRateCents));
      paragraph(r.reachable ? L"Connected to the guest entrance."
                            : L"No route to the entrance. Connect the door "
                              L"with corridor tiles.",
                42);
      button(left, y, 151, 31, L"Rate − $10", [this, r] {
        result(simulation.setRoomRate(
            r.id,
            std::max<std::int64_t>(2000, r.nightlyRateCents - 1000) / 100.0));
      });
      button(left + 163, y, 151, 31, L"Rate + $10", [this, r] {
        result(
            simulation.setRoomRate(r.id, (r.nightlyRateCents + 1000) / 100.0));
      });
      y += 40;
      button(left, y, 151, 31, L"Request clean",
             [this, r] { result(simulation.requestClean(r.id)); });
      button(left + 163, y, 151, 31, L"Request repair",
             [this, r] { result(simulation.requestRepair(r.id)); });
      y += 40;
      fullButton(r.closed                             ? L"Reopen room"
                 : r.status == RoomStatus::OutOfOrder ? L"Repair required"
                                                      : L"Close room",
                 [this, r] {
                   if (r.status == RoomStatus::OutOfOrder && !r.closed)
                     result(simulation.requestRepair(r.id));
                   else
                     result(simulation.closeRoom(r.id, !r.closed));
                 });
      fullButton(L"Demolish selected room", [this, r] {
        if (MessageBoxW(window,
                        L"Remove this room? Occupied or reserved rooms cannot "
                        L"be demolished.",
                        L"Demolish room", MB_YESNO | MB_ICONQUESTION) == IDYES)
          result(simulation.removeRoom(r.id));
      });
      separator();
    } else
      paragraph(L"Select a room on the map or in the list below.", 38);
    for (std::size_t i = static_cast<std::size_t>(tabScroll);
         i < snapshot.rooms.size() && y + 35 < bottom; ++i) {
      const auto r = snapshot.rooms[i];
      fullButton(
          wide(r.name) + L" · " + roomStatus(r.status),
          [this, r] {
            selected = r.id;
            changeFloor(r.floor);
            camera.setTarget({static_cast<float>(r.x + r.width / 2),
                              static_cast<float>(r.floor) * 3.2f,
                              static_cast<float>(r.y + r.height / 2)});
          },
          selected == r.id);
    }
  } else if (page == Page::Staff) {
    heading(L"Staff & shifts");
    paragraph(L"Hire an 8-hour shift beginning now. Staff travel to work and "
              L"collect supplies before servicing rooms.",
              60);
    for (int i = 0; i < 3; ++i) {
      const auto k = static_cast<PersonKind>(i + 1);
      fullButton(L"Hire " + role(k), [this, k] {
        StaffHire h;
        h.name = "Employee " + std::to_string(snapshot.people.size() + 1);
        h.role = k;
        h.shiftStartHour = snapshot.hour;
        h.shiftEndHour = (snapshot.hour + 8) % 24;
        h.hourlyWage = k == PersonKind::Maintenance ? 24. : 18.;
        result(simulation.hireStaff(h));
      });
    }
    separator();
    std::vector<PersonView> staff;
    for (const auto &p : snapshot.people)
      if (p.kind != PersonKind::Guest)
        staff.push_back(p);
    for (std::size_t i = static_cast<std::size_t>(tabScroll);
         i < staff.size() && y + 125 < bottom; ++i) {
      const auto p = staff[i];
      paragraph(wide(p.name) + L" · " + role(p.kind), 25);
      label(personState(p.state) + L" · fatigue", pct(p.fatigue));
      label(L"Wage / shift", money(p.hourlyWageCents) + L"/h · " +
                                 std::to_wstring(p.shiftStartHour) + L":00–" +
                                 std::to_wstring(p.shiftEndHour) + L":00");
      button(left, y, 151, 29, L"Move shift +8h", [this, p] {
        result(simulation.setStaffShift(p.id, (p.shiftStartHour + 8) % 24,
                                        (p.shiftEndHour + 8) % 24));
      });
      button(left + 163, y, 151, 29, L"Dismiss",
             [this, p] { result(simulation.fireStaff(p.id)); });
      y += 40;
      separator();
    }
  } else if (page == Page::Guests) {
    heading(L"Guest inspector");
    const auto isTerminal = [](GuestLifecycleState state) {
      return state == GuestLifecycleState::CompletedStay ||
             state == GuestLifecycleState::Cancelled ||
             state == GuestLifecycleState::NoShow ||
             state == GuestLifecycleState::WalkedRelocated;
    };
    std::vector<const GuestView *> guests;
    for (const auto &guest : snapshot.guests)
      guests.push_back(&guest);
    const auto activeCount = std::count_if(
        guests.begin(), guests.end(), [&](const auto *guest) {
          return !isTerminal(guest->lifecycle);
        });
    if (guests.empty())
      paragraph(
          L"Reservations are generated hourly from price and reputation. The "
          L"15:00 arrivals need a ready room, accessible reception, and a "
          L"receptionist on duty.",
          84);
    else {
      auto selectedIt = std::find_if(guests.begin(), guests.end(),
                                     [&](const auto *guest) {
                                       return guest->profile.id == selectedGuest;
                                     });
      if (selectedIt == guests.end()) {
        selectedIt = std::find_if(guests.begin(), guests.end(),
                                  [&](const auto *guest) {
                                    return !isTerminal(guest->lifecycle);
                                  });
        if (selectedIt == guests.end())
          selectedIt = guests.begin();
        selectedGuest = (*selectedIt)->profile.id;
      }
      const auto selectedIndex =
          static_cast<std::size_t>(std::distance(guests.begin(), selectedIt));
      label(L"Guests on property", std::to_wstring(activeCount) + L" · " +
                                        std::to_wstring(selectedIndex + 1) +
                                        L" of " +
                                        std::to_wstring(guests.size()));
      button(left, y, 151, 30, L"Previous guest", [this, selectedIndex] {
        if (snapshot.guests.empty())
          return;
        const auto next = selectedIndex == 0 ? snapshot.guests.size() - 1
                                             : selectedIndex - 1;
        selectedGuest = snapshot.guests[next].profile.id;
        guestInspectorTab = GuestInspectorTab::Overview;
      });
      button(left + 163, y, 151, 30, L"Next guest", [this, selectedIndex] {
        if (snapshot.guests.empty())
          return;
        selectedGuest = snapshot.guests[(selectedIndex + 1) %
                                        snapshot.guests.size()]
                            .profile.id;
        guestInspectorTab = GuestInspectorTab::Overview;
      });
      y += 35;
      button(left, y, 100, 30, L"Overview",
             [this] { guestInspectorTab = GuestInspectorTab::Overview; },
             guestInspectorTab == GuestInspectorTab::Overview);
      button(left + 104, y, 100, 30, L"Memories",
             [this] { guestInspectorTab = GuestInspectorTab::Memories; },
             guestInspectorTab == GuestInspectorTab::Memories);
      button(left + 208, y, 108, 30, L"Reviews",
             [this] { guestInspectorTab = GuestInspectorTab::Reviews; },
             guestInspectorTab == GuestInspectorTab::Reviews);
      y += 38;
      const auto &guest = **selectedIt;
      paragraph(L"Guest #" + std::to_wstring(guest.profile.id) + L" · " +
                    guestArchetypeName(guest.profile.archetype) + L" · " +
                    money(guest.profile.budgetPerNightCents) + L" / night",
                36);
      if (guestInspectorTab == GuestInspectorTab::Overview) {
        label(L"Stay / group",
              guestLifecycleName(guest.lifecycle) + L" · " +
                  L"group #" + std::to_wstring(guest.groupId) + L" · " +
                  std::to_wstring(guest.memberIds.size()) + L" guests · lead #" +
                  std::to_wstring(guest.leaderGuestId) + L" · " +
                  (guest.experience.isGroupLeader ? L"leader" : L"member"));
        if (guest.goalSelection) {
          label(L"Current goal", guestGoalName(guest.goalSelection->goal));
          if (guest.goalSelection->mandatory)
            paragraph(L"Priority action · discretionary factors not scored", 28);
          else {
            label(L"Goal utility", decimal(guest.goalSelection->utility));
            paragraph(L"Need " + pct(guest.goalSelection->needPressure * 100) +
                          L" · preference " +
                          pct(guest.goalSelection->preference * 100) +
                          L" · access " +
                          pct(guest.goalSelection->availabilityFactor * 100) +
                          L" · time " +
                          pct(guest.goalSelection->timeCompatibility * 100),
                      34);
            paragraph(L"Budget " +
                          pct(guest.goalSelection->budgetCompatibility * 100) +
                          L" · group " +
                          pct(guest.goalSelection->groupCompatibility * 100) +
                          L" · distance " +
                          pct(guest.goalSelection->distanceUtility * 100) +
                          L" · mood " +
                          pct(guest.goalSelection->moodModifier * 100),
                      34);
          }
        } else {
          label(L"Current goal", guestGoalName(guest.currentGoal));
          label(L"Goal utility", decimal(guest.currentGoalUtility));
        }
        label(L"Queue tolerance",
              decimal(guest.queueToleranceMinutes) + L" min");
        label(L"Energy / hunger",
              pct(guest.needs.values[static_cast<std::size_t>(GuestNeed::Energy)]) +
                  L" / " +
                  pct(guest.needs.values[static_cast<std::size_t>(GuestNeed::Hunger)]));
        label(L"Hygiene / comfort",
              pct(guest.needs.values[static_cast<std::size_t>(GuestNeed::Hygiene)]) +
                  L" / " +
                  pct(guest.needs.values[static_cast<std::size_t>(GuestNeed::Comfort)]));
        label(L"Entertainment / social",
              pct(guest.needs.values[static_cast<std::size_t>(GuestNeed::Entertainment)]) +
                  L" / " +
                  pct(guest.needs.values[static_cast<std::size_t>(GuestNeed::Social)]));
        label(L"Privacy / safety",
              pct(guest.needs.values[static_cast<std::size_t>(GuestNeed::Privacy)]) +
                  L" / " +
                  pct(guest.needs.values[static_cast<std::size_t>(GuestNeed::Safety)]));
        label(L"Service / cleanliness",
              pct(guest.needs.perceptions.serviceConfidence) + L" / " +
                  pct(guest.needs.perceptions.cleanlinessConfidence));
        label(L"Comfort / value",
              pct(guest.needs.perceptions.environmentComfort) + L" / " +
                  pct(guest.needs.perceptions.valuePerception));
        paragraph(L"Expectation / satisfaction", 25);
        constexpr std::array<const wchar_t *, GuestCategoryCount> shortNames = {
            L"Room", L"Clean", L"Service", L"Food", L"Amenity", L"Quiet",
            L"Convenience", L"Value", L"Arrival"};
        for (std::size_t row = 0; row < 3; ++row) {
          std::wstring line;
          for (std::size_t column = 0; column < 3; ++column) {
            const auto index = row * 3 + column;
            if (column)
              line += L" · ";
            line += shortNames[index];
            line += L" ";
            line += decimal(guest.profile.expectations[index]);
            line += L"/";
            line += decimal(guest.experience.categorySatisfaction[index]);
          }
          paragraph(line, 30);
        }
        const auto market = guest.marketReferenceNightlyRateCents
                                ? money(*guest.marketReferenceNightlyRateCents)
                                : L"unavailable";
        const auto noise = guest.measuredRoomNoise
                               ? decimal(*guest.measuredRoomNoise)
                               : L"unavailable";
        paragraph(L"Market rate " + market + L" · room noise " + noise, 30);
      } else if (guestInspectorTab == GuestInspectorTab::Memories) {
        paragraph(L"Active memories · " +
                      std::to_wstring(guest.activeMemories.size()),
                  28);
        for (std::size_t offset = 0;
             offset < guest.activeMemories.size() && offset < 7 && y + 50 < bottom;
             ++offset) {
          const auto &memory = guest.activeMemories[
              guest.activeMemories.size() - 1 - offset];
          const auto statement =
              memory.reviewStatement.empty()
                  ? guestCategoryName(memory.category) +
                        (memory.valence < 0.0 ? L" · negative" : L" · positive")
                  : wide(memory.reviewStatement);
          const auto source = memory.sourceEntityId
                                  ? L"source #" +
                                        std::to_wstring(*memory.sourceEntityId)
                                  : L"system event";
          paragraph(L"Day " +
                        std::to_wstring(memory.timestampSeconds / 86400 + 1) +
                        L" · location #" + std::to_wstring(memory.locationId) +
                        L" · " + source + L" · " + statement,
                    44);
        }
        std::size_t openComplaintCount{};
        const GuestComplaint *openComplaint{};
        for (const auto &complaint : guest.experience.complaints)
          if (complaint.open) {
            ++openComplaintCount;
            if (!openComplaint)
              openComplaint = &complaint;
          }
        paragraph(L"Open complaints · " +
                      std::to_wstring(openComplaintCount),
                  28);
        if (openComplaint && y + 76 < bottom) {
          const std::array<std::pair<std::wstring, GuestRecoveryOption>, 3>
              recoveryOptions = {{{L"Apologize", GuestRecoveryOption::ApologyOnly},
                                  {L"Upgrade", GuestRecoveryOption::RoomUpgrade},
                                  {L"Refund", GuestRecoveryOption::FullNightRefund}}};
          for (std::size_t i = 0; i < recoveryOptions.size(); ++i)
            button(left + static_cast<int>(i) * 104, y, 100, 30,
                   recoveryOptions[i].first,
                   [this, guestId = guest.profile.id,
                    complaintId = openComplaint->id,
                    option = recoveryOptions[i].second] {
                     result(simulation.resolveGuestComplaint(guestId,
                                                             complaintId,
                                                             option));
                   });
          y += 39;
        }
      } else {
        label(L"Rating scale", decimal(guest.reviewRatingMinimum) + L"–" +
                                  decimal(guest.reviewRatingMaximum) + L" / 10");
        label(L"Review score variation",
              L"±" + decimal(guest.reviewScoreNoiseRange));
        const ReviewView *latestReview{};
        for (const auto &review : snapshot.reviews)
          if (review.reservationId == guest.reservationId)
            latestReview = &review;
        if (latestReview) {
          label(L"Guest rating", decimal(latestReview->rating) + L" / 10");
          label(L"Overall satisfaction",
                pct(latestReview->overallSatisfaction));
          paragraph(wide(latestReview->text), 76);
        } else {
          paragraph(L"No completed-stay review is available for this guest yet.",
                    52);
        }
      }
    }
  } else if (page == Page::Supplies) {
    heading(L"Supplies & tasks");
    label(L"Clean linen", std::to_wstring(snapshot.inventory.linen));
    label(L"Towels", std::to_wstring(snapshot.inventory.towels));
    label(L"Amenities", std::to_wstring(snapshot.inventory.amenities));
    label(L"Cleaning chemicals", std::to_wstring(snapshot.inventory.chemicals));
    label(L"Repair parts", std::to_wstring(snapshot.inventory.parts));
    fullButton(L"Order supplies for 20 turnovers", [this] {
      result(simulation.orderSupplies({20, 40, 20, 20, 0}));
    });
    fullButton(L"Order 10 repair parts",
               [this] { result(simulation.orderSupplies({0, 0, 0, 0, 10})); });
    for (const auto &o : snapshot.supplyOrders)
      if (!o.delivered && y + 30 < bottom) {
        paragraph(L"Order #" + std::to_wstring(o.id) + L" · expected day " +
                      std::to_wstring(o.etaDay + 1),
                  27);
      }
    separator();
    std::vector<TaskView> tasks;
    for (const auto &t : snapshot.tasks)
      if (t.status != TaskStatus::Completed)
        tasks.push_back(t);
    for (std::size_t i = static_cast<std::size_t>(tabScroll);
         i < tasks.size() && y + 65 < bottom; ++i) {
      const auto &t = tasks[i];
      paragraph(taskName(t.kind) + L" #" + std::to_wstring(t.id), 23);
      paragraph(t.blockedReason.empty()
                    ? L"Worker #" + std::to_wstring(t.employeeId) + L" · " +
                          std::to_wstring(
                              static_cast<int>(t.workRemainingSeconds / 60)) +
                          L" min work"
                    : wide(t.blockedReason),
                36);
      separator();
    }
  } else if (page == Page::Finance) {
    heading(L"Property performance");
    label(L"Available cash", money(snapshot.economy.cashCents));
    label(L"Room revenue", money(snapshot.economy.revenueCents));
    label(L"Payroll", money(snapshot.economy.payrollCents));
    label(L"Supplies", money(snapshot.economy.supplyCostCents));
    label(L"Construction", money(snapshot.economy.constructionCostCents));
    label(L"Utilities", money(snapshot.economy.utilityCostCents));
    separator();
    label(L"Operating result",
          money(snapshot.economy.revenueCents - snapshot.economy.payrollCents -
                snapshot.economy.supplyCostCents -
                snapshot.economy.utilityCostCents));
    label(L"Property classification",
          std::to_wstring(snapshot.economy.stars) + L" stars");
    if (snapshot.economy.distressed)
      paragraph(L"Cash is below zero. Reduce costs, review room rates and "
                L"restore operating cash flow.",
                62);
    paragraph(L"Figures are campaign totals. Reviews below reflect completed "
              L"guest stays.",
              42);
    for (std::size_t i = static_cast<std::size_t>(tabScroll);
         i < snapshot.reviews.size() && y + 82 < bottom; ++i) {
      const auto &r = snapshot.reviews[snapshot.reviews.size() - 1 - i];
      paragraph(L"Day " + std::to_wstring(r.day + 1) + L" · " +
                    std::to_wstring(r.score) + L" / 100",
                23);
      paragraph(wide(r.text), 48);
      separator();
    }
  } else {
    heading(L"Your first hotel");
    paragraph(L"A good stay starts behind the scenes. Keep rooms ready, shifts "
              L"covered and supplies close to the work.",
              64);
    const std::array<std::wstring, 5> steps = {
        L"1   Inspect a bedroom. Check its rate, cleanliness and connection to "
        L"reception.",
        L"2   Press 1× below. Watch guests arrive and reception process the "
        L"queue.",
        L"3   After checkout, follow a housekeeper. Dirty rooms earn nothing "
        L"until serviced.",
        L"4   Use Supplies to check shortages. Order before stock runs out; "
        L"deliveries take time.",
        L"5   Add rooms only when staffing and cash can support them. Track "
        L"reviews and profit."};
    for (const auto &st : steps)
      if (y + 70 < bottom - 80) {
        paragraph(st, 61);
      }
    if (y + 90 < bottom - 80) {
      separator();
      paragraph(L"WASD pan · Q/E rotate · wheel zoom\nPgUp/PgDn floors · Space "
                L"pause\nF5 save · F9 load · Esc inspect",
                64);
    }
    if (y + 38 < bottom)
      fullButton(L"Start operating · 1× speed", [this] {
        speed = 1;
        page = Page::Rooms;
        notice = L"Hotel is running. Select a room to inspect its service.";
      });
    if (y + 38 < bottom)
      fullButton(L"New starter campaign", [this] { newCampaign(); });
  }
  if (page != Page::Build && page != Page::Guide) {
    button(left, height - FooterHeight - 42, 151, 28, L"Previous",
           [this] { tabScroll = std::max(0, tabScroll - 1); });
    button(left + 163, height - FooterHeight - 42, 151, 28, L"Next", [this] {
      int count = 0;
      switch (page) {
      case Page::Rooms:
        count = static_cast<int>(snapshot.rooms.size());
        break;
      case Page::Staff:
        for (const auto &p : snapshot.people)
          count += p.kind != PersonKind::Guest;
        break;
      case Page::Guests:
        for (const auto &p : snapshot.people)
          count +=
              p.kind == PersonKind::Guest && p.state != PersonState::CheckedOut;
        break;
      case Page::Supplies:
        for (const auto &t : snapshot.tasks)
          count += t.status != TaskStatus::Completed;
        break;
      default:
        count = static_cast<int>(snapshot.reviews.size());
        break;
      }
      tabScroll = std::min(std::max(0, count - 1), tabScroll + 1);
    });
  }
  const int by = height - FooterHeight + 9;
  const std::array<int, 5> speeds = {0, 1, 3, 8, 20};
  for (int i = 0; i < 5; ++i) {
    const int v = speeds[static_cast<std::size_t>(i)];
    button(
        15 + i * 52, by, 47, 31, v == 0 ? L"Pause" : std::to_wstring(v) + L"×",
        [this, v] { speed = v; }, speed == v);
  }
  button(291, by, 35, 31, L"−", [this] { changeFloor(floor - 1); });
  text(dc, small, L"Floor " + std::to_wstring(floor), 333, by + 7, 65, 24);
  button(401, by, 35, 31, L"+", [this] { changeFloor(floor + 1); });
  button(453, by, 102, 31, L"Walls", [this] {
    wallMode = static_cast<hh::renderer::WallRenderMode>(
        (static_cast<int>(wallMode) + 1) % 3);
  });
  button(563, by, 103, 31, L"Overlay", [this] {
    overlay = static_cast<Overlay>((static_cast<int>(overlay) + 1) % 4);
  });
  const std::array<std::wstring, 4> overlays = {
      L"Natural view", L"Green ready / blue occupied / amber dirty",
      L"Cleanliness: red 0 → green 100", L"Condition: red 0 → green 100"};
  text(dc, small, overlays[static_cast<std::size_t>(overlay)], 680, by + 7,
       width - 700, 22, Muted);
  // Status strip lies above the world, never over its interactive controls.
  text(dc, small, notice, 22, HeaderHeight - 19, sx - 40, 19, Accent,
       DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
  // Avoid painting over the D3D child surface.
  BitBlt(output, 0, 0, width, HeaderHeight, dc, 0, 0, SRCCOPY);
  BitBlt(output, sx, HeaderHeight, SidebarWidth,
         height - HeaderHeight - FooterHeight, dc, sx, HeaderHeight, SRCCOPY);
  BitBlt(output, 0, height - FooterHeight, width, FooterHeight, dc, 0,
         height - FooterHeight, SRCCOPY);
  SelectObject(dc, old);
  DeleteObject(bitmap);
  DeleteDC(dc);
}
} // namespace hh::client

