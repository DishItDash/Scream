# Spectral Smoother Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Deliver a separate Scream Custom AU with full-range spectral peak reduction.
**Architecture:** A streaming stereo STFT module with preallocated buffers handles reduction, delayed dry/mix, and removed-sound audition. Existing C/CPLUG DSP, parameters, UI and state wrap that module.
**Tech Stack:** C99, vendored PFFFT, CMake, CPLUG, macOS AUv2.
**Spec:** ../specs/2026-11-09-spectral-smoother-design.md

## Global Constraints
- Full-range, attenuation-only, linked stereo, outside the feedback loop.
- Constant reported latency; aligned dry and mix control; zero amount preserves tone.
- No callback allocation in the new processor; retain existing IDs and legacy state loading.
- Separate Scream Custom identity and settings; preserve licenses.

## Review Focus
- Variable and single-sample blocks produce identical timing (Task 1).
- Abrupt automation and audition transitions remain finite and bounded (Task 1).
- Bass and treble peaks both receive reduction, with no stereo drift (Task 1).
- Old states default to no smoothing; audition is never recalled (Task 2).
- Host reset flushes buffered audio and AU latency is in seconds (Task 3).

### Task 1: Streaming smoother
**Files:** src/spectral_smoother.h, src/spectral_smoother.c, tests/test_smoother.c, tests/run.sh.
**Interfaces:** opaque SpectralSmoother; create(rate), destroy, reset, latency; process stereo wet/dry/mix samples with amount, max dB and audition arguments.
- [x] Write tests for delayed identity, zero-limit reconstruction, stereo scaling, silence/reset, peak attenuation at bass/mid/treble, gain cap, and block-independent streaming.
- [x] Run tests and observe missing implementation failure.
- [x] Implement PFFFT STFT with Hann windows, 4x overlap, local spectral envelope, attack/release gain and parameter smoothing.
- [x] Run tests with address/undefined sanitizers; expect PASS. Commit.

### Task 2: Plugin integration and UI
**Files:** src/plugin.c, src/plugin.h, src/params_and_events.c, src/state.c, src/gui.c, CMakeLists.txt.
**Interfaces:** use Task 1 processor after stereo distortion and before output gain. Append amount and max-reduction host parameters; audition via atomic UI state.
- [x] Add integration checks for default values, round-trip/legacy state, latency, and parameter formatting.
- [x] Preserve raw wet, dry and mix per channel using preallocated processor input storage; consume after both channels.
- [x] Add accessible labeled controls to existing UI; name and identifiers become Scream Custom / DishItDash, update checks disabled for custom build.
- [x] Verify tests and compile AU. Commit.

### Task 3: Build, host validation, and review
**Files:** cmake AU wrapper patch, tools/build-macos.sh, tests/test_au.c, README-CUSTOM.md.
- [x] Fix upstream AU latency units and reset callback through a build-local patched wrapper (no submodule commit needed).
- [x] Build universal AU and sign locally; install only ScreamCustom.component.
- [x] Run auval and native AU integration harness: latency seconds, reset, state, controls, varied sample rates and blocks. Expect pass.
- [x] Check GUI and loading in installed Logic; document host version and limits (loaded in Logic 13.0; final UI adjustment requires host reopen, musical listening still pending).
- [x] Fresh whole-branch review, resolve material findings, rerun affected checks, commit results and user instructions.
