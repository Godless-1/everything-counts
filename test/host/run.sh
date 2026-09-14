#!/usr/bin/env bash
# EVERYTHING COUNTS - a card counting story for the Game Boy Advance
# Copyright (C) 2026 Godless-1
# SPDX-License-Identifier: AGPL-3.0-or-later
# Builds the rules engine against host shims (so the real gba.h and its
# inline ARM asm stay out of the way) and runs the verification suite.
set -e
cd "$(dirname "$0")/../.."
STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT
cp src/blackjack.c src/cards.c src/game.c src/rng.c \
   src/blackjack.h src/cards.h src/game.h src/rng.h "$STAGE/"
cp test/host/shim/*.h "$STAGE/"
cp test/host/sim.c "$STAGE/"
gcc -O2 -Wall -Wno-unused-function -I"$STAGE" -o "$STAGE/sim" \
    "$STAGE/sim.c" "$STAGE/blackjack.c" "$STAGE/cards.c" "$STAGE/game.c" "$STAGE/rng.c"
"$STAGE/sim"
