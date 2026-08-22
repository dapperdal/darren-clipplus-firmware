# Focus-shell equalizer

The Rockbox-based Clip+ fork now includes a compact EQ screen reached by
Select on the Settings item in the custom first-boot shell. It is not a
plugin or a deep settings maze.

- five visible bands fit the 128×64 screen;
- Up/Down changes the selected band's gain immediately in 1 dB steps;
- Select advances the active band;
- Home returns to the focus shell;
- each band is bounded to -12 dB through +12 dB;
- bars and numeric values provide a quick visual readout.

The implementation calls Rockbox's existing `dsp_set_eq_coefs()` path and
updates `global_settings.eq_band_settings`, retaining the proven DSP rather
than duplicating filter math. The other five Rockbox EQ bands remain available
to the underlying subsystem for later policy decisions; the first focus-shell
slice exposes five useful bands to keep the display readable.

The fork application and scrambled image were rebuilt after this change. The
screen has only been compile/link checked; target LCD rendering, action
mapping, persistence, clipping, and audible DSP behavior still require staged
manual hardware verification. No image has been installed.
