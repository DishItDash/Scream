# Scream spectral smoother

Status: approved by the user in chat.

## Intent

Add full-range adaptive resonance reduction after Scream's distortion. The user chose spectral smoothing and accepts added latency. The purpose is to tame excessive peaks while retaining aggressive character.

## Approaches

Spectral smoothing can attenuate many moving resonances simultaneously, at the cost of buffering latency and possible transient softening. Tracking dynamic notch filters offers lower latency but follows fewer peaks. The user selected spectral smoothing.

## Behavior and controls

Detect peaks relative to a smoothed local spectral envelope, rather than simply attenuating every loud frequency. Apply smoothly varying, attenuation-only gain across frequency and time. Analyze the distorted signal across the audible range, including bass and mids; usable range is limited by sample rate and spectral resolution. This first version detects output resonances, not proven differences from the input.

Proposed controls: Smooth Amount (0–100%), Maximum Reduction (0–12 dB), and Listen to Removed Sound. Start with smoothing at zero for unchanged tone, a 6 dB reduction cap, and full-range detection. No adjustable focus range in the first version. These controls and defaults are implementation proposals, not previously selected user preferences.

Listen auditions the difference between the latency-aligned distorted signal and its smoothed version, without dry signal or automatic gain boost. It is a temporary audition mode, not restored on session reload.

## Signal path and latency

Input -> existing Scream distortion -> spectral smoother -> dry/wet blend -> existing output gain. Keep the smoother outside Scream's feedback loop. Preserve current input gain and modulation behavior. The current implementation blends wet and dry inside the distortion sample loop; move that blend after smoothing while preserving its parameter smoothing and modulation.

Delay dry audio and corresponding mix-control timing to align with the buffered wet path. Use a constant latency for a given sample rate, including when Smooth Amount is zero, to avoid timing jumps. Report the actual latency through the AU wrapper for host delay compensation. At zero smoothing, return the original distorted signal with the same delay. Smooth changes into and out of processing.

Use linked stereo detection and the same reduction curve for both channels to avoid image wandering. Initialize FFT plans and allocate buffers outside the audio callback. Process arbitrary host block sizes without callback allocation, locks, or I/O. Clear buffered audio on reset and sample-rate changes.

FFT size, overlap, envelope bandwidth, and attack/release constants are builder choices to tune with measured reconstruction, bass resolution, listening, and latency results. Prefer approximately constant analysis duration across supported sample rates. Publish measured latency rather than promising an unverified number.

## Integration

Keep the existing C/CPLUG implementation and AUv2 target. Append new parameter IDs without renumbering existing ones. Persist smoothing amount and reduction limit, with zero smoothing when loading older states. Give the custom build a separate display name, bundle identifier, AU subtype, and preset/settings location where applicable, so it can coexist with upstream Scream. Provisional name: Scream Custom. Retain upstream license notices.

## Verification and boundaries

Verify identity reconstruction with no reduction; delayed dry and zero-amount paths; maximum attenuation bounds; silence and finite output under extreme settings; stereo linking; arbitrary block sizes; and 44.1, 48, 88.2, and 96 kHz operation. Check automation, reset, session recall, and reported latency. Build the AU, run Apple's AU validator, and verify loading and session recall in the user's installed Logic version before claiming host compatibility.

Listen on bass, drums, sustained synths, and sweeps to assess peak reduction, bass preservation, and transient softening. Use loudness-matched comparisons and the removed-sound audition. This is not an output limiter, a clone of Baby Audio's proprietary algorithm, or an attempt to remove every harmonic. New distortion flavors and input/output comparative detection are outside this first change.
