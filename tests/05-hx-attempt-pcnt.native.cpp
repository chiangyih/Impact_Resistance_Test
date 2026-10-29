// Actual 80-byte state/sidecar correlation; no device or credential access.
#include "../05-test/hx_attempt_pcnt.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
static HxPcntUnit unit(uint32_t a,uint32_t b,int count,bool high=false){HxPcntUnit u;u.start_begin=a;u.start_end=a+2;u.stop_begin=b;u.stop_end=b+2;u.read_begin=b+3;u.read_end=b+5;u.api_mask=15;u.attempt_mask=7;u.count=count;u.level_start=high;u.level_end=high;return u;}
static HxDiagRecord raw(uint32_t seq,bool accepted=true,int32_t value=-160000){HxDiagRecord r={};r.seq=seq;r.start_us=seq*10000+100;r.end_us=r.start_us+200;r.raw=value;r.bits24=static_cast<uint32_t>(value)&0xffffff;r.accepted=accepted;r.dout_after25=accepted;r.rx_before=seq-1;r.rx_after=accepted?seq:seq-1;r.err_before=0;r.err_after=accepted?0:1;r.pulses_issued=25;return r;}
static HxAttemptPcnt pair(const HxDiagRecord&r,int sc=50,int dc=9){HxAttemptPcnt p;p.seq=r.seq;p.policy=HXAP_PRESENT|HXAP_SETUP|HXAP_PROOF|HXAP_ELIGIBLE|HXAP_ENCLOSED|HXAP_FINISHED;p.prepare_begin=r.start_us-100;p.finish_end=r.end_us+105;p.unit[0]=hxapPack(unit(r.start_us-90,r.end_us+90,sc));p.unit[1]=hxapPack(unit(r.start_us-70,r.end_us+70,dc));return p;}
int main(){unsigned n=0;
{static_assert(sizeof(HxAttemptPcnt)==80&&sizeof(HxAttemptPcntRing)==20480);auto r=raw(1);auto p=pair(r);assert(hxapUsable(p,r));assert(p.unit[0].flags&256);++n;}
{auto r=raw(1,false,-529408);auto p=pair(r);assert(hxapUsable(p,r)&&!r.accepted&&r.raw==-529408);++n;}
{HxDiagState r;HxAttemptPcntRing s;hxdiagBegin(r,0);for(uint32_t i=1;i<=400;i++){auto x=raw(i);hxapObserveShared(r,s,x,pair(x));}assert(r.count==127&&r.pre_overwritten==273);for(uint16_t i=0;i<r.count;i++){auto j=hxdiagIndex(r,i);assert(s.records[j].seq==r.records[j].seq);}++n;}
{HxDiagState r;HxAttemptPcntRing s;hxdiagBegin(r,0);auto x=raw(1);auto p=pair(x);p.seq=99;hxapObserveShared(r,s,x,p);assert(r.records[0].seq==1&&s.records[0].seq==99&&!hxapUsable(s.records[0],r.records[0]));++n;}
{HxDiagState a,b;b.trigger_mask=1;HxAttemptPcntRing x,y;hxdiagBegin(a,0);hxdiagBegin(b,0);for(uint32_t i=1;i<=600;i++){auto r=raw(i,i!=400,i==200?-80:-160000);auto p=pair(r);hxapObserveShared(a,x,r,p);hxapObserveShared(b,y,r,p);}assert(a.trigger_seq==200&&b.trigger_seq==400&&a.mode==HXD_FROZEN&&b.mode==HXD_FROZEN);for(uint16_t i=0;i<a.count;i++){const auto j=hxdiagIndex(a,i);const auto &r=a.records[j];for(uint16_t k=0;k<b.count;k++){const auto z=hxdiagIndex(b,k);if(r.seq==b.records[z].seq)assert(!std::memcmp(&x.records[j],&y.records[z],sizeof(HxAttemptPcnt)));}}++n;}
{HxDiagState r;HxAttemptPcntRing s;hxdiagBegin(r,0);for(uint32_t i=1;i<=329;i++){auto x=raw(i,i!=201);hxapObserveShared(r,s,x,pair(x));}assert(r.mode==HXD_FROZEN);const auto copy=s;for(uint32_t i=330;i<400;i++){auto x=raw(i);hxapObserveShared(r,s,x,pair(x));}assert(!std::memcmp(&copy,&s,sizeof(s)));++n;}
{HxDiagState r;HxAttemptPcntRing s;hxdiagBegin(r,0);auto x=raw(1);x.start_us=600000000;x.end_us=600000200;hxapObserveShared(r,s,x,pair(x));assert(!r.count&&r.omitted_after_freeze==1&&s.records[0].seq==0);++n;}
{HxDiagState r;HxAttemptPcntRing s;hxdiagBegin(r,0);auto x=raw(1);x.start_us=599999900;x.end_us=600000100;hxapObserveShared(r,s,x,pair(x));assert(r.mode==HXD_FROZEN&&r.count==1&&s.records[0].seq==1);++n;}
{auto r=raw(1);auto p=pair(r,0,0);assert(hxapUsable(p,r)&&p.unit[0].count==0);p.unit[0]=HxAttemptPcntUnit();assert(!hxapUsable(p,r));++n;}
{auto r=raw(1);auto p=pair(r);p.unit[0].count=20000;p.unit[0].flags&=~256U;assert(!hxapUsable(p,r));p=pair(r);p.unit[1].flags|=128;assert(!hxapUnitValid(p.unit[1]));++n;}
{auto r=raw(1);auto p=pair(r);p.unit[0].stop_end=p.unit[0].stop_begin-1;assert(!hxapUnitValid(p.unit[0]));++n;}
{auto r=raw(1);auto p=pair(r);p.unit[1].start_begin=p.unit[0].start_end-1;assert(!hxapEnclosed(p));p=pair(r);p.unit[1].read_end=p.unit[0].stop_begin+1;assert(!hxapEnclosed(p));++n;}
{auto r=raw(1);auto p=pair(r);p.unit[1].start_end=r.start_us+1;assert(!hxapContainsRead(p,r));p=pair(r);p.prepare_begin=p.unit[0].start_begin+1;assert(!hxapUsable(p,r));p=pair(r);p.finish_end=p.unit[0].read_end-1;assert(!hxapUsable(p,r));++n;}
{auto r=raw(1);auto p=pair(r);p.unit[1].flags|=1024;assert(hxapUsable(p,r));assert(p.unit[1].flags&512);++n;}
{auto r=raw(1);auto p=pair(r);p.policy|=HXAP_GUARD;assert(!hxapUsable(p,r));p=pair(r);p.policy&=~HXAP_PROOF;p.policy|=static_cast<uint32_t>(HXP_NO_PROOF)<<16;assert(!hxapUsable(p,r));++n;}
{auto r=raw(1);auto p=pair(r);std::vector<uint32_t>w;hxapCanonical(p,[&](uint32_t x){w.push_back(x);});assert(w.size()==20);uint32_t c=0xffffffff;for(auto v:w)c=hxdiagCrcWord(c,v);assert(hxapCrc(p)==(c^0xffffffff));auto old=hxapCrc(p);p.unit[0].count++;assert(hxapCrc(p)!=old);++n;}
std::printf("%u native per-attempt PCNT cases passed; record=%zu bytes, two rings=%zu bytes. Not device validation.\n",n,sizeof(HxAttemptPcnt),2*sizeof(HxAttemptPcntRing));}
