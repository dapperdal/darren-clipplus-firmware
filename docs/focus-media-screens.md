# Focus media screens

The application-only fork now keeps Music and Podcasts out of the Rockbox
root/file browser. Each destination directly scans `/MUSIC` or `/PODCASTS`,
uses a bounded list of supported audio files, and presents one large readable
selection at a time with wrapping Up/Down navigation.

Select follows Rockbox's proven audio-file path: it replaces the current
playlist with the selected directory, inserts exactly one selected absolute
track, then starts the audio thread. Passing an MP3 as `playlist_create`'s
file argument is explicitly avoided because that parses binary audio as a
playlist and causes `PLAYLIST BUFFER FULL`. Now Playing waits briefly
for the asynchronous current-track metadata to become valid; it shows
`STARTING`, then a persistent `PLAYBACK ERROR` if the handoff fails, never a
false `NO TRACK` success state. A host mock regression repeats four sequential
selections across Music and Podcasts to verify playlist/current-track state
cannot leak.

Now Playing is custom: Select toggles pause/resume, Up/Down requests the
previous/next track, and Home returns to the focus shell. Radio currently has
a controlled `FM READY` status wrapper. Recordings is intentionally gated as
`NOT READY` pending diagnosis of the false disk-full report despite free
internal storage. Neither destination opens a stock Rockbox menu.

## Now Playing control legend

| Control | Action |
| --- | --- |
| Select or Clip+ Play/Pause (Up release) | Play/pause |
| Volume Up/Down | Adjust normal Rockbox volume; temporary full-screen scale |
| Left/Right, including repeat/hold | Seek backward/forward in 5-second steps |
| Home | Back to the focus shell |
| Power hold | Standard Clip+ shutdown |

The screen keeps a readable title, PLAYING/PAUSED/STARTING state, elapsed /
remaining / total time, and progress bar visible. Seek and volume feedback
uses a temporary full-screen overlay only for volume, then restores the
transport automatically.

Volume feedback is now a temporary full-screen stepped scale: ten rising bars,
the current volume value, and Darren styling. It resets on repeated volume
presses and expires after two seconds, returning to the prior Now Playing
transport. The host model checks low, middle, and high bounds plus timeout.

Desktop review prototypes:

- `outputs/clipplus-media-prototype.png`
- `outputs/clipplus-now-playing-prototype.png`

The simulator self-test covers the large media layout, explicit playback
error state, Now Playing, and selected-path-to-current-track handoff. The ARM
application image builds locally, but target display, storage scan, and
playback behavior remain unverified until manual Clip+ testing.
