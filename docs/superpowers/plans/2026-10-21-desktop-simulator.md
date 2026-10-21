# Desktop Simulator Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan inline.

**Goal:** Run the actual Blob Watch UI on desktop and test interactions and rendering without flashing hardware.

**Architecture:** Compile the existing pages, navigation, theme, timer and LVGL port against a desktop board adapter. SDL2 presents the framebuffer; a deterministic clock and injectable touch samples support headless integration scenarios and BMP screenshots.

**Tech Stack:** C++17, CMake, SDL2, LVGL 8.3.11, CTest, Python standard library.

**Spec:** Approved conversation: 240×240 circular display, shared UI sources, mouse touch/swipes, simulated time/sensors/brightness, screenshots and automated navigation/slider/timer checks. New branch and PR based on `feat--timer` (the repository's timer branch).

## Constraints
- Keep firmware behavior unchanged; desktop shims belong in simulator/.
- Keep hardware adapter implementation in src/platform/ and select it only in the desktop build.
- Use the firmware LVGL config, including RGB565 byte swapping and memory size.
- No duplicated UI or test-only accessors in production pages.

## Review focus
- A held swipe changes one page, including release and the opposite direction.
- Touch coordinates remain correct after resizing; round bezel rejects outside presses.
- RGB565 byte order preserves colors; screenshots share presentation masking.
- Slider dragging reaches limits and records PWM requests without navigation.
- Timer pause/resume/expiry and zero duration use deterministic time.

## Tasks
- [x] Add a failing Python integration contract that builds/runs the simulator and validates its screenshots and JSON report.
- [x] Add CMake dependency setup and desktop Arduino/vendor compatibility shims; compile the real app sources and a desktop board implementation.
- [x] Add SDL display, circular masking, mouse/swipe input, keyboard navigation/tilt/time acceleration and deterministic headless runtime.
- [x] Add integration scenarios for repeated navigation gestures, real pointer slider dragging, timer buttons/pause/resume/expiry/cancel, and deterministic screenshots; verify contract passes.
- [x] Document install/build/run/test commands and limitations. Add CI to build/test and retain screenshots.
- [x] Run host regression, CTest, screenshots inspection and pio run; obtain independent review, fix material findings, commit/push and open PR targeting feat--timer.

## Execution record
- Baseline: clean feat--timer at d6f14b7; existing navigation regression passed in previous integration. New branch codex/lvgl-desktop-simulator.
- Ruling: use the current clean checkout and new branch requested by the user; no extra worktree or planning approval needed because implementation is explicitly authorized.

- Implementation: shared source desktop build, SDL window/dummy driver, 32 real behavior checks, RGB565/circular BMP+PNG captures, repeat-run equality and committed visual references.
- Verification: host CTest 2/2 passes in Debug and clean downloaded-LVGL Release builds; pio run passes. Inspected all eight UI captures. Interactive SDL window launched and exited successfully.
- Independent review: found dropped batched mouse clicks; reproduced with failing actual-Start-button test, then delivered each pointer transition to LVGL without advancing virtual time. Regression passes; follow-up reviewer reports no remaining material findings.
- Ubuntu 24.04 container: CTest 2/2 passes, including the same visual hashes as macOS. Branch pushed and PR #3 opened toward feat--timer; GitHub Actions performs the same checks remotely.
