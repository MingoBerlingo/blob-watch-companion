# Motion Response Implementation Plan

> Use superpowers:executing-plans inline; keep the work local and validate before flashing.

**Goal:** Make quick tilts and sideways movement immediately affect Blob velocity, without increasing steady gravity.

**Architecture:** Expose full accelerometer (g) and gyro (degrees/second) samples in the board adapter. A small independent complementary gravity estimator predicts orientation from gyro and slowly corrects it from acceleration. Separate orientation changes and transient acceleration from steady gravity; convert movement to bounded center-of-mass velocity impulses, retaining soft-body deformation. Seed the first sample without a kick and reset on page entry. Default gravity returns to650.

**Tech stack:** C++17, existing QMI8658 driver, LVGL/SDL, CTest/PlatformIO.

- [x] Write regressions for static tilt/no repeated boost, no startup kick, quick tilt, sideways pulse with zero gyro, noise rejection, frame subdivision, invalid inputs and bounded physics impulses.
- [x] Add full IMU sampling through src/platform and simulator counterparts, without changing public driver symbols.
- [x] Add standalone motion_response module and soft_body_impulse; integrate with Blobpage and retain normal gravity.
- [x] Inspect updated local screenshots; run Debug/Release tests and firmware build. Review changes.
- [x] Flash the verified version for the user's trial, commit locally without pushing or CI.

## Validation

Both Debug/Release CTest configurations pass all six suites. The native RP2040
build and verified flash succeed. Review found no blockers; physical response
and gyro bias remain for the user's hardware trial. The complementary estimate
can absorb sustained translation and caps elapsed time at64ms. No push or CI.

## Follow-up: separate force roles

User steering: gyro should supply the weaker gravity direction, accelerometer
should supply stronger other forces; add more star spikes. Removed all direct
gyro-to-velocity impulses. Gravity is300px/s² and accelerometer response is9000
px/s²/g. Drift correction slows to2.5s and only runs while rotation is quiet and
acceleration norm is near1g. Star now has eight tips and eight valleys (16points,
480bytes). New regressions verify no gyro kick, stronger translation, frame-rate
consistency of each role, reduced gravity and eight distinct tips. Debug/Release
all six suites pass; default firmware builds successfully.
