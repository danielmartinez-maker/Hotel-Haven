from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label} anchor mismatch: {count}")
    return text.replace(old, new, 1)


workforce = Path("game/tests/WorkforceTests.cpp")
text = workforce.read_text(encoding="utf-8")
text = replace_once(
    text,
    'R"({"baseDemand":100,"initialLinen":1000,"initialTowels":2000,"initialAmenities":1000,"initialChemicals":1000,"initialParts":500,"staffBreakAfterMinutes":240,"staffBreakDurationMinutes":15})"',
    'R"({"baseDemand":100,"initialLinen":400,"initialTowels":400,"initialAmenities":250,"initialChemicals":250,"initialParts":100,"staffBreakAfterMinutes":240,"staffBreakDurationMinutes":15})"',
    "workforce-soak-physical-capacity",
)
workforce.write_text(text, encoding="utf-8")

simulation = Path("game/tests/SimulationTests.cpp")
text = simulation.read_text(encoding="utf-8")
text = replace_once(
    text,
    'R"({"baseDemand":100,"initialLinen":200,"initialTowels":400,"initialAmenities":200,"initialChemicals":200})"',
    'R"({"baseDemand":100,"roomConditionLossPerDay":0,"initialLinen":200,"initialTowels":400,"initialAmenities":200,"initialChemicals":200})"',
    "layout-isolates-engineering-failures",
)
text = replace_once(
    text,
    'require(s.loadDefinitions(R"({"baseDemand":0.85})").ok,\n          "layout benchmark steady demand rejected");',
    'require(s.loadDefinitions(R"({"baseDemand":100})").ok,\n          "layout benchmark saturated demand rejected");',
    "layout-saturated-throughput-demand",
)
simulation.write_text(text, encoding="utf-8")
