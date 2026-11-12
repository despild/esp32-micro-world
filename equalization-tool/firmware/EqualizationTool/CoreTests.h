// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <new>
#include <string.h>
#include "Measurement.h"
// USB-only deterministic tests on a separate object. Never feeds the live graph/ADC.
// Return a failure bitmask (0 = all 20 assertions passed).
static uint32_t runCoreTests(){
  uint32_t failures=0;unsigned index=0;
  auto check=[&](bool pass){if(!pass)failures|=uint32_t(1)<<index;++index;};
  check(eq::signed24(0x7fffff)==8388607);
  check(eq::signed24(0x800000)==-8388608);
  check(eq::signed24(0xffffff)==-1);
  auto *m=new(std::nothrow) eq::Measurement;
  if(!m)return 0x80000000;
  check(!m->start(eq::Job::Zero,0,0)&&isnan(m->currentPressure()));
  m->ingest(500000,1000);
  check(m->online&&!m->calibrated()&&isnan(m->currentPressure()));
  check(!m->start(eq::Job::Reference,2,1000));
  check(m->start(eq::Job::Zero,0,1000));
  for(unsigned i=1;i<=80;i++)m->ingest(500000,1000+i*25);
  check(m->zeroValid&&m->zero==500000&&!m->gainValid);
  m->ingest(500000,3025);
  check(!m->start(eq::Job::Reference,NAN,3025)&&!m->start(eq::Job::Reference,41,3025));
  check(m->start(eq::Job::Reference,2,3025));
  for(unsigned i=1;i<=80;i++)m->ingest(600000,3025+i*25);
  m->ingest(550000,5050);
  check(m->calibrated()&&fabsf(m->currentPressure()-1)<1e-5f&&m->saveGain);
  m->ingest(8388607,5075);
  check((m->currentFlags&eq::Saturated)&&isnan(m->currentPressure()));
  m->ingest(499000,5100);
  check((m->currentFlags&eq::OutsideRange)&&m->currentPressure()<0);
  m->tick(5700);
  check(!m->online&&!m->zeroValid&&m->gainValid&&isnan(m->currentPressure()));
  m->ingest(500000,5725);m->start(eq::Job::Zero,0,5725);m->ingest(500000,11000);
  check(m->job==eq::Job::None&&!strcmp(m->result,"timeout"));
  for(unsigned i=0;i<600;i++)m->ingest(500000,12000+i*25);
  check(m->count==512&&m->at(m->oldest()).seq==m->oldest()&&m->at(m->next-1).raw==500000);
  m->next=UINT32_MAX;m->ingest(1,28000);
  check(m->next==2&&m->count==1&&m->at(1).seq==1);
  delete m;
  eq::Button b;
  check(b.update(true,0)==0&&b.update(false,10)==0&&b.update(true,20)==0&&b.update(true,50)==0&&b.update(false,100)==0&&b.update(false,130)==1);
  check(b.update(true,200)==0&&b.update(true,230)==0&&b.update(true,1030)==2&&b.update(false,1100)==0&&b.update(false,1130)==0);
  eq::Button wrap;wrap.update(true,0xfffffff0);wrap.update(true,15);
  check(wrap.update(true,815)==2);
  return failures;
}
