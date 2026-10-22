# Position Follow and Rotating Face

User approved restoring the original accelerometer-to-position behavior while
retaining soft-body deformation, and requested the face rotate with the body.

Implemented and verified locally:
- Original axis/target mapping via read_tilt, bypassing the gyro gravity estimator.
- Fixed-step PD approximation of the old .75/.15 controller, bounded control force,
  center speed and circular target range; flat input recenters the body.
- Best-fit particle rotation drives eye/mouth bases and blinking eyelid direction.
  Body rotation comes from physical contacts, not a forced device-yaw spin.
- Tests for prompt response, settling/recentering, frame partition invariance,
  sustained rim interactions and quarter-turn body/face geometry.
- Debug/Release all six suites pass; native firmware builds; visual references
  inspected. Reviewer found no blockers. Device upload authorized by this session.

Changes remain local; no push or CI. The position response approximates the old
controller because fixed substeps, air drag and bounds differ from the original.
