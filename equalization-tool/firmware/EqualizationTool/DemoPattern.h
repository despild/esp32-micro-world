// Generated from demo-pattern.json by tools/embed_web.py. Display-only; never ADC data.
#pragma once
#include <stdint.h>
namespace eq {
constexpr uint32_t demoPeriodMs = 12000;
constexpr uint16_t demoKnots[][2] = {{0,0},{600,0},{700,8},{800,65},{900,74},{1050,72},{1200,70},{1320,7},{1450,0},{2000,0},{2080,8},{2180,82},{2300,88},{2420,86},{2550,9},{2700,0},{3250,0},{3330,11},{3420,75},{3550,82},{3700,78},{3820,7},{3950,0},{4500,0},{4580,12},{4680,87},{4800,90},{4920,87},{5060,12},{5200,0},{5350,0},{5430,11},{5530,81},{5660,84},{5780,78},{5920,8},{6070,0},{7000,0},{7120,12},{7240,68},{7380,72},{7800,71},{8200,74},{8600,72},{9000,70},{9400,71},{9750,68},{9910,10},{10050,0},{12000,0}};
inline float demoPercentAt(uint32_t now, uint32_t age=0) {
  const uint32_t phase=(now%demoPeriodMs+demoPeriodMs-age%demoPeriodMs)%demoPeriodMs;
  for(unsigned i=1;i<sizeof(demoKnots)/sizeof(demoKnots[0]);++i) {
    if(phase<=demoKnots[i][0]) {
      const auto &a=demoKnots[i-1], &b=demoKnots[i];
      return float(a[1])+(float(b[1])-a[1])*float(phase-a[0])/float(b[0]-a[0]);
    }
  }
  return 0;
}
}
