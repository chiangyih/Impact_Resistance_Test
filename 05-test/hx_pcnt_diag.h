#ifndef IRT_HX_PCNT_DIAG_H // PCNT只記MCU側數位邊緣，不改原HX字或時脈。
#define IRT_HX_PCNT_DIAG_H // 純主機可測狀態，不包含GPIO或driver。
#include "hx_read_diag.h" // 沿用CPU無號時間差及匯出CRC。
const uint8_t HXP_PROOFS = 8, HXP_REQUIRED = 4, HXP_UNITS = 2, HXP_STAGES = 4; // 有界正對照，不是ADC正確性proof。
const uint32_t HXP_LIMIT = 32767, HXP_VALID_MAX = HXP_LIMIT / 2, HXP_MAX_SPAN_US = 1000000; // pending補償僅覆蓋half內，較大回值保守無效。
enum HxPcntMode : uint8_t { HXP_WAITING, HXP_RUNNING, HXP_COMPLETE, HXP_SKIPPED, HXP_NO_LINK }; // 模式不改量測phase。
enum HxPcntReason : uint8_t { HXP_NONE, HXP_SETUP, HXP_NO_PROOF, HXP_BUSY, HXP_API, HXP_OVERFLOW, HXP_GPIO, HXP_TIME, HXP_RANGE }; // range不是宣稱實際overflow。
struct HxPcntPin { uint32_t function = 0, signal = 0, drive = 0, flags = 0; }; // flags依序PU/PD/IE/OE/OECTRL/OEINV/OD/SLP。
struct HxPcntUnit { // 每個unit獨立CPU括弧，SCK窗必須包住DATA窗。
  uint32_t start_begin = 0, start_end = 0, stop_begin = 0, stop_end = 0, read_begin = 0, read_end = 0; // API括弧不是管腳邊緣時刻。
  int32_t count = 0; uint32_t api_mask = 0, overflow = 0, valid = 0; // mask CLEAR/START/STOP/READ各一bit。
  uint32_t level_start_begin = 0, level_start_end = 0, level_start = 0, level_end_begin = 0, level_end_end = 0, level_end = 0, attempt_mask = 0; // START/STOP/READ是否實際呼叫另列，未知輸出NA。
}; // 結束unit固定摘要。
struct HxPcntProof { uint32_t seq = 0, bits24 = 0, accepted = 0, postcheck = 0, expected_data = 0, valid = 0; HxPcntUnit unit[HXP_UNITS] = {}; }; // 最多八筆既有clock正對照。
struct HxPcntDiag { // 所有可匯出資料固定RAM，driver配置另列heap。
  uint32_t mode = HXP_WAITING, reason = HXP_NONE, linked = 0, seq = 0, anchor = 0, setup_code = 0, clock_blocked = 0; // setup_code一才可用。
  uint32_t proof_count = 0, proof_good = 0, proof_bad = 0, proof_ready = 0, stage_mask = 0; // 四個良好accepted對照且無壞筆才ready。
  uint32_t event_begin = 0, event_end = 0, finish_begin = 0, finish_end = 0, sealed = 0, gpio_function = 0, gpio_signal = 0; // 首LINK啟停開銷另存。
  HxPcntPin pins[HXP_STAGES][HXP_UNITS] = {}; // BASE/DRIVER/READY/EVENT，不輸出其他記憶體。
  HxPcntProof proof[HXP_PROOFS] = {}; HxPcntUnit unit[HXP_UNITS] = {}; // 兩unit每次只計總雙edge。
}; // 結束固定PCNT資料。
inline bool hxpSamePin(const HxPcntPin &a, const HxPcntPin &b, bool add_input = false) { // 僅SCK可追加input enable，絕不改output。
  return a.function == b.function && a.signal == b.signal && a.drive == b.drive && ((a.flags | (add_input ? 4U : 0U)) == b.flags); // pull/drive/OE/route均比對。
} // 結束腳位差異檢查。
inline bool hxpBaseline(const HxPcntDiag &s) { // 原程式GPIO25為GPIO output無pull，33為INPUT_PULLUP。
  const HxPcntPin &a = s.pins[0][0], &b = s.pins[0][1]; // baseline由實際readback取得。
  return (s.stage_mask & 1U) && a.function == s.gpio_function && a.signal == s.gpio_signal && (a.flags & ~(4U | 16U)) == 8U && b.function == s.gpio_function && b.flags == 5U; // 原GPIO輸出的OECTRL可為零或一，但後續必須逐位保留。
} // 結束原始配置檢查。
inline bool hxpPinsMatch(const HxPcntDiag &s, uint8_t stage) { // DRIVER暫時pull另存，不拿它冒充READY。
  return stage < HXP_STAGES && (s.stage_mask & (1U << stage)) && hxpSamePin(s.pins[0][0], s.pins[stage][0], true) && hxpSamePin(s.pins[0][1], s.pins[stage][1]); // 未讀回即無效。
} // 結束配置一致性。
inline uint32_t hxpExpectedData(uint32_t bits, bool postcheck) { // 只供既有正常框的counter alive對照，不驗ADC真值。
  uint32_t edges = 0, level = 0; // 原DATA gate為LOW。
  for (int bit = 23; bit >= 0; --bit) { const uint32_t next = (bits >> bit) & 1U; edges += next != level; level = next; } // 原二十四個取得點的數位變化數。
  return edges + (level != (postcheck ? 1U : 0U)); // 最後原postcheck另算一次變化。
} // 結束正對照預期值。
inline bool hxpUnitValid(const HxPcntUnit &u) { // 停後get_count包含pending overflow補償，limit本身一律無效。
  return u.api_mask == 15 && u.attempt_mask == 7 && !u.overflow && u.count >= 0 && static_cast<uint32_t>(u.count) <= HXP_VALID_MAX && hxdiagDelta(u.stop_end, u.start_begin) < HXP_MAX_SPAN_US && hxdiagDelta(u.start_end, u.start_begin) < HXP_MAX_SPAN_US && hxdiagDelta(u.stop_begin, u.start_end) < HXP_MAX_SPAN_US && hxdiagDelta(u.read_begin, u.stop_end) < HXP_MAX_SPAN_US && hxdiagDelta(u.read_end, u.read_begin) < HXP_MAX_SPAN_US && hxdiagDelta(u.level_start_begin, u.start_end) < HXP_MAX_SPAN_US && hxdiagDelta(u.level_start_end, u.level_start_begin) < HXP_MAX_SPAN_US && hxdiagDelta(u.level_end_begin, u.level_start_end) < HXP_MAX_SPAN_US && hxdiagDelta(u.level_end_end, u.level_end_begin) < HXP_MAX_SPAN_US && hxdiagDelta(u.stop_begin, u.level_end_end) < HXP_MAX_SPAN_US && u.level_start <= 1 && u.level_end <= 1; // 過長或反序不造零。
} // 結束unit有效性。
inline bool hxpEnclosed(const HxPcntUnit *u) { // 用API最保守邊界證DATA窗包在SCK窗內。
  return hxdiagDelta(u[1].start_begin, u[0].start_end) < HXP_MAX_SPAN_US && hxdiagDelta(u[0].stop_begin, u[1].stop_end) < HXP_MAX_SPAN_US; // 不假定兩unit同時啟停。
} // 結束包絡檢查。
inline void hxpAddProof(HxPcntDiag &s, HxPcntProof p) { // caller已停兩個unit才分類，不影響原accepted。
  if (s.sealed || s.linked || s.proof_ready || s.proof_count >= HXP_PROOFS) return; // 首事件或封存後不回寫。
  p.expected_data = hxpExpectedData(p.bits24, p.postcheck); // DATA counter接原GPIO33，沒有內部替代訊號。
  for (uint8_t i = 0; i < HXP_UNITS; ++i) p.unit[i].valid = hxpUnitValid(p.unit[i]); // 逐unit API/overflow各自驗。
  p.valid = s.setup_code == 1 && hxpPinsMatch(s, 2) && p.accepted && p.postcheck && p.unit[0].valid && p.unit[1].valid && hxpEnclosed(p.unit) && p.unit[0].count == 50 && p.expected_data > 0 && static_cast<uint32_t>(p.unit[1].count) == p.expected_data && p.unit[1].level_start == 0 && p.unit[1].level_end == 1; // 只是本機counter alive。
  s.proof[s.proof_count++] = p; if (p.valid) ++s.proof_good; else ++s.proof_bad; s.proof_ready = s.proof_good >= HXP_REQUIRED && !s.proof_bad; // 不清壞筆來湊通過。
} // 結束一次對照。
inline bool hxpLink(HxPcntDiag &s, uint32_t seq, uint32_t anchor, bool eligible, uint32_t now) { // 與原HXF同一首LINK，無proof時保留NA。
  if (s.sealed || s.linked || s.mode != HXP_WAITING) return false; // 不再武裝。
  s.linked = 1; s.seq = seq; s.anchor = anchor; s.event_begin = now; // 原postcheck到此仍有早期盲區。
  s.reason = !eligible ? HXP_BUSY : s.setup_code != 1 ? HXP_SETUP : !s.proof_ready ? HXP_NO_PROOF : !hxpPinsMatch(s, 3) ? HXP_GPIO : HXP_NONE; // 不把未配置/未驗證當零。
  s.mode = s.reason ? HXP_SKIPPED : HXP_RUNNING; return s.mode == HXP_RUNNING; // 只有明確可用才啟硬體counter。
} // 結束首LINK鎖定。
inline void hxpComplete(HxPcntDiag &s, uint32_t now) { // caller先stop/read兩unit，後續合法clock不得污染。
  if (s.sealed || s.mode != HXP_RUNNING) return; // 固定資料不可改。
  for (uint8_t i = 0; i < HXP_UNITS; ++i) s.unit[i].valid = hxpUnitValid(s.unit[i]); // count=0只有valid才有意義。
  if ((!s.unit[0].valid || !s.unit[1].valid) && s.reason == HXP_NONE) s.reason = (s.unit[0].overflow || s.unit[1].overflow) ? HXP_OVERFLOW : (s.unit[0].count > static_cast<int32_t>(HXP_VALID_MAX) || s.unit[1].count > static_cast<int32_t>(HXP_VALID_MAX)) ? HXP_RANGE : HXP_API; // 回值超half不判無overflow，不改HX ERR。
  else if (!hxpEnclosed(s.unit)) { s.reason = HXP_TIME; s.unit[0].valid = s.unit[1].valid = 0; } // 包絡失敗不能推無clock期間的DATA事件。
  if (s.reason != HXP_NONE) s.unit[0].valid = s.unit[1].valid = 0; // 不完整兩窗不呈有效零計數。
  s.mode = HXP_COMPLETE; s.finish_end = now; // COMPLETE不保證有效。
} // 結束PCNT觀察。
inline void hxpSeal(HxPcntDiag &s) { // 僅在原LINK槽及HXF固定後使用。
  if (s.sealed || s.mode == HXP_RUNNING) return; // 不能封存還在變的counter。
  if (!s.linked) { s.mode = HXP_NO_LINK; } s.sealed = 1; // 無LINK不造零計數。
} // 結束固定snapshot。
template <typename Add> inline void hxpCanonical(const HxPcntDiag &s, Add add) { // 單一欄位順序供CRC與離線協定，全部LE uint32。
  const uint32_t meta[] = {s.mode,s.reason,s.linked,s.seq,s.anchor,s.setup_code,s.clock_blocked,s.proof_count,s.proof_good,s.proof_bad,s.proof_ready,s.stage_mask,s.event_begin,s.event_end,s.finish_begin,s.finish_end,s.sealed,s.gpio_function,s.gpio_signal}; // 十九個metadata。
  for (uint8_t i = 0; i < 19; ++i) add(meta[i]); // 包含有效性與真實時間。
  for (uint8_t stage = 0; stage < HXP_STAGES; ++stage) for (uint8_t i = 0; i < HXP_UNITS; ++i) { const HxPcntPin &p = s.pins[stage][i]; add(p.function); add(p.signal); add(p.drive); add(p.flags); } // 三十二個腳位word。
  auto unit = [&](const HxPcntUnit &u) { const uint32_t w[] = {u.start_begin,u.start_end,u.stop_begin,u.stop_end,u.read_begin,u.read_end,static_cast<uint32_t>(u.count),u.api_mask,u.overflow,u.valid,u.level_start_begin,u.level_start_end,u.level_start,u.level_end_begin,u.level_end_end,u.level_end,u.attempt_mask}; for (uint8_t i = 0; i < 17; ++i) add(w[i]); }; // 不依struct padding。
  for (uint8_t i = 0; i < HXP_PROOFS; ++i) { const HxPcntProof &p = s.proof[i]; add(p.seq); add(p.bits24); add(p.accepted); add(p.postcheck); add(p.expected_data); add(p.valid); for (uint8_t j = 0; j < HXP_UNITS; ++j) unit(p.unit[j]); } // 固定八個槽，未使用內部零以proof_count保護。
  for (uint8_t i = 0; i < HXP_UNITS; ++i) unit(s.unit[i]); // 真LINK的兩個窗另存。
} // 結束canonical欄位。
inline uint32_t hxpCrc(const HxPcntDiag &s) { uint32_t crc = 0xFFFFFFFFUL; hxpCanonical(s, [&](uint32_t w) { crc = hxdiagCrcWord(crc, w); }); return crc ^ 0xFFFFFFFFUL; } // 僅封存後匯出，不驗ADC真值。
#endif // 結束PCNT純狀態。
