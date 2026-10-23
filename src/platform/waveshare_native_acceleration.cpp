#include "platform/waveshare_native_board.h"
#include "QMI8658.h"

// Independent accelerometer reads leave the Blob's gyro sampling untouched.
bool waveshare_native_read_acceleration(float *x,float *y,float *z){
    if(!x||!y||!z) return false;
    float acc[3];QMI8658_read_acc_xyz(acc);
    *x=acc[0]/ONE_G;*y=acc[1]/ONE_G;*z=acc[2]/ONE_G;return true;
}
