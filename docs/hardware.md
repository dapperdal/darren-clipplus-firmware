# Clip+ hardware baseline

## Known target facts

These values are taken from the supplied Rockbox target configuration and target-specific files in the authorized local reference tree:

| Area | Baseline |
|---|---|
| Display | 128×64, 1-bit, vertical-packing framebuffer; LCD split at line 16 |
| SoC | AMS/AS3525v2 family; target CPU definition is 192 MHz |
| Storage | SD-protocol flash storage plus hot-swap/microSD support |
| Inputs | Up, Down, Left, Right, Home, Select, Power, Volume Up, Volume Down |
| Audio | Hardware supports common playback sample rates; software tone controls are needed |
| FM | SI4702-compatible tuner path, with RDS support in the reference |
| Recording | Microphone and FM recording paths are defined; multiple recording sample rates |
| Battery | 290 mAh default/min/max setting in the reference configuration |

The 128×64 LCD geometry is the design constraint for the first UI. The 192 MHz value is a reference target definition, not a performance guarantee for this project.

## Supplied production image

- Source: user-supplied production image, retained outside this repository
- Size: 15,728,640 bytes
- SHA-256: `66c6d0cb0dfffdbe77f3c53846d8b5fc495a4b1ce8b190cc07ff46f184fcc592`

The image is not copied into this project and has not been modified.

## Porting limitations

The current project has no verified linker script, bootloader handoff, codec binaries, LCD timing validation, storage driver, tuner driver, recorder path, or safe update format. The target-specific Rockbox files identify likely hardware boundaries but do not by themselves establish that an independent firmware image can safely replace the production firmware.
