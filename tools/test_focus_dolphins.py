#!/usr/bin/env python3
"""Exercise standby, playback, USB and display timeout in a built Clip+ sim.

Usage: test_focus_dolphins.py BUILD_DIR [--capture]
With --capture, also save one complete 20 s scene to shots/dolphin_frame_*.pgm.
The simulator's configuration/script/session files are restored after testing.
"""
import os
from pathlib import Path
import re
import subprocess
import sys

build = Path(sys.argv[1]).resolve()
disk = build / "simdisk"
capture = "--capture" in sys.argv[2:] or "--capture-only" in sys.argv[2:]
config = disk / ".rockbox/config.cfg"
script = disk / "focus_script.txt"
session = [script, config] + list((disk / ".rockbox").glob(".resume.cfg*")) + list(
    (disk / ".rockbox").glob(".playlist_control*"))
saved = {p: p.read_bytes() for p in session if p.exists()}
env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")


def run(name, commands, settings=""):
    config.write_text("backlight timeout: 0\nshuffle: off\nrepeat: all\n" + settings)
    script.write_text(commands + "\nq\n")
    result = subprocess.run([str(build / "rockboxui")], cwd=build, env=env,
                            capture_output=True, text=True, timeout=90)
    log = result.stdout + result.stderr
    (build / f"test-{name}.log").write_text(log)
    assert result.returncode == 0, log
    assert "Can't play" not in log and "PANIC" not in log, log
    shots = {m[0]: tuple(map(int, m[1:])) for m in re.findall(
        r"SIMSHOT (\S+) audio=(\d+) volume=(-?\d+) storage=(\d+) display=(\d+)", log)}
    return log, shots


def pixels(name):
    data = (disk / "shots" / f"{name}.pgm").read_bytes()
    return data.split(b"\n", 3)[3]


try:
    if "--capture-only" not in sys.argv[2:]:
        log, shots = run("home-usb", """w 5
    s test_home
    w 85
    s test_idle
    k HOME
    w 3
    s test_wake
    U 1 20
    w 5
    s test_usb_one
    w 85
    U 1 20
    w 5
    s test_usb_two""")
        assert pixels("test_home")[128*16:] != pixels("test_idle")[128*16:]
        assert pixels("test_home")[128*16:] == pixels("test_wake")[128*16:]
        assert log.count("All threads have acknowledged the connect.") == 2, log
        assert shots["test_usb_one"][2] == 1 and shots["test_usb_two"][2] == 2, log
        print("PASS Home standby, wake, USB from Home and from standby (all ACKs)", flush=True)

        # Music > Shuffle all; dismissing standby with SELECT must not pause it.
        log, shots = run("playback", """w 5
    k SELECT
    w 3
    k SELECT
    w 20
    s test_playing
    w 85
    s test_play_idle
    k SELECT
    w 3
    s test_play_wake
    k VOLDOWN
    w 3
    s test_volume
    U 1 20
    w 8
    s test_play_usb""")
        assert shots["test_playing"][0] & 1, log
        assert shots["test_play_idle"][0] & 1, log
        assert shots["test_play_wake"][0] & 1 and not shots["test_play_wake"][0] & 2, log
        assert shots["test_volume"][1] < shots["test_play_wake"][1], log
        assert shots["test_play_usb"][2] == 1, log
        assert "All threads have acknowledged the connect." in log, log
        print("PASS Playback continues through standby/wake; volume and USB work", flush=True)

        log, shots = run("timeout", "w 5\ns test_lit\nw 30\ns test_dark",
                         "backlight timeout: 2\n")
        assert shots["test_lit"][3] == 1 and shots["test_dark"][3] == 0, log
        print("PASS OLED timeout is respected", flush=True)

        log, shots = run("disabled", "w 5\ns test_off_home\nw 100\ns test_off_later",
                         "dolphin standby: off\n")
        assert pixels("test_off_home")[128*16:] == pixels("test_off_later")[128*16:]
        print("PASS Standby can be disabled through saved settings", flush=True)

    if capture:
        commands = "w 5\nh DOWN 4\nw 12\nk SELECT\nw 3\nh DOWN 7\nw 12\ns test_dolphin_menu\nk SELECT\nw 3\n"
        commands += "\n".join(f"s dolphin_frame_{i:03}\nw 0" for i in range(200))
        commands += "\nk HOME\nw 3\ns test_dolphin_menu_return"
        log, shots = run("capture", commands)
        assert len({pixels(f"dolphin_frame_{i:03}") for i in range(200)}) > 150
        assert pixels("test_dolphin_menu")[128*16:] == pixels("test_dolphin_menu_return")[128*16:]
        print("PASS Manual Dolphins entry and return; captured 200 animated frames", flush=True)
finally:
    current = set([script, config] + list((disk / ".rockbox").glob(".resume.cfg*")) + list(
        (disk / ".rockbox").glob(".playlist_control*")))
    for path in current:
        if path in saved:
            path.write_bytes(saved[path])
        else:
            path.unlink(missing_ok=True)
