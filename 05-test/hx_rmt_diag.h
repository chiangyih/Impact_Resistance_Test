#ifndef IRT_HX_RMT_DIAG_H
#define IRT_HX_RMT_DIAG_H
#include "hx_attempt_pcnt.h"
#include <string.h>
const uint32_t HXR_SYMBOLS=64, HXR_EVENTS=3, HXR_RESOLUTION=10000000, HXR_IDLE_NS=1000000, HXR_TIMEOUT_US=5000;
enum HxrReason : uint32_t { HXR_API=1, HXR_TIMEOUT=2, HXR_CAPACITY=4, HXR_CALLBACK=8, HXR_OVERLAP=16, HXR_IDENTITY=32, HXR_NOT_ARMED=64, HXR_GPIO=128, HXR_LATE=256, HXR_SYMBOL=512, HXR_NOT_BOUND=1024 };
struct HxrUnit { uint32_t request_begin=0,request_end=0,callback_us=0,count=0,callback_flags=0; uint32_t symbols[HXR_SYMBOLS]={}; };
struct HxrTrace { HxDiagRecord raw={}; HxAttemptPcnt pcnt={}; uint32_t seq=0,token=0,reason=0,arm_mask=0,done_mask=0,read_bound=0,request_events=0,retained=0,prepare_begin=0,prepare_end=0,hook_begin=0,hook_end=0,safe_copy_mask=0; HxrUnit unit[2]={}; };
struct HxrCallback { uint32_t *buffer=nullptr; uint32_t token=0,count=0,at_us=0,flags=0,calls=0,done=0; };
struct HxrState {
  HxrTrace work[2]={},event[HXR_EVENTS]={}; HxrCallback cb[2]={}; HxPcntPin pins[5][2]={};
  uint32_t setup=0,setup_attempts=0,stage_mask=0,terminal=0,pending=0,current=0,next_token=0,attempts=0,skipped=0,last_seq=0,event_selected[HXR_EVENTS]={};
  uint32_t allocation_before=0,allocation_after=0,allocated_bytes=0,cleanup_mask=0,resolution[2]={},channel_id[2]={};
};
static_assert(sizeof(HxrState)<=4096,"RMT state must fit one bounded setup heap allocation");
inline bool hxrDataDifference(const HxDiagRecord&r,const HxAttemptPcnt&p) {
  const uint32_t f=p.unit[1].flags; return hxapUsable(p,r)&&(f&512)&&(f&2048)&&!(f&1024)&&((f>>12)&1)==r.dout_after25&&p.unit[1].count!=static_cast<int32_t>(hxpExpectedData(r.bits24,r.dout_after25));
}
inline uint32_t hxrSymbolReason(const HxrUnit&u) {
  if(!u.count||u.count>=HXR_SYMBOLS)return HXR_CAPACITY;
  bool ended=false,seen=false;uint32_t prior=0;
  for(uint32_t i=0;i<u.count;i++)for(uint8_t half=0;half<2;half++){
    const uint32_t v=u.symbols[i]>>(16*half),duration=v&32767,level=(v>>15)&1;
    if(!duration){ended=true;continue;}if(ended||(seen&&level==prior))return HXR_SYMBOL;seen=true;prior=level;
  }
  return seen?0:HXR_SYMBOL;
}
inline uint32_t hxrSckReason(const HxrUnit&u) {
  const uint32_t basic=hxrSymbolReason(u);if(basic)return basic;
  uint32_t highs=0,runs=0,first=0,last=0;
  for(uint32_t i=0;i<u.count;i++)for(uint8_t half=0;half<2;half++){const uint32_t v=u.symbols[i]>>(16*half);if(!(v&32767))continue;const uint32_t level=(v>>15)&1;if(!runs)first=level;last=level;++runs;highs+=level;}
  (void)first;(void)last;return highs==25&&runs>=49&&runs<=51?0:HXR_SYMBOL; // ESP32 may omit initial LOW and terminate idle with zero, so endpoint LOW runs are not mandatory.
}
inline int hxrFreshProofRole(const HxPcntDiag&d,const HxDiagRecord&r) {
  if(!d.proof_count||d.proof_count>HXP_PROOFS)return -1;
  const HxPcntProof &proof=d.proof[d.proof_count-1];return proof.seq==r.seq?(proof.valid?2:1):-1; // A source-policy bit alone is never evidence that a new proof was appended.
}
inline bool hxrProofIdentity(const HxPcntProof&proof,const HxDiagRecord&r,const HxAttemptPcnt&p) {
  if(proof.seq!=r.seq||p.seq!=r.seq||proof.bits24!=r.bits24||proof.accepted!=r.accepted||proof.postcheck!=r.dout_after25)return false;
  for(uint8_t i=0;i<2;i++){const HxAttemptPcntUnit packed=hxapPack(proof.unit[i]);if(memcmp(&packed,&p.unit[i],sizeof(packed)))return false;}return true;
}
inline void hxrSelect(HxrState&s,const HxDiagRecord&r,const HxAttemptPcnt&p,int proof_role=-1) {
  const bool wanted[3]={!r.accepted,proof_role==1,proof_role==2};
  HxrTrace *w=s.pending&&s.work[s.current].seq==r.seq?&s.work[s.current]:nullptr;
  if(w){w->raw=r;w->pcnt=p;w->read_bound=1;}
  for(uint8_t e=0;e<3;e++)if(wanted[e]&&!s.event_selected[e]){
    s.event_selected[e]=r.seq;
    if(w)w->request_events|=1U<<e;
    else{s.event[e]=HxrTrace();s.event[e].raw=r;s.event[e].pcnt=p;s.event[e].seq=r.seq;s.event[e].reason=HXR_NOT_ARMED;s.event[e].read_bound=1;s.event[e].retained=1;}
  }
}
inline HxrTrace hxrStableCopy(const HxrTrace&w) {
  HxrTrace t;t.raw=w.raw;t.pcnt=w.pcnt;t.seq=w.seq;t.token=w.token;t.reason=w.reason;t.arm_mask=w.arm_mask;t.done_mask=w.done_mask;t.read_bound=w.read_bound;t.request_events=w.request_events;t.prepare_begin=w.prepare_begin;t.prepare_end=w.prepare_end;t.hook_begin=w.hook_begin;t.hook_end=w.hook_end;t.safe_copy_mask=w.safe_copy_mask;
  for(uint8_t i=0;i<2;i++)if(w.safe_copy_mask&(1U<<i))t.unit[i]=w.unit[i];else{t.unit[i].request_begin=w.unit[i].request_begin;t.unit[i].request_end=w.unit[i].request_end;t.unit[i].callback_us=w.unit[i].callback_us;t.unit[i].count=w.unit[i].count;t.unit[i].callback_flags=w.unit[i].callback_flags;}
  return t; // Pending user buffer may still be written by the driver ISR: never copy it on timeout.
}
inline void hxrRetain(HxrState&s,HxrTrace&w) {
  for(uint8_t e=0;e<3;e++)if(w.request_events&(1U<<e)){s.event[e]=hxrStableCopy(w);s.event[e].retained=1;} // BAD/GOOD lock the original strict proof classification even when RMT data is missing or invalid.
}
template<class Add>inline void hxrTraceWords(const HxrTrace&t,Add add) {
  add(t.seq);add(t.token);add(t.reason);add(t.arm_mask);add(t.done_mask);add(t.read_bound);add(t.request_events);add(t.retained);
  add(t.prepare_begin);add(t.prepare_end);add(t.hook_begin);add(t.hook_end);add(t.safe_copy_mask);
  const HxDiagRecord&r=t.raw;const uint32_t raw[]={r.seq,r.start_us,r.end_us,r.attempt_gap_us,r.last_accept_ms_before,r.bits24,static_cast<uint32_t>(r.raw),r.rx_before,r.rx_after,r.err_before,r.err_after,r.flags,r.cal_count_after,r.phase_after,r.cal_mode_after,r.accepted,r.dout_after25,r.pulses_issued,r.context_flags};for(auto v:raw)add(v);
  hxapCanonical(t.pcnt,add);
  for(const auto&u:t.unit){add(u.request_begin);add(u.request_end);add(u.callback_us);add(u.count);add(u.callback_flags);for(uint32_t v:u.symbols)add(v);}
}
inline uint32_t hxrTraceCrc(const HxrTrace&t){uint32_t c=0xffffffff;hxrTraceWords(t,[&](uint32_t v){c=hxdiagCrcWord(c,v);});return c^0xffffffff;}
template<class Add>inline void hxrMetaWords(const HxrState*s,uint32_t closed,Add add,bool attempted=false){
  const uint32_t m[]={s?s->setup:(attempted?2U:0U),s?s->setup_attempts:(attempted?1U:0U),s?s->stage_mask:0,s?s->terminal:0,s?s->pending:0,s?s->attempts:0,s?s->skipped:0,s?s->last_seq:0,s?s->allocated_bytes:0,s?s->allocation_before:0,s?s->allocation_after:0,s?s->cleanup_mask:0,s?s->resolution[0]:0,s?s->resolution[1]:0,s?s->channel_id[0]:0,s?s->channel_id[1]:0,closed,s?s->event_selected[0]:0,s?s->event_selected[1]:0,s?s->event_selected[2]:0,HXR_RESOLUTION,HXR_IDLE_NS,HXR_TIMEOUT_US,HXR_SYMBOLS};for(auto v:m)add(v);
  for(uint8_t a=0;a<5;a++)for(uint8_t b=0;b<2;b++){const HxPcntPin p=s?s->pins[a][b]:HxPcntPin();add(p.function);add(p.signal);add(p.drive);add(p.flags);}
}
#endif
