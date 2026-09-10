from pathlib import Path

path = Path("game/src/Simulation.cpp")
text = path.read_text(encoding="utf-8")

old = """    impl_->services.tickSecond();
    impl_->food.tickSecond();
"""
new = """    impl_->services.tickSecond();
    const auto engineering = impl_->services.engineering().snapshot();
    for (const auto &asset : engineering.assets)
      if (auto *room = impl_->getRoom(asset.id))
        room->condition = asset.condition / 100.0;
    impl_->food.tickSecond();
"""

if text.count(old) != 1:
    raise SystemExit(f"engineering authority anchor count={text.count(old)}")
text = text.replace(old, new, 1)

path.write_text(text, encoding="utf-8")
