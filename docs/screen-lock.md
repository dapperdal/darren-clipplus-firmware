# Home screen lock

Hold Home to activate the native Rockbox soft-keylock path in the custom
Clip+ contexts. While locked, normal navigation, playback, volume, and seek
buttons are ignored, while music continues. A full-screen `LOCKED / HOLD
HOME` notice appears for two seconds. Hold Home again to unlock. A short Home
press remains the normal one-level Back action, and Power remains standard
shutdown.

The host simulator regression covers lock activation, visible lock notice,
blocked Down input, playback continuity, notice timeout, and held-Home unlock.
The target build uses the reference Clip+ repeat mapping; manual hardware
verification should confirm the hold duration, audio continuity, and the
lock/unlock feedback before release.
