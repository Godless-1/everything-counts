#!/usr/bin/env python3
# EVERYTHING COUNTS - a card counting story for the Game Boy Advance
# Copyright (C) 2026 Godless-1
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Renders a PSG register log (play -a file) to a WAV and lists the notes.

  psg.py <psg.log> <out.wav> [first_frame [frames]]

The log holds registers 0x60-0x9F once per frame.  This is a plain model
of the four legacy channels (square, square, wave, noise): good enough to
hear a tune and to check pitches, not a cycle-exact emulator.
"""
import math, struct, sys, wave
import numpy as np

RATE = 32768
SPF = RATE * 280896 // 16777216          # samples per GBA frame (59.73 Hz)
NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
DUTY = [0.125, 0.25, 0.5, 0.75]

def note_name(f):
    if f <= 0: return '--'
    m = int(round(69 + 12 * math.log2(f / 440.0)))
    return '%s%d' % (NAMES[m % 12], m // 12 - 1)

def u16(b, off): return b[off - 0x60] | (b[off - 0x60 + 1] << 8)

class Env:
    def __init__(s): s.vol = 0; s.step = 0; s.up = 0; s.acc = 0; s.on = False
    def trigger(s, cnt):
        s.vol = cnt >> 12; s.up = (cnt >> 11) & 1; s.step = (cnt >> 8) & 7
        s.acc = 0; s.on = not (s.vol == 0 and not s.up)
    def frame(s):
        if not s.on or not s.step: return
        s.acc += 64.0 / 59.73
        while s.acc >= s.step:
            s.acc -= s.step
            s.vol = min(15, s.vol + 1) if s.up else max(0, s.vol - 1)

def render(log, out, first=0, count=None):
    data = open(log, 'rb').read()
    frames = [data[i:i + 64] for i in range(0, len(data) - 63, 64)]
    frames = frames[first:first + count if count else None]
    pcm = np.zeros(len(frames) * SPF, dtype=np.float32)
    env = [Env(), Env(), None, Env()]
    phase = [0.0, 0.0, 0.0]
    lfsr, nacc = 0x7FFF, 0.0
    notes = {0: [], 1: []}
    t = np.arange(SPF, dtype=np.float32)
    for fi, b in enumerate(frames):
        seg = np.zeros(SPF, dtype=np.float32)
        # ---- squares
        for ch, (cnt_r, frq_r) in enumerate(((0x62, 0x64), (0x68, 0x6C))):
            cnt, frq = u16(b, cnt_r), u16(b, frq_r)
            f = 131072.0 / (2048 - (frq & 0x7FF))
            if frq & 0x8000:
                env[ch].trigger(cnt)
                notes[ch].append((fi, note_name(f), cnt >> 12))
            if cnt == 0: env[ch].on = False
            if env[ch].on and env[ch].vol:
                ph = (phase[ch] + t * f / RATE) % 1.0
                seg += np.where(ph < DUTY[(cnt >> 6) & 3], 1.0, -1.0) * (env[ch].vol / 15.0)
            phase[ch] = (phase[ch] + SPF * f / RATE) % 1.0
            env[ch].frame()
        # ---- wave
        sel, wcnt, wfrq = u16(b, 0x70), u16(b, 0x72), u16(b, 0x74)
        vcode = (wcnt >> 13) & 3
        if (sel & 0x80) and vcode:
            ram = b[0x30:0x40]
            smp = np.array([n for byte in ram for n in (byte >> 4, byte & 15)], dtype=np.float32)
            f = 65536.0 / (2048 - (wfrq & 0x7FF))
            idx = ((phase[2] + t * f / RATE) % 1.0 * 32).astype(np.int32)
            seg += (smp[idx] / 7.5 - 1.0) * [0, 1.0, 0.5, 0.25][vcode]
            phase[2] = (phase[2] + SPF * f / RATE) % 1.0
        # ---- noise
        ncnt, nfrq = u16(b, 0x78), u16(b, 0x7C)
        if nfrq & 0x8000:
            env[3].trigger(ncnt); lfsr = 0x7FFF
        if ncnt == 0: env[3].on = False
        if env[3].on and env[3].vol:
            r, s7, sh = nfrq & 7, (nfrq >> 3) & 1, (nfrq >> 4) & 15
            f = 524288.0 / (r if r else 0.5) / (2 ** (sh + 1))
            o = np.empty(SPF, dtype=np.float32)
            for i in range(SPF):
                nacc += f / RATE
                while nacc >= 1.0:
                    nacc -= 1.0
                    x = (lfsr ^ (lfsr >> 1)) & 1
                    lfsr = (lfsr >> 1) | (x << 14)
                    if s7: lfsr = (lfsr & ~0x40) | (x << 6)
                o[i] = 1.0 if (lfsr & 1) == 0 else -1.0
            seg += o * (env[3].vol / 15.0)
        env[3].frame()
        pcm[fi * SPF:(fi + 1) * SPF] = seg
    pcm = np.clip(pcm / 4.0, -1, 1)
    with wave.open(out, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE)
        w.writeframes((pcm * 30000).astype('<i2').tobytes())
    return notes, len(frames)

if __name__ == '__main__':
    first = int(sys.argv[3]) if len(sys.argv) > 3 else 0
    count = int(sys.argv[4]) if len(sys.argv) > 4 else None
    notes, n = render(sys.argv[1], sys.argv[2], first, count)
    print('%d frames -> %s (%.1f s)' % (n, sys.argv[2], n / 59.73))
    for ch, name in ((0, 'lead'), (1, 'bass')):
        print(name + ': ' + ' '.join('%s' % nm for _, nm, _ in notes[ch][:96]))
