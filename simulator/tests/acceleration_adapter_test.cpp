#include "platform/waveshare_native_board.h"
#include <cmath>
#include <cstdio>
void QMI8658_read_acc_xyz(float acc[3]){acc[0]=2.45175f;acc[1]=-4.9035f;acc[2]=9.807f;}
int main(){
 float x,y,z;
 if(!waveshare_native_read_acceleration(&x,&y,&z) || std::fabs(x-0.25f)>0.00001f ||
    std::fabs(y+0.5f)>0.00001f || std::fabs(z-1)>0.00001f){
  std::puts("FAIL Native acceleration-only readings must convert m/s2 to g");return 1;
 }
 if(waveshare_native_read_acceleration(nullptr,&y,&z)){std::puts("FAIL Null output rejected");return 1;}
 std::puts("Native acceleration adapter units passed");
}
