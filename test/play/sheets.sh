#!/usr/bin/env bash
# EVERYTHING COUNTS - a card counting story for the Game Boy Advance
# Copyright (C) 2026 Godless-1
# SPDX-License-Identifier: AGPL-3.0-or-later
# sheets.sh <dir> [per-sheet]: contact sheets of every PNG in <dir>
cd "$1"; N=${2:-12}
rm -f sheet-*.png
ls *.png | grep -v '^sheet-' | split -l "$N" -d - .sheetlist-
i=0
for l in .sheetlist-*; do
    python3 "$OLDPWD/test/play/topng.py" --sheet "sheet-$(printf %02d $i).png" --cols=4 $(cat "$l")
    i=$((i+1)); rm "$l"
done
ls sheet-*.png
