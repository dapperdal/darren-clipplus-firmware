# Battery estimate

Custom Darren screens show a small battery status readout in the header or
Now Playing status area. When Rockbox reports a calibrated estimate it appears
as `BAT 2H15` (an approximate two hours and fifteen minutes remaining). While
charging it shows `CHG`; when the estimate is unavailable it shows `BAT --`.

The target reads Rockbox's existing `battery_time()` and `charging_state()`
values. The value is refreshed at most every 30 seconds to avoid unnecessary
power use and redraw churn. It is guidance, not a guarantee: codec, volume,
backlight, storage activity, battery age, temperature, and calibration affect
actual time. No new battery calibration model is introduced by the focus shell.

The host simulator covers calibrated, charging, and unavailable fallback
states. Real Clip+ readings still need manual comparison against playback and
charging conditions before treating the estimate as accurate.
