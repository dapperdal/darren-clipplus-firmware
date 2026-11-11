#!/usr/bin/env python3
"""Capture the current firmware tour and package the simulator's dolphin frames.
Usage: capture_focus_tour.py SIM_BUILD OUTPUT_DIR
Run test_focus_dolphins.py SIM_BUILD --capture first.
"""
import os
from pathlib import Path
import subprocess
import sys
from PIL import Image

build = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2]).resolve()
out.mkdir(parents=True, exist_ok=True)
disk = build / "simdisk"
config = disk / ".rockbox/config.cfg"
script = disk / "focus_script.txt"
saved = {p: p.read_bytes() for p in [config, script] if p.exists()}
commands = "w 5\ns tour_00\n"
for i in range(1, 6):
    commands += f"k DOWN\nw 4\ns tour_{i:02}\n"
commands += """h UP 4
w 12
k SELECT
w 4
s tour_06
k SELECT
w 15
s tour_07
k DOWN
w 4
s tour_08
k HOME
w 4
k VOLDOWN
w 2
s tour_09
w 20
k HOME
w 4
k HOME
w 4
k DOWN
w 4
k DOWN
w 4
k SELECT
w 8
s tour_10
k SELECT
w 4
s tour_11
k DOWN
w 4
k SELECT
w 4
s tour_12
k HOME
w 4
k HOME
w 4
h DOWN 2
w 10
k SELECT
w 4
s tour_13
k SELECT
w 4
s tour_14
k HOME
w 4
h DOWN 5
w 12
k SELECT
w 4
s tour_15
k HOME
w 4
k UP
w 4
k SELECT
w 4
s tour_16
k HOME
w 4
k HOME
w 4
h UP 1
w 6
k SELECT
w 4
k SELECT
w 4
s tour_17
q
"""
try:
    config.write_text("backlight timeout: 0\nshuffle: off\nrepeat: all\n")
    script.write_text(commands)
    result = subprocess.run([str(build / "rockboxui")], cwd=build,
                            env=dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy"),
                            capture_output=True, text=True, timeout=90)
    (build / "capture-tour.log").write_text(result.stdout + result.stderr)
    assert result.returncode == 0, result.stdout + result.stderr
finally:
    for p in [config, script]:
        if p in saved:
            p.write_bytes(saved[p])
        else:
            p.unlink(missing_ok=True)


def colour(name):
    raw = Image.open(disk / "shots" / f"{name}.pgm").convert("L")
    image = Image.new("RGB", (128, 66))
    for y in range(64):
        for x in range(128):
            if raw.getpixel((x, y)):
                image.putpixel((x, y if y < 16 else y + 2),
                               (255, 202, 74) if y < 16 else (65, 190, 255))
    return image.resize((512, 264), Image.Resampling.NEAREST)


def sheet(frames, name):
    strip = Image.new("RGB", (512, 264 * len(frames)))
    for i, frame in enumerate(frames):
        strip.paste(frame, (0, 264 * i))
    strip.save(out / name, optimize=True)

frames = [colour(f"tour_{i:02}") for i in range(18)]
sheet(frames, "clipplus-screens.png")
dolphins = [colour(f"dolphin_frame_{i:03}") for i in range(200)]
sheet(dolphins, "clipplus-dolphins.png")
dolphins[40].save(out / "clipplus-dolphin-still.png")
dolphins[0].save(out / "clipplus-dolphins.gif", save_all=True,
                  append_images=dolphins[1:], duration=100, loop=0, optimize=False)
print(f"Captured 18 tour screens and 200 dolphin frames in {out}")
