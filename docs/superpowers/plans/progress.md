# SDD ledger — plan: docs/superpowers/plans/2026-11-09-spectral-smoother.md
Pre-flight: Task 2 consumes Task 1 stereo processor; Task 3 verifies Task 2 AU wrapper. Interfaces consistent.
Ruling: Work in the existing fresh clone on codex/custom-flavors; no parallel user changes or extra worktree needed.
Ruling: Proceed with native execution under existing implementation approval; no repeated approval checkpoint.

Task 1: complete — tests/run.sh passes under ASan/UBSan at 44.1/48/88.2/96 kHz. Initial missing-source RED, then GREEN. Impulse reconstruction additionally caught unsigned envelope-coefficient negation; corrected and passed with 2.98e-8 error.
Task 2: complete — separate ScreamCustom universal AU, full-range UI, legacy reserved-byte state extension, font and notices bundled. Compile caught a missing shared function declaration; corrected.
Task 3: complete — auval succeeds; native AU tests pass four rates with nonzero input callback, variable blocks, audio-thread state recall, and reset. Logic Pro 13.0 listed and opened Scream Custom alongside Cure Audio Scream.
Final review: independent reviewer found the harness's null callback context prevented input callback invocation. Fixed with non-null context, callback-count and energy assertions; native tests pass.
Final: restored parameters could be overwritten by older queued events. Audio-thread recall assertion failed first; restoring through the existing event queue fixed it; full AU suite passed.
Ruling: FFT size 4096 at 44.1/48 kHz, 8192 at 88.2/96 kHz, four overlaps. Added 85–93 ms latency enables useful bass resolution; slower than a live tracking effect, explicitly accepted by user.
Ruling: Ship the self-contained universal AU with local ad-hoc signing and keep the feature branch; public release notarization and other plugin formats are outside this task.
UI verification: first Logic screenshot revealed logos obscuring Smooth label. Restricted logos to original 32px title row and rebuilt; installed final binary. Logic already loaded the earlier module and should be reopened to pick up final fixes.
Limit: objective DSP/host checks completed; subjective listening on the user's music and full Logic project reopen testing were not performed. State round-trip and audio-thread application verified by native AU harness.
