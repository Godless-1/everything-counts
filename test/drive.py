#!/usr/bin/env python3
# EVERYTHING COUNTS - a card counting story for the Game Boy Advance
# Copyright (C) 2026 Godless-1
# SPDX-License-Identifier: AGPL-3.0-or-later
"""
Headless test harness for EVERYTHING COUNTS.

Boots the ROM in mGBA on a private Xvfb display, drives it with xdotool and
captures native-resolution 240x160 screenshots by grabbing the emulator's
display widget and downscaling it.

usage: drive.py <rom> <outdir> [token ...]
  wait:<sec>            let the game run
  key:<name>            tap a button
  hold:<name>:<sec>     hold a button
  mash:<name>:<count>   tap repeatedly
  shot:<label>          capture a screenshot
"""
import os, re, subprocess, sys, time, shutil

MGBA = os.environ.get("EC_MGBA") or shutil.which("mgba-qt") or shutil.which("mgba") or \
       os.path.expanduser("~/.local/share/gba-toolchain/squashfs-root/usr/bin/mgba-qt")
DISPLAY = os.environ.get("EC_DISPLAY", ":77")
SCREEN = (960, 720)

KEYMAP = {
    'a': 'x', 'b': 'z', 'l': 'a', 'r': 's',
    'start': 'Return', 'select': 'BackSpace',
    'up': 'Up', 'down': 'Down', 'left': 'Left', 'right': 'Right',
}

def sh(*args, **kw):
    return subprocess.run(args, capture_output=True, text=True, **kw)


class Session:
    def __init__(self, rom, outdir, scale=3):
        self.rom = os.path.abspath(rom)
        self.outdir = os.path.abspath(outdir)
        self.scale = scale
        os.makedirs(self.outdir, exist_ok=True)
        env = dict(os.environ)
        for k in ('WAYLAND_DISPLAY', 'QT_QPA_PLATFORMTHEME'):
            env.pop(k, None)
        env.update(DISPLAY=DISPLAY, QT_QPA_PLATFORM='xcb', XDG_SESSION_TYPE='x11',
                   XDG_CONFIG_HOME=os.path.join(self.outdir, 'mgbacfg'),
                   QT_LOGGING_RULES='*=false')
        os.makedirs(env['XDG_CONFIG_HOME'], exist_ok=True)
        self.env = env
        self.n = 0

    # ---- lifecycle ----------------------------------------------------
    def start(self):
        self.xvfb = subprocess.Popen(
            ['Xvfb', DISPLAY, '-screen', '0', '%dx%dx24' % SCREEN, '-nolisten', 'tcp'],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, start_new_session=True)
        sock = '/tmp/.X11-unix/X' + DISPLAY[1:]
        for _ in range(60):
            time.sleep(0.25)
            if os.path.exists(sock):
                break
        time.sleep(0.6)

        self.log = open(os.path.join(self.outdir, 'mgba.log'), 'w')
        self.mgba = subprocess.Popen(
            [MGBA, '-%d' % self.scale, '-C', 'mute=1', '-C', 'videoSync=1',
             '-C', 'audioSync=0', '-C', 'fpsTarget=60', '-C', 'volume=0',
             '-C', 'lockAspectRatio=1', self.rom],
            env=self.env, stdout=self.log, stderr=self.log, start_new_session=True)

        self.win = self.area = None
        for _ in range(80):
            time.sleep(0.4)
            if self._locate():
                break
        if not self.win:
            raise RuntimeError("mGBA window never appeared; see %s/mgba.log" % self.outdir)
        sh('xdotool', 'windowfocus', self.win, env=self.env)
        time.sleep(1.0)

    def _locate(self):
        tree = sh('xwininfo', '-root', '-tree', env=self.env).stdout
        gw, gh = 240 * self.scale, 160 * self.scale
        best = None
        for line in tree.split('\n'):
            m = re.search(r'(0x[0-9a-f]+) .*?  (\d+)x(\d+)\+-?\d+\+-?\d+  \+(-?\d+)\+(-?\d+)', line)
            if not m:
                continue
            w, h, ax, ay = int(m.group(2)), int(m.group(3)), int(m.group(4)), int(m.group(5))
            if w == gw and h == gh and ax >= 0 and ay >= 0:
                best = (m.group(1), (ax, ay, w, h))
        if best:
            top = None
            for line in tree.split('\n'):
                m = re.search(r'(0x[0-9a-f]+) "mGBA - ', line)
                if m:
                    top = m.group(1)
            self.win = top or best[0]
            self.area = best[1]
            return True
        return False

    def title(self):
        return sh('xdotool', 'getwindowname', self.win, env=self.env).stdout.strip()

    def fps(self):
        m = re.search(r'\(([\d.]+) fps\)', self.title())
        return float(m.group(1)) if m else -1.0

    # ---- input ---------------------------------------------------------
    def key(self, name, dur=0.10):
        k = KEYMAP[name]
        sh('xdotool', 'windowfocus', self.win, env=self.env)
        sh('xdotool', 'keydown', k, env=self.env)
        time.sleep(dur)
        sh('xdotool', 'keyup', k, env=self.env)
        time.sleep(0.16)

    # ---- capture -------------------------------------------------------
    def shot(self, label):
        from PIL import Image
        raw = os.path.join(self.outdir, '.raw.png')
        sh('import', '-window', 'root', raw, env=self.env)
        ax, ay, w, h = self.area
        im = Image.open(raw).convert('RGB').crop((ax, ay, ax + w, ay + h))
        im = im.resize((240, 160), Image.NEAREST)
        self.n += 1
        dst = os.path.join(self.outdir, "%02d-%s.png" % (self.n, label))
        im.save(dst)
        os.remove(raw)
        print("  shot -> %s" % os.path.relpath(dst))
        return dst

    def stop(self):
        for p in (getattr(self, 'mgba', None), getattr(self, 'xvfb', None)):
            if p:
                try:
                    p.terminate(); p.wait(timeout=5)
                except Exception:
                    try: p.kill()
                    except Exception: pass
        try: self.log.close()
        except Exception: pass


def run_script(s, script):
    for tok in script:
        parts = tok.split(':')
        c = parts[0]
        if   c == 'wait':  time.sleep(float(parts[1]))
        elif c == 'key':   s.key(parts[1])
        elif c == 'hold':  s.key(parts[1], float(parts[2]))
        elif c == 'mash':
            for _ in range(int(parts[2])): s.key(parts[1])
        elif c == 'shot':  s.shot(parts[1])
        elif c == 'fps':   print("  fps =", s.fps())
        else: print("unknown token", tok)


def main():
    rom, outdir = sys.argv[1], sys.argv[2]
    script = sys.argv[3:] or ['wait:3', 'shot:boot']
    s = Session(rom, outdir)
    s.start()
    try:
        run_script(s, script)
    finally:
        s.stop()

if __name__ == '__main__':
    main()
