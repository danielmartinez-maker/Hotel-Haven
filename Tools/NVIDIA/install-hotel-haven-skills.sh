#!/usr/bin/env bash
set -euo pipefail

skills=(
  cuopt-install
  cuopt-numerical-optimization-formulation
  cuopt-numerical-optimization-api
  cuopt-multi-objective-exploration
  omniverse-usd-performance-tuning
)

for skill in "${skills[@]}"; do
  echo "Installing NVIDIA skill: ${skill}"
  npx skills add nvidia/skills --skill "${skill}" --agent codex --global --yes
done

echo "Hotel Haven NVIDIA skill set installed. Restart/reload Codex if newly installed skills are not visible."
