# Audio format roadmap

## FLAC — planned, not implemented

FLAC is a supported planned format for ClipPlus. The current simulator does not decode audio files and the target firmware does not yet contain a decoder; this entry is a roadmap contract, not a claim of working playback.

The authorized Rockbox reference demonstrates that FLAC is feasible on the Clip+ class target:

- `lib/rbcodec/codecs/flac.c` provides the codec entry point and resume/seek handling.
- `lib/rbcodec/metadata/flac.c` parses STREAMINFO and metadata.
- `lib/rbcodec/codecs/libffmpegFLAC` supplies the decoder library.
- The codec build maps `flac.codec` to `libffmpegFLAC.a`.
- The reference decoder defaults to a 4,608-sample maximum frame block and stereo channel support; its source describes 8,192-sample builds for devices with sufficient processor/memory headroom.

## Independent integration path

1. Define a small `audio_decoder` boundary for file reads, seeks, sample output, yielding, and cancellation; keep the UI and storage index independent of codec implementation.
2. Choose a license-compatible FLAC decoder after reviewing complete provenance and notices. Do not copy Rockbox codec files into this project without deciding how their GPL/LGPL obligations will be met.
3. Start with native FLAC limits: stereo, 16/24-bit input converted to the player’s PCM path, standard 4,608-sample frames, and a bounded metadata/seek-table budget.
4. Link the decoder as a loadable or separately bounded codec component so its code and working buffers cannot overwrite the UI, resume state, or storage buffers.
5. Add indexed extension recognition, duration/seek metadata, resume checkpoints, and a clear “unsupported FLAC parameters” error for files outside the validated profile.

## Resource and reliability constraints

The reference target defines a 192 MHz AS3525v2 CPU, a 0xF8000-byte codec region, and a 0x80000-byte plugin buffer. Those are useful planning bounds, not yet verified memory maps for this independent firmware. FLAC’s per-frame predictor/residual buffers, PCM output, file read-ahead, tag parsing, and seek table must be measured together. Higher sample rates, large blocks, multichannel files, 24-bit output, and album art increase pressure on both CPU time and RAM; downmixing must not be assumed free.

## Required verification before enabling playback

- Host decoder tests against official FLAC reference output, including CRC and malformed-stream cases.
- Target-sized memory/link map with stack high-water marks and codec-buffer accounting.
- Sustained playback at supported sample rates with underrun instrumentation and battery-current measurements.
- Seek, pause/resume, power loss, storage removal, and corrupt-file recovery tests.
- Corpus coverage for mono/stereo, 16/24-bit, compression levels, block sizes at and beyond the supported limit, large metadata, and missing/invalid seek tables.
- UI acceptance: format, bitrate/sample rate, duration, and actionable unsupported-file status fit within the 128×64 display.

