# Branding transitions

The application-owned Clip+ startup transition now shows a quiet Darren mark
with the shared focus typography instead of Rockbox's logo/version screen.
The application-owned shutdown transition similarly shows Darren briefly
before the LCD powers down. The custom home screen no longer labels itself
`HOME`; the shared header only shows meaningful context such as EQ, Music,
Podcasts, or Now Playing.

The earliest splash belongs to the installed Clip+ bootloader, before the
application image runs. Removing that branding requires a separately rebuilt
and recovery-verified bootloader artifact based on the exact stock image; it
is intentionally not part of this application-only package and has not been
written to the player.
