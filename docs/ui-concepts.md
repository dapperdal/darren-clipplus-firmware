# UI concepts

The simulator currently produces two independent 128×64 home-screen previews:

- `build/clipplus-home.pgm`: centered three-item carousel with icon treatment.
- `build/clipplus-list.pgm`: minimal text-only vertical menu with generous spacing and a single selection bar.
- `build/clipplus-single.pgm`: one large centered destination at a time, navigated with Up/Down, with six quiet position dots.

Both views use the same six destinations — Music, Radio, Recordings, Settings, Podcasts, and Now Playing — plus the same `home_index`, physical-control input handling, and long-press shortcuts. Podcasts is reserved for resumable podcast/audiobook-style playback in future target integration. The list view intentionally has no icons, row boxes, decorative grid, or bottom status strip; playback remains available through the Play control and Now Playing.

The single-item view is deliberately sparse: it has no icons, full list, bottom strip, or decorative chrome. Small Up and Down arrows above and below the title make the physical navigation discoverable; Left/Right has no hint because it has no action on this screen. Now Playing is split over two large readable lines because the full phrase cannot fit at the same type size on a 128-pixel-wide display.
