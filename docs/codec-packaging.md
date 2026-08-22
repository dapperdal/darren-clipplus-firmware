# Clip+ codec packaging

Rockbox's standard target package places loadable codec binaries in
`.rockbox/codecs/`. The Clip+ MP3 decoder is named `mpa.codec`; FLAC is
`flac.codec`. The staged focus update includes both ARM codec binaries built
from the same Clip+ fork/configuration as `rockbox.sansa`.

The package verifier requires both codecs, the application image, and the
Rockbox build-info file. It fails closed when `mpa.codec` is absent, so a
firmware-only package cannot be staged as a playable update again.

This packaging step does not prove hardware decoding or playback. It proves
that the expected runtime resources exist at the official `.rockbox/codecs`
paths and were built from the matching fork build directory.
