#pragma once
// The vendor acceleration-only API returns m/s²; its full-motion API returns mg.
#define ONE_G (9.807f)
void QMI8658_read_acc_xyz(float acc[3]);
