// Actual portable PCNT snapshot/state. No COM, GPIO, network or credentials.
#include "../05-test/hx_pcnt_diag.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
static HxPcntUnit unit(uint32_t start, uint32_t stop, int count, bool data = false) { HxPcntUnit u; u.start_begin=start;u.start_end=start+1;u.level_start_begin=start+2;u.level_start_end=start+3;u.level_start=0;u.level_end_begin=stop-3;u.level_end_end=stop-2;u.level_end=data?1:0;u.stop_begin=stop;u.stop_end=stop+1;u.read_begin=stop+2;u.read_end=stop+3;u.api_mask=15;u.attempt_mask=7;u.count=count;return u; }
static HxPcntDiag state(){HxPcntDiag s;s.gpio_function=2;s.gpio_signal=256;s.setup_code=1;s.stage_mask=15;for(unsigned i=0;i<4;i++){s.pins[i][0]={2,256,2,i?28U:24U};s.pins[i][1]={2,256,2,5};}s.pins[1][0].flags=29;return s;}
static HxPcntProof proof(unsigned seq=1){HxPcntProof p;p.seq=seq;p.bits24=0xfd818c;p.accepted=p.postcheck=1;p.unit[0]=unit(seq*10000,seq*10000+300,50);p.unit[1]=unit(seq*10000+10,seq*10000+290,hxpExpectedData(p.bits24,true),true);return p;}
static void ready(HxPcntDiag&s){for(unsigned i=1;i<=4;i++)hxpAddProof(s,proof(i));assert(s.proof_ready);}
int main(){unsigned n=0;
{auto s=state();assert(hxpBaseline(s)&&hxpPinsMatch(s,2));s.pins[2][0].signal=1;assert(!hxpPinsMatch(s,2));s=state();s.pins[2][1].flags|=8;assert(!hxpPinsMatch(s,2));++n;}
{auto s=state();s.pins[2][0].flags^=16;assert(!hxpPinsMatch(s,2));s=state();for(auto &stage:s.pins)stage[0].flags&=~16U;assert(hxpBaseline(s)&&hxpPinsMatch(s,2));++n;}
{auto s=state();s.pins[0][0].flags|=1;assert(!hxpBaseline(s));s=state();s.pins[0][1].flags=4;assert(!hxpBaseline(s));++n;}
{assert(hxpExpectedData(0xffffff,true)==1);assert(hxpExpectedData(0,true)==1);assert(hxpExpectedData(0xaaaaaa,true)==25);++n;}
{auto s=state();ready(s);assert(s.proof_count==4&&s.proof_good==4&&!s.proof_bad);hxpAddProof(s,proof(5));assert(s.proof_count==4);++n;}
{auto s=state();auto p=proof();p.unit[1].count++;hxpAddProof(s,p);for(unsigned i=2;i<=10;i++)hxpAddProof(s,proof(i));assert(s.proof_count==8&&s.proof_good==7&&s.proof_bad==1&&!s.proof_ready);++n;}
{auto s=state();assert(!hxpLink(s,9,90000,true,90001)&&s.mode==HXP_SKIPPED&&s.reason==HXP_NO_PROOF);auto copy=s;assert(!hxpLink(s,10,100000,true,100001)&&!std::memcmp(&copy,&s,sizeof(s)));++n;}
{auto s=state();ready(s);assert(hxpLink(s,9,90000,true,90001));s.unit[0]=unit(90010,364000,0);s.unit[1]=unit(90020,363990,2,true);s.event_end=90030;s.finish_begin=363980;hxpComplete(s,364010);assert(s.mode==HXP_COMPLETE&&!s.reason&&s.unit[0].valid&&s.unit[1].valid);hxpSeal(s);auto copy=s;hxpComplete(s,400000);hxpAddProof(s,proof());assert(!std::memcmp(&copy,&s,sizeof(s)));++n;}
{auto s=state();ready(s);assert(!hxpLink(s,9,90000,false,90001)&&s.reason==HXP_BUSY);hxpSeal(s);assert(s.sealed);++n;}
{auto s=state();hxpSeal(s);assert(s.mode==HXP_NO_LINK&&!s.linked&&s.sealed);assert(!hxpLink(s,1,1,true,2));++n;}
{auto s=state();ready(s);s.pins[3][0].drive++;assert(!hxpLink(s,9,90000,true,90001)&&s.reason==HXP_GPIO);++n;}
{auto u=unit(100,1000,0);assert(hxpUnitValid(u));u.count=16383;assert(hxpUnitValid(u));u.count=16384;assert(!hxpUnitValid(u));u.count=32767;assert(!hxpUnitValid(u));u.count=-1;assert(!hxpUnitValid(u));++n;}
{auto driver=[](int hw,bool pending){return hw+(pending&&hw<=32767/2?32767:0);};auto u=unit(100,1000,driver(20000,true));assert(!hxpUnitValid(u));u.count=driver(0,true);assert(!hxpUnitValid(u));u.count=driver(10000,true);assert(!hxpUnitValid(u));++n;}
{auto s=state();ready(s);hxpLink(s,9,90000,true,90001);s.unit[0]=unit(90010,100000,0);s.unit[1]=unit(90020,99990,20000,true);hxpComplete(s,100010);assert(s.reason==HXP_RANGE&&!s.unit[0].valid&&!s.unit[1].valid);++n;}
{auto u=unit(100,1000,0);u.overflow=1;assert(!hxpUnitValid(u));u=unit(100,1000,0);u.api_mask&=~4U;assert(!hxpUnitValid(u));u=unit(100,1000100,0);assert(!hxpUnitValid(u));++n;}
{auto s=state();ready(s);hxpLink(s,9,90000,true,90001);s.unit[0]=unit(90020,100000,0);s.unit[1]=unit(90010,99990,0,true);hxpComplete(s,100010);assert(s.reason==HXP_TIME&&!s.unit[0].valid&&!s.unit[1].valid);++n;}
{auto s=state();ready(s);hxpLink(s,9,0xffffff00U,true,0xffffff01U);s.unit[0]=unit(0xffffff10U,100000,0);s.unit[1]=unit(0xffffff20U,99990,2,true);hxpComplete(s,100010);assert(!s.reason&&s.unit[0].valid&&s.unit[1].valid);++n;}
{auto s=state();std::vector<uint32_t>w;hxpCanonical(s,[&](uint32_t x){w.push_back(x);});assert(w.size()==405);uint32_t c=0xffffffff;for(auto x:w)c=hxdiagCrcWord(c,x);assert(hxpCrc(s)==(c^0xffffffff));auto old=hxpCrc(s);s.pins[0][0].drive++;assert(hxpCrc(s)!=old);++n;}
std::printf("%u native PCNT cases passed; snapshot=%zu bytes; no device/edge validation.\n",n,sizeof(HxPcntDiag));}
