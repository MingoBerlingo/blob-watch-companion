# Running Timer Controls Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan inline.

**Goal:** Implement the supplied running/paused timer reference while preserving the setup, Blob and Brightness screens.

**Architecture:** Keep the shared LVGL pages and timer state machine. Draw the segmented ring and geometric button icons with LVGL primitives; route the back button through the page manager. Store RGB565 action colors in shared_state.h and reuse the existing violet theme accent.

**Tech Stack:** Existing LVGL 8.3.11, RP2040 PlatformIO firmware, CMake/SDL simulator.

**Spec:** User's attached flow: running screen with segmented violet progress, large remaining time and orange pause; paused screen with smaller time, silver back-to-Blob, red cancel-to-setup and green resume. Only started timer UI changes. Icons use basic drawing primitives. Colors chosen from newdigate/rgb565_colors README. The user authorized implementation and asked questions up front; expiry clarification was offered asynchronously.

## Constraints and decisions
- Preserve setup/Blob/Brightness rendering and navigation outside the new back action.
- Pause freezes the remaining duration and reveals actions; resume hides actions.
- Back goes to Blob and retains the paused timer, including at full duration; reentry shows actions.
- Cancel resets to setup. Expiry follows the second supplied image: original duration in gray, Timer ended title, gray back-to-setup and orange repeat. No ring or paused actions.
- Keep all colors RGB565; use existing theme violet and black/white. New colors: Davy's gray 0x52AA, Orange 0xFD20, Screamin Green 0x67EC, Fiery Rose 0xFAAE, Silver 0xBDF7.
- No bitmap/SVG assets for geometric icons. Use custom LVGL draw callbacks and circular touch targets.

## Review focus
- Freeze exactly at click time, including unsigned millis rollover and pause at expiry.
- Back/return preserves a paused timer even before any elapsed time.
- Resume returns to the running screen; background taps do not pause or toggle actions.
- Setup and other pages must retain their existing image hashes.
- Ring and icons fit the circular display, update with remaining duration and use no clickable overlay.

## Tasks
- [x] Write failing host tests for pause/actions, resume, expiry/restart and paused exit/reentry.
- [x] Update shared configuration, timer logic and back navigation; verify host tests.
- [x] Implement primitive icon/ring drawing and running/paused layouts while leaving setup untouched.
- [x] Update simulator scenarios to exercise real pointer buttons, back/return, cancel, restart and unchanged non-timer screens. Inspect screenshots before changing timer visual baselines.
- [x] Run CTest, pio run and independent review; fix material findings, commit/push and open a PR into feat--timer.

## Execution record
- PR #3 merged into feat--timer at b86b6f9. Work on codex/timer-running-controls.

- User clarification: a second reference specifies the ended screen and its two actions. This replaces the provisional expiry default.

- TDD: the first host test failed on pause/action state, then passed after logic changes. The new simulator pointer flow failed on the old UI, then passed. A visual check caught the first-run time font; added a failing height regression and fixed it.
- Validation: 43 integration checks and all 3 CTest targets pass; host timer tests include rollover, precise pause, full-duration reentry, pause-at-expiry and background expiry.
- Visual review: inspected 10:00 running, paused and ended renders against both supplied images. Existing setup, Blob, Brightness and color-boundary captures retain their original hashes.
- Firmware: pio run passes (45,672 bytes RAM; 318,372 bytes flash). Independent reviewer reports no material findings.

- Published branch codex/timer-running-controls and opened PR #4 targeting feat--timer. GitHub Actions runs the same integration/visual checks remotely.

- CI follow-up: all 43 behavior checks passed on Linux, but 64 ring pixels differed due to host libm half-pixel rounding. Replaced floating-point ring geometry with LVGL integer sine interpolation/projection; re-inspected ring references, reran local tests, and obtained a second independent review with no material findings. Increased the time formatting buffer to 16 bytes for GCC range diagnostics.
