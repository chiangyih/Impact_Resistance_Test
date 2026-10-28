#ifndef IRT_HX_FAILURE_DIAG_H // 首次LINK後維持既有SCK LOW的有界離散觀察。
#define IRT_HX_FAILURE_DIAG_H // 不提供重設、補clock或接受失敗字的恢復策略。
#include "hx_read_diag.h" // 沿用無號CPU時間差及只在匯出時計算的CRC。
const uint8_t HXF_POINTS = 12; // 前六點短觀察，後六點由主迴圈服務。
const uint32_t HXF_TARGET_US[HXF_POINTS] = {2, 10, 50, 200, 1000, 2000, 5000, 20000, 50000, 100000, 150000, 250000}; // 全部相對原postcheck後CPU anchor。
const uint32_t HXF_EARLY_US = 2000, HXF_HOLD_US = 250000; // 目標截止，不是排程硬即時上限。
enum HxFailMode : uint8_t { HXF_WAITING, HXF_HOLD, HXF_COMPLETE, HXF_SKIPPED_BUSY, HXF_CLOSED_NO_LINK, HXF_TRUNCATED }; // 未取樣與有效LOW分開。
enum HxFailStop : uint8_t { HXF_STOP_NONE, HXF_STOP_TARGET, HXF_STOP_BUSY, HXF_STOP_RING, HXF_STOP_WINDOW }; // 後窗/總窗可先截止。
struct HxFailPoint { // 每點的CPU讀取括弧，不是連續GPIO波形。
  uint32_t begin_us = 0, end_us = 0; uint8_t dout = 0, valid = 0, missed = 0; // valid為零時數值欄匯出NA。
}; // 結束固定點摘要。
struct HxFailureDiag { // 首次符合窗口的LINK鎖定後不可覆寫。
  HxFailPoint points[HXF_POINTS] = {}; // 不配置heap。
  uint32_t seq = 0, read_start_us = 0, read_end_us = 0, probe_start_us = 0, probe_end_us = 0; // 原讀取與介入分開計時。
  uint32_t resume_start_us = 0, resume_seq = 0, rx = 0, err_before = 0, err_after = 0, bits24 = 0; int32_t raw = 0; // 下一次真正attempt另標有效性。
  uint8_t mode = HXF_WAITING, stop_reason = HXF_STOP_NONE, next = 0, phase_before = 0, cal_before = 0; // 背景狀態取失敗當時。
  bool captured = false, resume_valid = false; // 未觸發或尚未resume不能當零時刻。
}; // 結束固定trace。
inline bool hxfStart(HxFailureDiag &s, uint32_t seq, uint32_t read_start, uint32_t read_end, uint32_t bits, uint32_t rx, uint32_t errors, uint8_t phase, uint8_t cal, bool eligible, uint32_t now) { // caller只從原postcheck拒絕路徑呼叫。
  if (s.mode != HXF_WAITING) return false; // 第一事件固定，包括busy時的未介入證據。
  s.captured = true; s.seq = seq; s.read_start_us = read_start; s.read_end_us = read_end; s.probe_start_us = now; // anchor永不於2ms後重設。
  s.bits24 = bits & 0xFFFFFFUL; s.raw = static_cast<int32_t>(s.bits24) - ((s.bits24 & 0x800000UL) ? 16777216L : 0L); // 僅診斷解碼，不寫有效RAW。
  s.rx = rx; s.err_after = errors; s.err_before = errors - 1; s.phase_before = phase; s.cal_before = cal; // 原拒絕已使ERR加一，RX保持。
  s.mode = eligible ? HXF_HOLD : HXF_SKIPPED_BUSY; s.stop_reason = eligible ? HXF_STOP_NONE : HXF_STOP_BUSY; // 原IDLE且無CAL才延後下一HX attempt。
  if (!eligible) s.probe_end_us = now; // busy不等待、不清狀態。
  return true; // 一次鎖定不代表資料或時序通過。
} // 結束首次LINK鎖定。
inline void hxfStop(HxFailureDiag &s, uint8_t reason, uint32_t now) { // 只結束診斷hold，不改原感測器或校正。
  if (s.mode == HXF_WAITING) { s.mode = HXF_CLOSED_NO_LINK; s.stop_reason = reason; return; } // 沒有LINK，不造一筆零失敗。
  if (s.mode != HXF_HOLD) return; // 已結束證據不可改寫。
  s.mode = HXF_TRUNCATED; s.stop_reason = reason; s.probe_end_us = now; // 剩餘點保留valid0，與逾點不同。
} // 結束外部期限服務。
inline int hxfDue(HxFailureDiag &s, uint32_t now) { // 非阻塞選一個到期點，不catch-up成多個假準時樣本。
  if (s.mode != HXF_HOLD || s.next >= HXF_POINTS) return -1; // 尚未觸發或已完成時不讀GPIO。
  const uint32_t elapsed = hxdiagDelta(now, s.read_end_us); // 所有點使用同一原anchor。
  while (s.next + 1 < HXF_POINTS && elapsed >= HXF_TARGET_US[s.next + 1]) { s.points[s.next].missed = 1; ++s.next; } // 跨越下一目標的點標missing，不補讀。
  return elapsed >= HXF_TARGET_US[s.next] ? s.next : -1; // 最後點允許晚到，匯出實際CPU括弧。
} // 結束到期點選擇。
inline void hxfSample(HxFailureDiag &s, uint32_t before, uint32_t after, bool high) { // caller只讀DATA，沒有SCK寫入或額外clock。
  if (hxfDue(s, before) < 0) return; // 以實際before重新分類，中斷若跨點就標missing，不補採舊target。
  HxFailPoint &p = s.points[s.next++]; p.begin_us = before; p.end_us = after; p.dout = high ? 1 : 0; p.valid = 1; // LOW/HIGH只代表這次離散讀取。
  if (s.next == HXF_POINTS) { s.mode = HXF_COMPLETE; s.stop_reason = HXF_STOP_TARGET; s.probe_end_us = after; } // 實際hold可因排程超過250ms。
} // 結束單一樣本。
inline void hxfResume(HxFailureDiag &s, uint32_t start, uint32_t seq, bool ring_open) { // 只在ring尚未FROZEN時標記下一次真正attempt。
  if (!ring_open || (s.mode != HXF_COMPLETE && s.mode != HXF_TRUNCATED) || s.resume_valid) return; // 匯出後不回寫resume或改變CRC，不把poll算attempt。
  s.resume_start_us = start; s.resume_seq = seq; s.resume_valid = true; // 不重設trace或修改下一框接受結果。
} // 結束恢復觀察標籤。
inline uint32_t hxfCrc(const HxFailureDiag &s) { // 整個固定trace的canonical CRC，僅匯出時計算。
  uint32_t crc = 0xFFFFFFFFUL; // CRC不是ADC真值或GPIO波形驗證。
  const uint32_t words[] = {s.mode, s.stop_reason, s.captured, s.seq, s.read_start_us, s.read_end_us, s.probe_start_us, s.probe_end_us, s.resume_start_us, s.resume_seq, s.resume_valid, s.rx, s.err_before, s.err_after, s.bits24, static_cast<uint32_t>(s.raw), s.phase_before, s.cal_before, s.next}; // 固定十九個metadata word。
  for (uint8_t i = 0; i < 19; ++i) crc = hxdiagCrcWord(crc, words[i]); // 不依賴struct padding或平台端序。
  for (uint8_t i = 0; i < HXF_POINTS; ++i) { const HxFailPoint &p = s.points[i]; const uint32_t point[] = {i, HXF_TARGET_US[i], p.begin_us, p.end_us, p.dout, p.valid, p.missed}; for (uint8_t j = 0; j < 7; ++j) crc = hxdiagCrcWord(crc, point[j]); } // invalid內部零仍由valid保護，CSV輸出NA。
  return crc ^ 0xFFFFFFFFUL; // 完整性涵蓋valid、missing、實際CPU時間及DATA值。
} // 結束匯出完整性。
#endif // 結束失敗後DATA診斷。
