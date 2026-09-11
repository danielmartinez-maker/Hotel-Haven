# Hotel Haven Balance Lab Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a deterministic Pareto-analysis CLI for Hotel Haven balance experiments.

**Architecture:** Headless simulation runs remain the source of truth. The Balance Lab consumes run summaries, computes objective vectors, extracts non-dominated points, builds payoff tables, identifies a normalized knee candidate, and emits epsilon-sweep manifests.

**Tech Stack:** Python 3.11 standard library.

**Spec:** `docs/superpowers/specs/2026-09-09-nvidia-optimization-openusd-design.md`

## Global Constraints

- Five objectives: operating cost, guest dissatisfaction, fatigue load, excess wait, negative GOP.
- Input/output ordering must be deterministic.
- No third-party Python dependency is required for baseline use.

---

### Task 1: Pareto core

**Files:**
- Create: `Tools/BalanceLab/balance_lab.py`
- Create: `Tools/BalanceLab/test_balance_lab.py`

- [ ] Write failing tests for dominance, frontier extraction, anchor/payoff table, normalization, and knee selection.
- [ ] Run tests and confirm RED.
- [ ] Implement minimal pure functions and rerun GREEN.

### Task 2: CLI and sweep manifests

**Files:**
- Modify: `Tools/BalanceLab/balance_lab.py`
- Modify: `Tools/BalanceLab/test_balance_lab.py`
- Create: `Tools/BalanceLab/README.md`

- [ ] Write failing tests for deterministic JSON/CSV output and epsilon-grid generation.
- [ ] Run RED.
- [ ] Implement `frontier` and `epsilon-grid` CLI commands.
- [ ] Run GREEN.

### Task 3: CI

**Files:**
- Create: `.github/workflows/balance-lab-ci.yml`

- [ ] Run `python -m unittest` on Windows and Ubuntu.
