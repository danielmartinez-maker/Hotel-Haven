from pathlib import Path

path = Path("game/tests/SimulationTests.cpp")
text = path.read_text(encoding="utf-8")

old = '''  bool restored = false;
  for (const auto &roomView : serviced.rooms)
    restored |=
        roomView.condition == 100 && roomView.status != RoomStatus::OutOfOrder;
  require(restored, "maintenance staff did not restore a failed room");
'''
new = '''  bool restored = false;
  for (const auto &roomView : serviced.rooms)
    // FINAL-04 Engineering owns condition; corrective work restores at least
    // the authoritative 80% service floor rather than resetting to 100%.
    restored |= roomView.condition >= 80.0 &&
                roomView.status != RoomStatus::OutOfOrder;
  require(restored, "maintenance staff did not restore a failed room");
'''

if text.count(old) != 1:
    raise SystemExit(f"engineering authority test anchor count={text.count(old)}")
text = text.replace(old, new, 1)
path.write_text(text, encoding="utf-8")
