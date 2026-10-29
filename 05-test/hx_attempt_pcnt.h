#ifndef IRT_HX_ATTEMPT_PCNT_H // 每讀框的固定sidecar，不改原五十六byte RAW紀錄。
#define IRT_HX_ATTEMPT_PCNT_H // count是MCU pad邊緣總數，不是脈寬或ADC真值。
#include "hx_pcnt_diag.h" // 沿用IDF半限及有界CPU時間契約。
const uint32_t HXAP_PRESENT = 1, HXAP_SETUP = 2, HXAP_PROOF = 4, HXAP_ELIGIBLE = 8, HXAP_ENCLOSED = 16, HXAP_PROOF_SOURCE = 32, HXAP_GUARD = 64, HXAP_FINISHED = 128; // pair policy低八bit。
struct HxAttemptPcntUnit { // 六個API括弧及count/masks，沒有假造GPIO端點時間。
  uint32_t start_begin = 0, start_end = 0, stop_begin = 0, stop_end = 0, read_begin = 0, read_end = 0; int32_t count = 0; uint32_t flags = 0; // flags=api4/attempt3/overflow/valid/startKnown/startLevel/endKnown/endLevel。
}; // 結束每unit八個canonical word。
struct HxAttemptPcnt { uint32_t seq = 0, policy = 0, prepare_begin = 0, finish_end = 0; HxAttemptPcntUnit unit[2] = {}; }; // 同一seq、含CLEAR前與全部收尾後的CPU時間。
struct HxAttemptPcntRing { HxAttemptPcnt records[HXD_CAPACITY] = {}; }; // 與原RAW相同實體index，由實際append決定是否寫入。
static_assert(sizeof(HxAttemptPcntUnit) == 32 && sizeof(HxAttemptPcnt) == 80, "Attempt PCNT sidecar schema changed"); // 兩槽新增四萬零九百六十byte。
inline bool hxapUnitValid(const HxAttemptPcntUnit &u) { // 不依accepted或期待DATA數目來濾除count。
  return (u.flags & 15) == 15 && ((u.flags >> 4) & 7) == 7 && !(u.flags & 128) && u.count >= 0 && static_cast<uint32_t>(u.count) <= HXP_VALID_MAX && hxdiagDelta(u.stop_end,u.start_begin) < HXP_MAX_SPAN_US && hxdiagDelta(u.start_end,u.start_begin) < HXP_MAX_SPAN_US && hxdiagDelta(u.stop_begin,u.start_end) < HXP_MAX_SPAN_US && hxdiagDelta(u.stop_end,u.stop_begin) < HXP_MAX_SPAN_US && hxdiagDelta(u.read_begin,u.stop_end) < HXP_MAX_SPAN_US && hxdiagDelta(u.read_end,u.read_begin) < HXP_MAX_SPAN_US; // pending/range/API/反序失效，不造零。
} // 結束unit有效性。
inline HxAttemptPcntUnit hxapPack(const HxPcntUnit &u) { // 深拷貝在hold CLEAR同硬體之前，避免首LINK覆寫frame證據。
  HxAttemptPcntUnit p; p.start_begin=u.start_begin;p.start_end=u.start_end;p.stop_begin=u.stop_begin;p.stop_end=u.stop_end;p.read_begin=u.read_begin;p.read_end=u.read_end;p.count=u.count; // 原時間/count如實保存。
  p.flags=(u.api_mask&15)|((u.attempt_mask&7)<<4)|(u.overflow?128U:0U)|((u.attempt_mask&1)?512U:0U)|((u.level_start&1)<<10)|((u.attempt_mask&2)?2048U:0U)|((u.level_end&1)<<12); // level是窗內離散值，不是精確counter閘門端點。
  if(hxapUnitValid(p)){p.flags|=256;}return p; // validity只關於硬體讀取，不對RAW接受作任何決策。
} // 結束compact拷貝。
inline bool hxapEnclosed(const HxAttemptPcnt &p) { return hxdiagDelta(p.unit[1].start_begin,p.unit[0].start_end)<HXP_MAX_SPAN_US&&hxdiagDelta(p.unit[0].stop_begin,p.unit[1].stop_end)<HXP_MAX_SPAN_US&&hxdiagDelta(p.unit[0].stop_begin,p.unit[1].read_end)<HXP_MAX_SPAN_US; } // SCK窗包含DATA窗，DATA停後讀回也在SCK停之前。
inline bool hxapContainsRead(const HxAttemptPcnt &p,const HxDiagRecord &r) { // 每unit至少包住原read CPU括弧，還可能含lead-in/tail變化。
  for(uint8_t i=0;i<2;++i){if(hxdiagDelta(r.start_us,p.unit[i].start_end)>=HXP_MAX_SPAN_US||hxdiagDelta(p.unit[i].stop_begin,r.end_us)>=HXP_MAX_SPAN_US)return false;}return true; // 不把窗口總數全歸給train內。
} // 結束原read包含關係。
inline bool hxapUsable(const HxAttemptPcnt &p,const HxDiagRecord &r) { // 原accepted=0的LINK也可具有有效counter。
  const uint32_t required=HXAP_PRESENT|HXAP_SETUP|HXAP_PROOF|HXAP_ELIGIBLE|HXAP_ENCLOSED|HXAP_FINISHED;return p.seq==r.seq&&(p.policy&required)==required&&!(p.policy&HXAP_GUARD)&&!(p.policy>>16)&&((p.unit[0].flags&256)!=0)&&((p.unit[1].flags&256)!=0)&&hxapUnitValid(p.unit[0])&&hxapUnitValid(p.unit[1])&&hxapEnclosed(p)&&hxapContainsRead(p,r)&&hxdiagDelta(p.unit[0].start_begin,p.prepare_begin)<HXP_MAX_SPAN_US&&hxdiagDelta(p.finish_end,p.unit[0].read_end)<HXP_MAX_SPAN_US&&hxdiagDelta(p.finish_end,p.prepare_begin)<HXP_MAX_SPAN_US; // mismatch不影響valid或RAW，只作離線候選。
} // 結束pair用途有效性。
inline void hxapObserveShared(HxDiagState &raw,HxAttemptPcntRing &side,HxDiagRecord r,HxAttemptPcnt p) { // 不另分配seq、只跟真實RAW append。
  const uint16_t index=raw.next;hxdiagObserveShared(raw,r);if(raw.next!=index)side.records[index]=p; // 不替舊pending換標；錯seq由Aligned/parser拒絕，未append不造ghost。
} // 結束同步保存。
template<typename Add>inline void hxapCanonical(const HxAttemptPcnt &p,Add add){add(p.seq);add(p.policy);add(p.prepare_begin);add(p.finish_end);for(uint8_t i=0;i<2;++i){const HxAttemptPcntUnit &u=p.unit[i];add(u.start_begin);add(u.start_end);add(u.stop_begin);add(u.stop_end);add(u.read_begin);add(u.read_end);add(static_cast<uint32_t>(u.count));add(u.flags);}} // 每row二十個LE uint32，不依padding。
inline uint32_t hxapCrc(const HxAttemptPcnt &p){uint32_t crc=0xFFFFFFFFUL;hxapCanonical(p,[&](uint32_t w){crc=hxdiagCrcWord(crc,w);});return crc^0xFFFFFFFFUL;} // 只在凍結匯出時計算。
#endif // 結束每讀框PCNT純狀態。
