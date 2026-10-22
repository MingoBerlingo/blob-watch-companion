# AGENTS

Repository guidance for coding agents working on Blob Watch Companion.

## Project Summary

- PlatformIO firmware for Waveshare RP2040-Touch-LCD-1.28.
- Main app code lives in `src/`.
- Native board and peripheral drivers are in `driver/`.
- Reference vendor demo lives in `examples/waveshare_native/`.

## Key Paths

- `platformio.ini`: PlatformIO environments and build settings.
- `src/touch_lcd_demo.ino`: Main firmware entry point.
- `src/platform/waveshare_native_board.*`: Board abstraction layer.
- `src/apps/blob_native/`: Blob Watch app logic (state, draw, face, overlay).
- `driver/board/`: Low-level board config helpers.
- `driver/display/`: LCD and paint stack.
- `driver/sensors/`: Touch and IMU drivers.

## Build And Flash

Use PlatformIO from the repository root.

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

If upload fails, instruct user to enter BOOTSEL mode and retry.

## Editing Guidelines

- Keep hardware-facing changes in `src/platform/` or `driver/`.
- Keep app behavior and UI logic in `src/apps/blob_native/`.
- Do not rename public driver files or symbols without explicit request.
- Naming rule for scoped code: keep public API names explicit (for example, `timer_*`), but avoid repeating the domain prefix for private helpers inside narrow namespaces (for example, use `sync_total_ms` inside `timer_internal` instead of `timer_sync_total_ms`).
- Preserve existing Arduino/PlatformIO style and includes.
- Prefer minimal, targeted patches and avoid unrelated refactors.

## Validation Guidance

After code changes, run at least:

```bash
pio run
```

If serial behavior changed, also verify monitor output at 115200 baud.

## Desktop UI Validation

For UI, navigation, or timer changes, also build and test the shared-source desktop simulator:

```bash
cmake -S simulator -B simulator/build -DCMAKE_BUILD_TYPE=Release
cmake --build simulator/build --parallel
ctest --test-dir simulator/build --output-on-failure
```

Inspect the generated PNGs in `simulator/build/screenshots/` when changing visuals.
Only update `simulator/tests/visual-baselines.json` and its PNG references after
reviewing an intentional UI change; ordinary test runs must not update them.
See `simulator/README.md` for dependencies, controls, and explicit baseline updates.
Firmware validation with `pio run` is still required.

## Local Tests Only

Run tests locally. Do not trigger, re-run, enable, or use GitHub Actions/CI for
validation unless the user explicitly requests it. Preserve manual-only workflow
triggers; push and pull-request activity must not start paid/minute-consuming CI.
Use the desktop simulator and local containers for cross-platform checks.
