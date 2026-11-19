#!/usr/bin/env python3
# EVERYTHING COUNTS - a card counting story for the Game Boy Advance
# Copyright (C) 2026 Godless-1
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Helpers for the screen audit that `make test` runs (see check.py).

  findings(outdir)   text problems of one audit run: the harness events.log plus
                     pixel.log (written by pixelcheck.py), minus what allowlist.tsv
                     and the card-rank echo rule explain
  term_order(outdir, walk)
                     the first screen of a lesson walk that uses a term must carry
                     its plain-words gloss (term_order.tsv)

  python3 test/play/audit.py findings <outdir>
  python3 test/play/audit.py order <outdir> <walk name>     (exit 1 when a check fails)
"""
import os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))


def _rows(name):
    return [l.rstrip('\n').split('\t') for l in open(os.path.join(HERE, name))
            if l.strip() and not l.startswith('#')]


def load_allow():
    """kind, script or *, regex on the rest of the line, class id, reason (20+ characters)"""
    rows = _rows('allowlist.tsv')
    for r in rows:
        if len(r) != 5 or not re.match(r'AL-\d+$', r[3]) or len(r[4]) < 20:
            raise SystemExit('bad allowlist row: %r' % r)
    return rows


def _glyph_rows(path):
    out = []
    if os.path.exists(path):
        for l in open(path):
            c = l.rstrip('\n').split('\t')
            if len(c) >= 8:
                out.append((int(c[0]), int(c[1]), c[7]))
    return out


def _echo_twin(outdir, shot, ch, x, y, cache={}):
    """src/cards.c draws every card rank twice (a white copy 1 px down and right under the
    coloured one), so the checker measures the rank against its own echo: class AL-01."""
    key = (outdir, shot)
    if key not in cache:
        cache[key] = _glyph_rows(os.path.join(outdir, shot + '.glyphs.tsv'))
    return any(s == ch and (gx, gy) != (x, y) and abs(gx - x) <= 1 and abs(gy - y) <= 1
               for gx, gy, s in cache[key])


def run_pixelcheck(outdir):
    subprocess.run([sys.executable, os.path.join(HERE, 'pixelcheck.py'), outdir],
                   capture_output=True, text=True)


def findings(outdir, script):
    allow = load_allow()
    bad = []
    for log in ('events.log', 'pixel.log'):
        p = os.path.join(outdir, log)
        if not os.path.exists(p):
            continue
        n = 0
        for line in open(p):
            line = line.strip()
            if not line:
                continue
            if log == 'events.log' and re.match(r'frame \d+', line):
                n += 1
            m = re.match(r'(?:frame \d+\s+)?(\w+)\s+(.*)', line)
            if not m:
                bad.append(line)
                continue
            kind, rest = m.groups()
            if kind in ('LOWCONTRAST', 'BOXTIGHT'):
                mm = re.search(r'(?:(\S+) )?"(.*)" at (\d+),(\d+)', rest)
                if mm:
                    shot = mm.group(1) if log == 'pixel.log' else 'ev%03d-%s' % (n - 1, kind)
                    if shot and _echo_twin(outdir, shot, mm.group(2), int(mm.group(3)), int(mm.group(4))):
                        continue
            if any(a[0] == kind and a[1] in ('*', script) and re.search(a[2], rest) for a in allow):
                continue
            bad.append(line)
    return bad


def _norm(t):
    return re.sub(r'\s+', ' ', t.replace('\n', ' ')).strip()


def screens(outdir):
    """the text of every shot, in the order the harness drew them"""
    fs = [f for f in os.listdir(outdir) if f.endswith('.text.txt') and 'BOXTIGHT' not in f and '-menu' not in f
          and not f.startswith('waitfor')]
    fs.sort(key=lambda f: os.stat(os.path.join(outdir, f)).st_mtime_ns)
    return [(f, _norm(open(os.path.join(outdir, f)).read())) for f in fs]


def term_order(outdir, walk):
    """problems: a term whose first screen (or the one after it) has no gloss, or never shows"""
    scr = screens(outdir)
    bad = []
    for w, term, gloss in _rows('term_order.tsv'):
        if w != walk:
            continue
        t, g = re.compile(term, re.I), re.compile(gloss, re.I)
        first = next((i for i, (_, x) in enumerate(scr) if t.search(x)), None)
        if first is None:
            bad.append('%s: term /%s/ never drawn (the walk lost a page?)' % (walk, term))
            continue
        near = scr[first][1] + ' ' + (scr[first + 1][1] if first + 1 < len(scr) else '')
        if not g.search(near):
            bad.append('%s: /%s/ first used on %s without its gloss /%s/' % (walk, term, scr[first][0], gloss))
    return bad


if __name__ == '__main__':
    if sys.argv[1] == 'findings':
        r = findings(sys.argv[2], os.path.basename(sys.argv[2].rstrip('/')))
    else:
        r = term_order(sys.argv[2], sys.argv[3])
    print('\n'.join(r) or 'ok')
    sys.exit(1 if r else 0)
