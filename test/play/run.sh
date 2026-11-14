#!/usr/bin/env bash
# EVERYTHING COUNTS - a card counting story for the Game Boy Advance
# Copyright (C) 2026 Godless-1
# SPDX-License-Identifier: AGPL-3.0-or-later
#
# Builds the whole game for the build machine (address and undefined-
# behaviour sanitizers on) and runs one script through it.
#
#   test/play/run.sh build
#   test/play/run.sh <script> <outdir> [extra play options]
#
# Screenshots land in <outdir> as PNG; <outdir>/events.log lists every
# clipped, overlapping or painted-over piece of text the run saw.
set -e
cd "$(dirname "$0")/../.."
OUT=build-host
BIN=$OUT/play

build() {
    make -s gen
    rm -rf "$OUT/stage"; mkdir -p "$OUT/stage"
    cp src/*.c src/*.h src/gen/*.c src/gen/*.h "$OUT/stage/"
    python3 test/play/mkshim.py src/gba.h "$OUT/stage/gba.h"
    cp test/play/host.c "$OUT/stage/"
    # the game supplies its own freestanding memcpy & co; keep them out of
    # libc's way, and rename its main so the harness can own the real one
    gcc -O1 -g -std=gnu11 -fsanitize=address,undefined -fno-sanitize-recover=undefined \
        -fno-omit-frame-pointer -fno-builtin -fno-strict-aliasing \
        -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-missing-field-initializers \
        -DEC_HOST -Dmain=game_main -Dmemcpy=ec_host_memcpy -Dmemset=ec_host_memset \
        -Dmemmove=ec_host_memmove -I"$OUT/stage" \
        $(ls "$OUT"/stage/*.c | grep -v '/host\.c$') -c
    mkdir -p "$OUT/obj"; mv ./*.o "$OUT/obj/"
    gcc -O1 -g -std=gnu11 -fsanitize=address,undefined -fno-omit-frame-pointer \
        -Wall -Wextra -DEC_HOST -I"$OUT/stage" -c "$OUT/stage/host.c" -o "$OUT/obj/host.o"
    gcc -fsanitize=address,undefined "$OUT"/obj/*.o -o "$BIN"
}

if [ "$1" = build ]; then build; exit 0; fi
[ -x "$BIN" ] || build

SCRIPT=$1; DIR=$2; shift 2
mkdir -p "$DIR"
ASAN_OPTIONS=detect_leaks=0:abort_on_error=0 UBSAN_OPTIONS=print_stacktrace=1 \
    "$BIN" -o "$DIR" "$@" "$SCRIPT"
python3 test/play/topng.py "$DIR"
