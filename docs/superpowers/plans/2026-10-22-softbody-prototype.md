# 2D Soft-body Prototype Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan inline, task-by-task.

**Goal:** Build and test a local, unflashed soft-body Blob prototype driven by tilt and colliding with the circular display boundary.

**Architecture:** A standalone fixed-storage physics module owns 24 particle positions/velocities and rest offsets. Symplectic Euler integration uses fixed 8 ms steps, capped catch-up, neighbour springs, rotational shape-matching forces and damping. Circular boundary collision projects particles inward, reflects only outward velocity and damps tangential movement. The existing 48-point outline interpolates the particles; the face follows their centroid upright. Preserve the existing procedural implementation behind a config flag.

**Tech Stack:** C++17 firmware/SDL simulator, LVGL 8.3.11, PlatformIO, local CTest.

## Task 1: Standalone physics with regression tests
- [x] Write tests for rest equilibrium, translation, rotation invariance, local deformation/recovery, finite bounded particles under long stress, circle collision, timestep partition independence and capped stalls.
- [x] Confirm tests fail before adding the module.
- [x] Implement soft_body.h/.cpp with no heap, no LVGL or board dependencies; initialize a circular rest shape once, with no animated sine-wave deformation (user steering).
- [x] Benchmark repeated fixed updates, report storage and desktop costs honestly; no board-FPS claims.

## Task 2: Preview integration
- [x] Select prototype by BLOB_SOFT_BODY_ENABLED; keep radius 50, 2 px outline, no fill.
- [x] Drive acceleration from the existing IMU adapter and track actual elapsed milliseconds, resetting the clock when entering the page.
- [x] Keep facial details inside the body, use its centroid and average velocity for existing face animation.
- [x] Add deterministic screenshots of rim compression and post-release recovery; inspect before updating visual baselines.
- [x] Verify timer/brightness/navigation tests still pass; keep touch reserved for existing navigation (no dragging in this prototype).

## Task 3: Verify and deliver locally
- [x] Run Debug/Release CTest, firmware default and optional performance builds.
- [x] Review the implementation, document source concepts and prototype limitations, open the local simulator for inspection.
- [x] Commit only on the local prototype branch; no push, CI or board flashing.

Reference concepts: https://lisyarus.github.io/blog/posts/soft-body-physics.html and https://www.lexaloffle.com/bbs/?tid=141367 . Implement independently in the repo's C++ style; no copied cartridge code/assets.

## Results

- Debug and Release: all five CTest suites pass; 53 shared-page interaction checks pass.
- Physics regression includes four simulated minutes of stress and periodic finite/boundary/area/edge-crossing checks.
- Default and diagnostic RP2040 builds pass; no device upload, push or CI was performed.
- Model storage: 704 bytes. Release benchmark: approximately 20–25 ms per 100,000 fixed steps on this Mac, not device timing.
- Generated and inspected rest/contact/rebound/settling screenshots and a six-second GIF; interactive SDL preview opened.
- Reviewer found no blockers; limitations are documented in simulator/README.md.
