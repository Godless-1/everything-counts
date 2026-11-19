#!/usr/bin/env python3
# EVERYTHING COUNTS - a card counting story for the Game Boy Advance
# Copyright (C) 2026 Godless-1
# SPDX-License-Identifier: AGPL-3.0-or-later
"""End-to-end checks: plays scripted sessions through the host build of the
whole game and asserts on the bankroll, the save file and the text watcher.

  python3 test/play/check.py        (run.sh build is done first if needed)
"""
import os, re, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RUN = os.path.join(ROOT, 'test', 'play', 'run.sh')
SCR = os.path.join(ROOT, 'test', 'play', 'scripts')
fails = checks = 0

def check(cond, what):
    global fails, checks
    checks += 1
    if not cond:
        fails += 1
        print('  FAIL  ' + what)

def play(script, out, *opts):
    r = subprocess.run([RUN, os.path.join(SCR, script), out, *opts],
                       capture_output=True, text=True, cwd=ROOT)
    if r.returncode != 0 or 'AddressSanitizer' in r.stderr or 'runtime error' in r.stderr:
        print(r.stdout[-2000:], r.stderr[-4000:])
        check(False, script + ' ran clean (no crash, no sanitizer report)')
    dumps = {}
    for line in r.stdout.splitlines():
        if line.startswith('DUMP '):
            f = line.split()
            dumps[f[1]] = dict(kv.split('=', 1) for kv in f[2:] if '=' in kv)
    ev = open(os.path.join(out, 'events.log')).read().strip().splitlines()
    return dumps, ev

def num(d, label, key):
    return int(d[label][key]) if label in d else None

with tempfile.TemporaryDirectory() as tmp:
    print('-- the money: five stacked hands at the Golden Lotus')
    d, ev = play('table-money.txt', os.path.join(tmp, 'money'))
    start = num(d, 'start', 'creds')
    want = [('h1', 7500, 'split 8s, doubled 11 wins, 18 wins: +3 bets'),
            ('h2', 7500, 'insured 19 against a dealer blackjack: even'),
            ('h3', 11250, 'blackjack pays 3 to 2'),
            ('h4', 10000, 'surrender costs half a bet'),
            ('h5', 7500, 'bust costs one bet')]
    for label, delta, what in want:
        check(num(d, label, 'creds') == start + delta, what)
        check(num(d, label, 'tnet') == delta, what + ' (table net)')
    check(num(d, 'left', 'creds') == start + 7500, 'cashing out returns nothing extra')
    check(ev == [], 'no clipped, overlapping or hidden text at the table: %r' % ev[:3])

    print('-- the save: drills from the title must not erase a campaign')
    sav = os.path.join(tmp, 'ec.sav')
    play('save-a.txt', os.path.join(tmp, 'sa'), '-s', sav)
    d, ev = play('save-b.txt', os.path.join(tmp, 'sb'), '-s', sav)
    check(num(d, 'boot', 'night') == 3 and num(d, 'boot', 'creds') == 900000,
          'the saved run is live from the first frame')
    best = d.get('drilled', {}).get('drills', '0,').split(',')[0]
    check(int(best) > 0, 'the tag drill recorded a best score')
    check(ev == [], 'no text problems in the tag drill: %r' % ev[:3])
    d, ev = play('save-c.txt', os.path.join(tmp, 'sc'), '-s', sav)
    check(num(d, 'reboot', 'night') == 3 and num(d, 'reboot', 'creds') == 900000,
          'after the drill, the campaign is still in the save')
    check(d.get('reboot', {}).get('drills', '0,').split(',')[0] == best,
          'and the drill best was saved with it')
    check(num(d, 'hub', 'night') == 3 and num(d, 'hub', 'escrow') == 220000,
          'CONTINUE resumes that campaign')

    print('-- the locks: a fresh cartridge cannot open MOD 2, MOD 3, the chart or a drill')
    # the script itself fails (exit 3) if a locked A opens anything: it waits for the
    # refusal text 'FINISH MOD 1 FIRST' / 'NEEDS MOD 3' on screen, then the menu title
    d, ev = play('audit-locks.txt', os.path.join(tmp, 'locks'))
    check(all(k in d for k in ('mod2refused', 'teachrefused', 'rangerefused', 'opened')),
          'the lock script ran to its end (every refusal text was shown)')
    check(num(d, 'rangerefused', 'lessons') == 0, 'refused entries finish no lesson')
    check(num(d, 'opened', 'lessons') == 1 and num(d, 'opened', 'night') == 1,
          'with MOD 1 ticked, MOD 2 opens (its first page was reached)')

    print('-- every screen: the tours must show no broken text')
    for t in sorted(f for f in os.listdir(SCR) if f.startswith('tour-')):
        d, ev = play(t, os.path.join(tmp, t))
        check(ev == [], '%s: %d text problem(s): %r' % (t, len(ev), ev[:3]))

print('\n%d checks, %d failures' % (checks, fails))
sys.exit(1 if fails else 0)
