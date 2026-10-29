// The actual setup transaction and fallback RAW hook, no ESP32 or COM access.
#include "../05-test/hx_attempt_storage.h"
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <algorithm>
struct Allocator {
  unsigned calls = 0, frees = 0, fail_at = 0; size_t remaining = HXAS_MIN_REMAINING;
  std::vector<void*> live;
  void *allocate(size_t n) { assert(n == 20480); ++calls; if (calls == fail_at) return nullptr; void *p = std::malloc(n); assert(p); live.push_back(p); return p; }
  void release(void *p) { assert(p); auto i = std::find(live.begin(),live.end(),p); assert(i != live.end()); live.erase(i); ++frees; std::free(p); }
  bool begin(HxAttemptStorage &s) { return hxasAllocate(s,[&](size_t n){return allocate(n);},[&](void*p){release(p);},[&](){return remaining;}); }
  ~Allocator() { for (void *p:live) std::free(p); }
};
static HxDiagRecord record(unsigned i) { HxDiagRecord r={}; r.seq=i; r.start_us=i*1000; r.end_us=r.start_us+200; r.accepted=i!=200; r.dout_after25=r.accepted; r.pulses_issued=25; r.raw=-160000; r.bits24=0xfd8f00; return r; }
int main() { unsigned n=0;
  { HxAttemptStorage s; Allocator a; a.fail_at=1; assert(!a.begin(s)&&s.state==HXAS_FIRST_FAILED&&s.attempts==1&&!s.ready()&&!s.any&&!s.link&&a.calls==1&&a.frees==0); a.fail_at=0; assert(!a.begin(s)&&a.calls==1&&s.attempts==1); ++n; }
  { HxAttemptStorage s; Allocator a; a.fail_at=2; assert(!a.begin(s)&&s.state==HXAS_SECOND_FAILED&&s.attempts==1&&!s.any&&!s.link&&a.calls==2&&a.frees==1&&a.live.empty()); assert(!a.begin(s)&&a.calls==2&&a.frees==1); ++n; }
  { HxAttemptStorage s; Allocator a; a.remaining=HXAS_MIN_REMAINING-1; assert(!a.begin(s)&&s.state==HXAS_RESERVE_FAILED&&!s.any&&!s.link&&a.calls==2&&a.frees==2&&a.live.empty()); a.remaining=999999; assert(!a.begin(s)&&a.calls==2&&s.attempts==1); ++n; }
  { HxAttemptStorage s; Allocator a; assert(a.begin(s)&&s.ready()&&s.any!=s.link&&s.attempts==1&&a.calls==2&&a.frees==0); for(unsigned i=0;i<256;++i)assert(s.any->records[i].seq==0&&s.link->records[i].policy==0); auto *p=s.any; assert(a.begin(s)&&s.any==p&&a.calls==2); ++n; }
  { HxAttemptStorage s; Allocator a; a.fail_at=2; a.begin(s); HxDiagState raw; hxdiagBegin(raw,0); for(unsigned i=1;i<=328;++i){auto r=record(i);HxAttemptPcnt p;p.seq=i;hxasObserve(raw,s.any,r,p);} assert(raw.mode==HXD_FROZEN&&raw.count==256&&raw.trigger_seq==200&&raw.pre_count==127&&raw.post_count==128); for(uint16_t i=0;i<raw.count;++i)assert(raw.records[hxdiagIndex(raw,i)].seq==73U+i); assert(a.calls==2&&a.frees==1); ++n; }
  { HxAttemptStorage s; Allocator a; assert(a.begin(s)); HxDiagState x,y; y.trigger_mask=HXD_LINK; hxdiagBegin(x,0);hxdiagBegin(y,0);for(unsigned i=1;i<=400;++i){auto r=record(i);HxAttemptPcnt p;p.seq=i;hxasObserve(x,s.any,r,p);hxasObserve(y,s.link,r,p);}for(uint16_t i=0;i<x.count;++i){auto j=hxdiagIndex(x,i);assert(x.records[j].seq==s.any->records[j].seq);}for(uint16_t i=0;i<y.count;++i){auto j=hxdiagIndex(y,i);assert(y.records[j].seq==s.link->records[j].seq);} assert(a.calls==2&&a.frees==0); ++n; }
  std::printf("%u native atomic sidecar allocation / RAW fallback cases passed; heap_caps / WiFi / device still require QC.\n",n);
}
