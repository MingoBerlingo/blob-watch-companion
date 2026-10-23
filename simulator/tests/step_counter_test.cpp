#include "apps/blob_native/steps/step_counter.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <initializer_list>
using namespace blob_native;
namespace {
void check(bool ok,const char *message){if(!ok){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
void feed(StepCounter &s,uint32_t &now,int samples,float amplitude,float hz,bool sideways=false){
 for(int i=0;i<samples;++i){now+=20;const float g=1+amplitude*std::sin(6.283185307f*hz*i*0.02f);
  step_counter_update(s,now,sideways ? g:0,0,sideways ? 0:g);}
}
}
int main(){
 StepCounter s{};uint32_t now=0;
 feed(s,now,1000,0.025f,8);check(s.total==0,"Rest and tremor are not steps");
 for(int i=0;i<500;++i){now+=20;const float a=i*0.01f;step_counter_update(s,now,std::sin(a),0,std::cos(a));}
 check(s.total==0,"Changing orientation without walking is not a step");
 s={};now=0;feed(s,now,100,0,1);step_counter_update(s,now+=20,0,0,2.5f);feed(s,now,150,0,1);
 check(s.total==0,"An isolated impact is not a walking bout");
 for(float hz:{1.0f,1.8f,3.0f}){
  s={};now=0;feed(s,now,100,0,1);feed(s,now,1000,0.25f,hz);feed(s,now,100,0,1);
  const int expected=static_cast<int>(20*hz);
  std::printf("cadence %.1f Hz: %u steps (about %d expected)\n",hz,s.total,expected);
  check(s.total>=expected-2 && s.total<=expected+1,"Walking cadence counts approximately one step per cycle");
 }
 s={};now=0;feed(s,now,100,0,1);feed(s,now,1000,0.3f,5);
 check(s.total==0,"Fast repetitive shaking does not alias into walking cadence");
 StepCounter upright{},sideways{};uint32_t t1=0,t2=0;
 feed(upright,t1,100,0,1);feed(sideways,t2,100,0,1,true);
 feed(upright,t1,1000,0.25f,2);feed(sideways,t2,1000,0.25f,2,true);
 check(upright.total==sideways.total && upright.total>30,"Counting is independent of watch orientation");
 const auto count=upright.total;
 step_counter_update(upright,t1+=20,NAN,0,1);step_counter_update(upright,t1+=20,0,0,INFINITY);
 feed(upright,t1,100,0,1);check(upright.total==count,"Invalid samples do not add steps");
 s={};now=0xfffff000u;feed(s,now,100,0,1);feed(s,now,1000,0.25f,2);
 check(s.total>30,"Millis wrap does not interrupt walking detection");
 s={};s.total=UINT32_MAX-1;now=0;feed(s,now,100,0,1);feed(s,now,1000,0.25f,2);
 check(s.total==UINT32_MAX,"Total saturates without overflow");
 std::puts("Step detector regressions passed");
}
