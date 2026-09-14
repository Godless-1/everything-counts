#!/usr/bin/env bash
# EVERYTHING COUNTS - a card counting story for the Game Boy Advance
# Copyright (C) 2026 Godless-1
# SPDX-License-Identifier: AGPL-3.0-or-later
# Smoke run: boot the ROM and capture a screenshot.
set -e
cd "$(dirname "$0")/.."
python3 test/drive.py dist/everything-counts.gba test/shots "$@"
