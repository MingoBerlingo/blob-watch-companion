#include "apps/blob_native/steps/step_counter.h"
#include <math.h>
namespace blob_native {
namespace {
void peak(StepCounter &s,uint32_t now){
 const uint32_t interval=now-s.last_peak_ms;
 if(s.streak && interval<280){s.streak=0;s.last_interval_ms=0;s.last_peak_ms=now;return;}
 const uint32_t difference=interval>s.last_interval_ms ? interval-s.last_interval_ms:s.last_interval_ms-interval;
 if(!s.streak || interval>1600 || (s.last_interval_ms && difference>fmaxf(120,s.last_interval_ms*0.4f))){
  s.streak=1;s.last_interval_ms=0;
 }else{
  s.last_interval_ms=interval;
  const uint32_t added=s.streak==2 ? 3:(s.streak>=3 ? 1:0);
  if(s.streak<3) ++s.streak;
  s.total=UINT32_MAX-s.total<added ? UINT32_MAX:s.total+added;
 }
 s.last_peak_ms=now;
}
}
void step_counter_update(StepCounter &s,uint32_t now,float ax,float ay,float az){
 if(!isfinite(ax)||!isfinite(ay)||!isfinite(az)) {s.initialized=false;s.streak=0;return;}
 const float magnitude=sqrtf(ax*ax+ay*ay+az*az);
 if(!isfinite(magnitude)||magnitude<0.2f||magnitude>3){s.initialized=false;s.streak=0;return;}
 if(!s.initialized || now-s.last_sample_ms>250){
  s.last_sample_ms=now;s.baseline=magnitude;s.filtered=0;
  s.initialized=true;s.peak_active=false;s.armed=true;s.streak=0;s.last_interval_ms=0;return;
 }
 const uint32_t elapsed=now-s.last_sample_ms;
 if(!elapsed) return;
 s.last_sample_ms=now;
 const float dt=elapsed/1000.0f;
 s.baseline+=(magnitude-s.baseline)*(1-expf(-dt/0.8f));
 s.filtered+=(magnitude-s.baseline-s.filtered)*(1-expf(-dt/0.05f));
 if(s.armed && !s.peak_active && s.filtered>0.10f){
  s.peak_start_ms=now;s.peak_active=true;s.armed=false;
 }
 if(s.peak_active){
  const uint32_t width=now-s.peak_start_ms;
  if(width>650) s.peak_active=false;
  else if(s.filtered<0.03f){
   if(width>=40) peak(s,s.peak_start_ms);
   s.peak_active=false;
  }
 }
 if(s.filtered<-0.04f) s.armed=true;
}
}
