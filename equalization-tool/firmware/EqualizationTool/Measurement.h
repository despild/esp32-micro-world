// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <stdint.h>
#include <math.h>
#include "Config.h"

namespace eq {
enum Flag : uint8_t { Missing=1, Saturated=2, Uncalibrated=4, Collecting=8, OutsideRange=16, Gap=32 };
inline int32_t signed24(uint32_t bits) { return (bits & 0x800000) ? int32_t(bits | 0xff000000) : int32_t(bits); }
struct Sample { uint32_t seq, ms; int32_t raw; float pressure; uint8_t flags; };
enum class Job : uint8_t { None, Zero, Reference };
struct Button {
  bool last=false, stable=false, longSent=false;
  uint32_t changed=0, pressed=0;
  // 1 = short release, 2 = long press; long press never produces a short release.
  int update(bool down, uint32_t now) {
    if(down!=last){last=down;changed=now;}
    int event=0;
    if(down!=stable && uint32_t(now-changed)>=30){
      stable=down;
      if(down){pressed=now;longSent=false;}else if(!longSent)event=1;
    }
    if(stable&&!longSent&&uint32_t(now-pressed)>=800){longSent=true;event=2;}
    return event;
  }
};
class Measurement {
public:
  static constexpr unsigned Capacity=512;
  Sample samples[Capacity]{};
  unsigned count=0;
  uint32_t next=1, revision=1, lastGood=0, lastEvent=0;
  bool online=false, zeroValid=false, gainValid=false;
  float zero=0, gain=0, reference=2, zeroNoise=0;
  Job job=Job::None;
  unsigned collected=0;
  const char *result="idle";
  bool saveGain=false;
  int32_t currentRaw=0;
  uint8_t currentFlags=Missing;

  void clear(){count=0;++revision;}
  uint32_t oldest()const{return next-count;}
  const Sample &at(uint32_t seq)const{return samples[(seq-1)%Capacity];}
  bool calibrated()const{return zeroValid&&gainValid;}
  float currentPressure()const{
    return online&&calibrated()&&!(currentFlags&(Missing|Saturated|Collecting|Gap)) ? (currentRaw-zero)*gain : NAN;
  }
  bool start(Job kind,float kPa,uint32_t now){
    if(job!=Job::None||!online||(currentFlags&(Missing|Saturated|Gap)))return false;
    if(kind==Job::Reference&&(!zeroValid||!isfinite(kPa)||kPa<0.2f||kPa>40))return false;
    if(kind==Job::None)return false;
    if(kind==Job::Zero)zeroValid=false;
    job=kind;reference=kPa;collected=0;sum=0;low=INT32_MAX;high=INT32_MIN;started=now;
    result="collecting";clear();return true;
  }
  void cancel(){if(job!=Job::None){job=Job::None;result="cancelled";clear();}}
  void fault(uint32_t now,uint8_t flag=Missing){
    if(online||zeroValid||job!=Job::None)clear();
    online=false;zeroValid=false;currentFlags=flag;
    if(job!=Job::None){job=Job::None;result="sensor_error";}
    push(now,0,NAN,flag);lastEvent=now;
  }
  void tick(uint32_t now){
    if(job!=Job::None&&uint32_t(now-started)>5000){job=Job::None;result="timeout";clear();}
    if(uint32_t(now-lastEvent)>=cfg::sampleTimeoutMs)fault(now);
  }
  void ingest(int32_t raw,uint32_t now,bool gap=false){
    if(job!=Job::None&&uint32_t(now-started)>5000){job=Job::None;result="timeout";clear();}
    lastGood=lastEvent=now;online=true;currentRaw=raw;
    uint8_t flags=0;
    if(raw<=-8388608||raw>=8388607)flags|=Saturated;
    if(gap)flags|=Gap;
    if(flags&&(job!=Job::None)){job=Job::None;result="sensor_error";clear();}
    if(job!=Job::None){
      flags|=Collecting;sum+=raw;if(raw<low)low=raw;if(raw>high)high=raw;++collected;
      if(collected>=cfg::calibrationSamples){
        const float mean=float(double(sum)/collected),spread=float(int64_t(high)-low);
        if(job==Job::Zero){
          const float allowed=gainValid?0.15f/fabsf(gain):50000.f;
          if(spread<=allowed){zero=mean;zeroNoise=spread;zeroValid=true;result="zero_ok";}else result="unstable";
        }else{
          const float delta=mean-zero;
          if(fabsf(delta)<1000)result="reference_too_small";
          else if(spread>fabsf(delta)*0.05f)result="unstable";
          else {gain=reference/delta;gainValid=true;saveGain=true;result="calibration_ok";}
        }
        job=Job::None;clear();
      }
    }
    if(!calibrated())flags|=Uncalibrated;
    float p=NAN;
    if(calibrated()&&!(flags&(Saturated|Collecting|Gap))){p=(raw-zero)*gain;if(p<0||p>40)flags|=OutsideRange;}
    currentFlags=flags;push(now,raw,p,flags);
  }
private:
  int64_t sum=0;
  int32_t low=0,high=0;
  uint32_t started=0;
  void push(uint32_t now,int32_t raw,float p,uint8_t flags){
    if(next==UINT32_MAX){next=1;count=0;++revision;}
    samples[(next-1)%Capacity]={next,now,raw,p,flags};++next;if(count<Capacity)++count;
  }
};
}
