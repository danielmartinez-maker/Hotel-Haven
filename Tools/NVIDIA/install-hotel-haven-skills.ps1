$ErrorActionPreference = "Stop"

$skills = @(
    "cuopt-install",
    "cuopt-numerical-optimization-formulation",
    "cuopt-numerical-optimization-api",
    "cuopt-multi-objective-exploration",
    "omniverse-usd-performance-tuning"
)

foreach ($skill in $skills) {
    Write-Host "Installing NVIDIA skill: $skill"
    npx skills add nvidia/skills --skill $skill --agent codex --global --yes
}

Write-Host "Hotel Haven NVIDIA skill set installed. Restart/reload Codex if newly installed skills are not visible."
