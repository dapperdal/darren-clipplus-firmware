# Storage and media policy

## Current product preference

The first firmware target prioritizes the Clip+'s built-in 8 GB storage. microSD is optional future expansion and must not be required for boot, library indexing, podcast playback, long mixes, or resume state.

## Media-model requirements

- Keep the internal volume as the primary library root and make it usable without a card inserted.
- Store a crash-safe media index with incremental rescans, so a large podcast/mix library does not require a full scan at every boot.
- Persist resume checkpoints by stable media identity plus byte/sample position, with atomic replacement and recovery after power loss.
- Treat podcasts and long mixes as first-class resumable media, preserving per-item position rather than only a global last-track position.
- Make storage errors and removed/inconsistent media recoverable without losing the internal index or playback state.
- Add microSD as a separately mounted, optional library root later; card insertion/removal must not invalidate internal-storage playback.

The index format, wear budget, free-space policy, and recovery journal still need to be designed and tested against the actual device filesystem and power-loss behavior.

