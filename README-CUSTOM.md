# Scream Custom 1.1.0

Personal fork of Cure Audio Scream with full-range spectral resonance suppression.

## In Logic Pro

Choose **Audio FX → Audio Units → DishItDash → Scream Custom → Stereo**.

The original Cure Audio Scream remains separate. This build is stereo-only, like upstream.

- **Smooth:** drag vertically or use the mouse wheel to set strength. Start around 30–50%; zero preserves the original distorted tone with the same latency. Double-click resets to zero.
- **Max Reduction:** drag vertically to cap per-frequency attenuation between 0 and 12 dB. Default 6 dB; try 3 dB for gentle smoothing. Double-click resets to 6 dB.
- **Listen to Removed:** audition only the removed signal. Click again to return to normal. This mode is not stored in sessions.
- The existing **Wet** control blends the smoothed distortion with latency-aligned dry audio. Existing input autogain behavior remains unchanged.

The smoother follows peaks throughout the spectrum relative to their local spectral neighborhood. It does not know whether Scream created a resonance or inherited it from the input. Strong settings can attenuate intentional notes/resonances and soften attacks. It is not a peak-output limiter.

Latency is 4096 samples at 44.1/48 kHz (92.88/85.33 ms) and 8192 samples at 88.2/96 kHz (92.88/85.33 ms). It stays constant when Smooth is zero, and is reported to Logic for compensation. Use normal plug-in delay compensation; a host's low-latency mode may bypass high-latency effects.

## Build and install

Requires macOS Command Line Tools, CMake, and the official Sokol shader compiler. Initialize dependencies with `git submodule update --init --recursive`. Set `SOKOL_SHDC` to your `sokol-shdc` executable, then run `sh tools/build-macos.sh`.

The universal arm64/x86_64 AU is `build/custom/Release/ScreamCustom.component`. Build does not install automatically. Quit Logic before replacing a loaded build, then copy it to `~/Library/Audio/Plug-Ins/Components/`. Reopen Logic. The component includes its font and license notices. This local build is ad-hoc signed, not notarized for public distribution.

Only the AU is validated for this fork. Existing upstream installer scripts are not the custom installer; use the instructions above.

## Validation

- `sh tests/run.sh`: address/undefined sanitizer tests of streaming DSP, impulse reconstruction, delayed identity/mix, stereo linking, full-range tones, cap, reset, audition, and automation.
- `sh tests/run-au.sh`: actual AU loading, parameters, current/legacy state recall (including audio-thread values after rendering), sample-rate/variable-block handling, nonzero input callback audio, and reset-to-silence.
- `auval -v aufx Scsm Dshd`: Apple Audio Unit validation after installation.

Validated on this Mac with Logic Pro 13.0: component discovery and editor loading. Native tests cover 44.1/48/88.2/96 kHz; auval additionally exercises its standard formats and automation. Listening evaluation on the user's music remains the next step; no claim of sonic equivalence to Baby Audio Smooth Operator is made.

## Implementation

A preallocated PFFFT STFT uses Hann windows and four overlapping frames. Stereo-linked local spectral contrast drives attenuation with 10 ms attack and 120 ms release. Wet audio, dry audio, and wet-mix automation share a fixed delay; smoothing sits outside the nonlinear feedback loop. New parameter IDs 14 and 15 leave upstream IDs unchanged. Two reserved state doubles carry settings; legacy states load with smoothing off.

`cmake/CustomAU.cmake` patches the pinned wrapper into the build directory to report latency/tail in seconds, clear DSP on reset, prepare the default sample rate, and report the actual parameter-list length. Submodule source remains untouched. Upstream update checks are disabled to avoid offering an incompatible upstream installer.

Cure Audio and dependency notices are retained in LICENSE and bundled resources. PFFFT's notice is in src/libs/pffft.h; the font's license is assets/OFL.txt.
