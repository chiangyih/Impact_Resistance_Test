// Compile actual public adapter with deterministic API failures/late events; no hardware.
#include "hx-pcnt-stubs/mock.h"
#include "../05-test/hx_pcnt_adapter.h"
#include <cassert>
#include <cstdio>
#include <cstring>
static void reset(){for(auto u:fakeUnits)delete u;fakeUnits.clear();fakeCalls.clear();fakeFail.clear();fakeFailUnit=-1;fakeCorruptReady=false;fakeNow=1000;for(auto&p:fakePins)p= {};for(auto&v:fakeLevels)v=0;fakePins[25].oe=true;fakePins[25].oe_ctrl_by_periph=true;fakePins[33].ie=true;fakePins[33].pu=true;hx_pcnt=HxPcntDiag();hx_pcnt_rt=HxPcntRuntime();hx_pcnt_ever_overflow=0;hx_pcnt_irq_closed=false;}
static HxDiagState window(){HxDiagState s;hxdiagBegin(s,0);return s;}
static void proof(unsigned seq=1,bool accepted=true){fakeLevels[33]=0;prepareHxPcntProof();assert(hx_pcnt_rt.proof_active);fakeNow+=200;fakeUnits[0]->count=50;fakeUnits[1]->count=hxpExpectedData(0xfd818c,accepted);fakeLevels[33]=accepted?1:0;finishHxPcntProof(seq,0xfd818c,accepted);assert(!fakeUnits[0]->running&&!fakeUnits[1]->running);}
static void ready(){for(unsigned i=1;i<=4;i++)proof(i);assert(hx_pcnt.proof_ready);}
int main(){unsigned n=0;
{reset();initializeHxPcnt(25,33);assert(hx_pcnt.setup_code==1&&!hx_pcnt.clock_blocked&&fakePins[25].oe&&fakePins[25].ie&&!fakePins[25].pu&&fakePins[33].pu&&!fakePins[33].oe);++n;}
{reset();fakePins[25].sig_out=7;initializeHxPcnt(25,33);assert(hx_pcnt.setup_code!=1&&hx_pcnt.clock_blocked&&fakeUnits.empty());++n;}
{reset();fakeFail="allocate";fakeFailUnit=1;initializeHxPcnt(25,33);assert(hx_pcnt.setup_code!=1&&!hx_pcnt.clock_blocked&&!fakePins[25].pu&&fakePins[33].pu);++n;}
{reset();fakeCorruptReady=true;initializeHxPcnt(25,33);assert(hx_pcnt.clock_blocked&&hx_pcnt.setup_code!=1&&fakePins[25].sig_out==7);++n;}
{reset();initializeHxPcnt(25,33);ready();auto count=hx_pcnt.proof_count;prepareHxPcntProof();assert(!hx_pcnt_rt.proof_active&&hx_pcnt.proof_count==count);++n;}
{reset();initializeHxPcnt(25,33);proof(1,false);assert(hx_pcnt.proof_bad&&!hx_pcnt.proof_ready);auto w=window();startHxPcntHold(2,fakeNow-1,true,w);assert(hx_pcnt.mode==HXP_SKIPPED&&hx_pcnt.reason==HXP_NO_PROOF&&!fakeUnits[0]->running&&!fakeUnits[1]->running);++n;}
{reset();initializeHxPcnt(25,33);ready();fakeCalls.clear();fakeLevels[33]=0;auto w=window();const auto anchor=fakeNow-1;startHxPcntHold(10,anchor,true,w);assert(hx_pcnt.mode==HXP_RUNNING);fakeNow+=273000;fakeUnits[0]->count=0;fakeUnits[1]->count=2;finishHxPcntHold(false);assert(hx_pcnt.mode==HXP_COMPLETE&&!hx_pcnt.reason&&hxpEnclosed(hx_pcnt.unit));const std::vector<std::string> want={"C0","S0","C1","S1","T1","R1","T0","R0"};assert(fakeCalls==want&&!fakeUnits[0]->running&&!fakeUnits[1]->running);sealHxPcnt(true,false);const auto copy=hx_pcnt;pcnt_watch_event_data_t event;hxPcntOverflow(fakeUnits[1],&event,&hx_pcnt_index[1]);assert(!std::memcmp(&copy,&hx_pcnt,sizeof(copy)));++n;}
{reset();initializeHxPcnt(25,33);ready();auto w=window();fakeLevels[33]=0;startHxPcntHold(10,fakeNow-1,true,w);fakeNow+=250000;fakeUnits[1]->count=0;fakeUnits[1]->pending=true;finishHxPcntHold(false);assert(hx_pcnt.reason==HXP_OVERFLOW&&!hx_pcnt.unit[1].valid&&hx_pcnt.unit[1].count==32767);++n;}
{reset();initializeHxPcnt(25,33);ready();auto w=window();fakeLevels[33]=0;startHxPcntHold(10,fakeNow-1,true,w);fakeNow+=250000;fakeUnits[1]->count=20000;fakeUnits[1]->pending=true;finishHxPcntHold(false);assert(hx_pcnt.reason==HXP_RANGE&&!hx_pcnt.unit[1].valid&&!hx_pcnt.unit[1].overflow);++n;}
{reset();initializeHxPcnt(25,33);ready();pcnt_watch_event_data_t event;hxPcntOverflow(fakeUnits[1],&event,&hx_pcnt_index[1]);auto w=window();startHxPcntHold(10,fakeNow-1,true,w);fakeNow+=250000;finishHxPcntHold(false);assert(hx_pcnt.reason==HXP_OVERFLOW&&!hx_pcnt.unit[1].valid);++n;}
{reset();initializeHxPcnt(25,33);ready();auto w=window();startHxPcntHold(10,fakeNow-1,true,w);fakeNow+=250000;fakeFail="stop";fakeFailUnit=1;finishHxPcntHold(false);assert(hx_pcnt.clock_blocked&&!hx_pcnt.unit[1].valid);++n;}
{reset();initializeHxPcnt(25,33);ready();auto w=window();fakeNow=600000001;startHxPcntHold(10,599999999,true,w);assert(hx_pcnt.mode==HXP_SKIPPED&&hx_pcnt.reason==HXP_TIME&&!fakeUnits[0]->running&&!fakeUnits[1]->running);++n;}
{reset();initializeHxPcnt(25,33);fakeFail="clear";fakeFailUnit=0;prepareHxPcntProof();assert(!hx_pcnt_rt.pending.unit[0].attempt_mask);finishHxPcntProof(1,0xfd818c,true);assert(hx_pcnt.proof_bad);++n;}
{reset();initializeHxPcnt(25,33);ready();sealHxPcnt(true,false);assert(hx_pcnt.mode==HXP_NO_LINK&&hx_pcnt.sealed);auto copy=hx_pcnt;prepareHxPcntProof();auto w=window();startHxPcntHold(10,fakeNow,true,w);assert(!std::memcmp(&copy,&hx_pcnt,sizeof(copy)));++n;}
{reset();initializeHxPcnt(25,33);ready();fakePins[25].drv++;auto w=window();startHxPcntHold(10,fakeNow,true,w);assert(hx_pcnt.clock_blocked&&hx_pcnt.mode==HXP_SKIPPED&&hx_pcnt.reason==HXP_GPIO);++n;}
reset();std::printf("%u native actual-adapter mock cases passed; not pad/ISR/edge validation.\n",n);}
