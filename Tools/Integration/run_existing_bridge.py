from pathlib import Path
import runpy

# Earlier TDD commits already composed `optimization/` at the root and linked
# hh_game to hh_optimization. Restore only those two exact preconditions in the
# runner workspace so the reviewed bridge script can apply atomically; the
# bridge immediately re-adds them alongside the scheduler seam.
root = Path("CMakeLists.txt")
root_text = root.read_text(encoding="utf-8")
current_root = (
    "add_subdirectory(Tools/ContentPipeline)\n"
    "add_subdirectory(optimization)\n"
    "add_subdirectory(game)"
)
legacy_root = "add_subdirectory(Tools/ContentPipeline)\nadd_subdirectory(game)"
if root_text.count(current_root) != 1:
    raise RuntimeError("unexpected root CMake optimizer composition state")
root.write_text(root_text.replace(current_root, legacy_root, 1), encoding="utf-8")

game = Path("game/CMakeLists.txt")
game_text = game.read_text(encoding="utf-8")
current_link = "target_link_libraries(hh_game PRIVATE hh_assets hh_optimization)"
legacy_link = "target_link_libraries(hh_game PRIVATE hh_assets)"
if game_text.count(current_link) != 1:
    raise RuntimeError("unexpected game optimizer link state")
game.write_text(game_text.replace(current_link, legacy_link, 1), encoding="utf-8")

runpy.run_path(
    "Tools/Integration/apply_full_game_nvidia_bridge.py", run_name="__main__"
)
