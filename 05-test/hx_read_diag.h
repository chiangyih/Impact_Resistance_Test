#ifndef IRT_HX_READ_DIAG_H // 固定RAM逐次讀取摘要，不包含GPIO或輸出操作。
#define IRT_HX_READ_DIAG_H // 本狀態機與量測、校正及警示計數分離。
#include <stdint.h> // 使用明確寬度與無號時間差。

const uint16_t HXD_CAPACITY = 256, HXD_PRE = 127, HXD_POST = 128; // 前史加觸發筆加後史恰為固定容量。
const uint32_t HXD_WINDOW_US = 600000000UL, HXD_POST_US = 20000000UL; // 總觀測十分鐘，後窗最多二十秒，不因新事件延長。
enum HxDiagMode : uint8_t { HXD_LIVE, HXD_POST_CAPTURE, HXD_FROZEN }; // 僅表示診斷ring狀態。
enum HxDiagCause : uint32_t { HXD_LINK = 1, HXD_SAT = 2, HXD_JUMP = 4 }; // 檢索事件，不更動有效性或修正RAW。
enum HxDiagFreeze : uint8_t { HXD_NOT_FROZEN, HXD_POST_FULL, HXD_POST_TIMEOUT, HXD_WINDOW_LIMIT, HXD_OVERFLOW }; // 凍結原因分列。
struct HxDiagRecord { // 每筆記錄一次真正進入二十五脈衝的嘗試，包含拒絕框。
  uint32_t seq, start_us, end_us, attempt_gap_us, last_accept_ms_before; // CPU時間與上一成功讀取的毫秒快取。
  uint32_t bits24; int32_t raw; // 原始取得字與相同字的補數解碼，不是假定ADC真值。
  uint32_t rx_before, rx_after, err_before, err_after, flags; // 原警示/接收計數原樣綁定。
  uint16_t cal_count_after; uint8_t phase_after, cal_mode_after; // 原本量測/校正處理完成後的背景脈絡。
  uint8_t accepted, dout_after25, pulses_issued, context_flags; // 接受/拒絕、原後檢查及軟體脈衝數；context位零表示原先見過成功框。
}; // 結束固定摘要欄位。
static_assert(sizeof(HxDiagRecord) == 56, "HX diagnostic record size changed"); // 目前固定記錄共14336 bytes，不分配heap。
struct HxDiagState { // 診斷資料只由主迴圈寫入，沒有ISR修改。
  HxDiagRecord records[HXD_CAPACITY] = {}; // 固定容量，未凍結時只保留有限前史。
  uint16_t next = 0, count = 0, pre_count = 0, post_count = 0; // 記錄索引與實際保存數。
  uint32_t attempts = 0, omitted_after_freeze = 0, pre_overwritten = 0; // 計數只含已走到hook的完成嘗試；前史覆寫與凍結後未收錄不同於overflow。
  uint32_t trigger_mask = HXD_LINK | HXD_SAT | HXD_JUMP; // ANY與LINK槽各自選觸發原因，但保存相同原始flags。
  uint32_t export_attempts = 0, export_omitted = 0, export_pre_overwritten = 0; // 封存後匯出固定計數，live計數另列STATUS。
  uint32_t start_us = 0, trigger_us = 0, freeze_us = 0, trigger_seq = 0, trigger_flags = 0; // 所有裝置時間均為boot micros低三十二位。
  uint32_t last_attempt_start_us = 0; int32_t last_accepted_raw = 0; // 用有效旗標分辨零時刻或零RAW。
  uint8_t mode = HXD_LIVE, freeze_reason = HXD_NOT_FROZEN; // 不影響一般phase。
  bool started = false, attempt_seen = false, accepted_seen = false, overflow = false, seq_wrapped = false; // 未知與有效零分開。
  bool export_sealed = false, export_seq_wrapped = false; // 兩槽payload及匯出metadata各自封存一次。
}; // 結束固定RAM狀態。
inline uint32_t hxdiagDelta(uint32_t now, uint32_t then) { // 窗口小於三十二位微秒週期，可用無號差跨wrap。
  return now - then; // 不把boot時間當UTC或精準ADC時間。
} // 結束無號差。
inline void hxdiagBegin(HxDiagState &s, uint32_t now) { // 只允許新boot的一次開始，不提供清除/重新啟動命令。
  if (s.started) return; // 不覆寫先前證據。
  s.started = true; s.start_us = now; // 從setup明確開始，沒有attempt也會到總期限。
} // 結束開始。
inline void hxdiagFreeze(HxDiagState &s, uint32_t now, uint8_t reason) { // 只凍結診斷，不STOP/CLEAR或改校正。
  if (s.mode == HXD_FROZEN) return; // 第一個凍結時間固定。
  s.mode = HXD_FROZEN; s.freeze_us = now; s.freeze_reason = reason; // caller提供讀取括弧端點或服務CPU時間，非實體GPIO凍結瞬間。
} // 結束凍結。
inline void hxdiagTick(HxDiagState &s, uint32_t now) { // 可在無新資料時處理有界截止。
  if (!s.started || s.mode == HXD_FROZEN) return; // 不改已凍結的紀錄。
  if (hxdiagDelta(now, s.start_us) >= HXD_WINDOW_US) hxdiagFreeze(s, now, HXD_WINDOW_LIMIT); // 總期限優先且永不延長。
  else if (s.mode == HXD_POST_CAPTURE && hxdiagDelta(now, s.trigger_us) >= HXD_POST_US) hxdiagFreeze(s, now, HXD_POST_TIMEOUT); // HX停止更新也可結束後窗。
} // 結束截止服務。
inline bool hxdiagAppend(HxDiagState &s, const HxDiagRecord &record) { // 固定RAM寫入，不輸出或配置記憶體。
  if (s.count >= HXD_CAPACITY) { s.overflow = true; hxdiagFreeze(s, record.end_us, HXD_OVERFLOW); return false; } // 不靜默覆寫事件後史。
  s.records[s.next] = record; s.next = (s.next + 1) % HXD_CAPACITY; ++s.count; // 只複製固定摘要。
  return true; // 寫入是否成功可供狀態機判斷。
} // 結束追加。
inline bool hxdiagCanRetain(const HxDiagState &s, uint32_t start) { // 與Observe相同的開始時間判準，不改狀態或延長期限。
  return s.started && s.mode != HXD_FROZEN && hxdiagDelta(start, s.start_us) < HXD_WINDOW_US && (s.mode != HXD_POST_CAPTURE || hxdiagDelta(start, s.trigger_us) < HXD_POST_US); // resume與probe不得綁到已逾期的attempt。
} // 結束保留資格。
inline void hxdiagSeal(HxDiagState &s) { // 在Observe處理完未收錄計數後或Tick後封存，避免header後續漂移。
  if (s.mode != HXD_FROZEN || s.export_sealed) return; // 不凍結活動槽，不重新封存。
  s.export_attempts = s.attempts; s.export_omitted = s.omitted_after_freeze; s.export_pre_overwritten = s.pre_overwritten; s.export_seq_wrapped = s.seq_wrapped; s.export_sealed = true; // 所有後續live attempt不回寫。
} // 結束一次封存。
inline void hxdiagObserveShared(HxDiagState &s, HxDiagRecord record) { // 以唯一完成attempt序號分送兩槽，不由槽各自配號。
  if (!s.started) return; // setup開始前不推定存在資料。
  s.attempts = record.seq; if (!record.seq) s.seq_wrapped = true; // caller每個完成attempt只配一次序號，包含拒絕。
  hxdiagTick(s, record.start_us); // 超過總觀測期限的嘗試不重新開窗。
  if (s.mode == HXD_FROZEN) { ++s.omitted_after_freeze; return; } // 凍結後不覆寫，明示未收錄數。
  record.attempt_gap_us = s.attempt_seen ? hxdiagDelta(record.start_us, s.last_attempt_start_us) : 0; // 首筆間隔無效由context旗標說明。
  if (s.attempt_seen) record.context_flags |= 2; s.attempt_seen = true; s.last_attempt_start_us = record.start_us; // 記錄的是attempt間隔，不是每次poll間隔。
  record.flags = record.accepted ? 0 : HXD_LINK; // 以原DATA後檢結果分類，不改原判斷。
  if (record.accepted) { // 只與原契約接受的RAW比較，大跳也照存照接受。
    if (record.raw == -8388608L || record.raw == 8388607L) record.flags |= HXD_SAT; // 飽和另列，不抹除真值。
    const int32_t difference = record.raw - s.last_accepted_raw; // 二十四位差值可由int32完整表示。
    if (s.accepted_seen && (difference >= 1000 || difference <= -1000)) record.flags |= HXD_JUMP; // 一千count僅檢索事件，不是驗收門檻。
    s.last_accepted_raw = record.raw; s.accepted_seen = true; // 不使用拒絕框改寫比較基準。
  } // 結束接受框脈絡。
  if (s.mode == HXD_LIVE && (record.flags & s.trigger_mask)) { // 第一個符合槽政策的事件固定，其他flags仍原樣保留。
    s.pre_count = s.count; s.trigger_seq = record.seq; s.trigger_us = record.end_us; s.trigger_flags = record.flags; s.mode = HXD_POST_CAPTURE; // 觸發筆不算前後筆。
    if (!hxdiagAppend(s, record)) return; // 容量異常保留overflow。
  } else if (s.mode == HXD_LIVE) { // 一般前史以固定長度滾動。
    if (!hxdiagAppend(s, record)) return; // 不假定每次寫入必成功。
    if (s.count > HXD_PRE) { s.count = HXD_PRE; ++s.pre_overwritten; } // 有意滾動覆寫需明示，不假稱保存全程。
  } else { // 事件後史逐筆保存，包括新增拒絕框。
    if (!hxdiagAppend(s, record)) return; ++s.post_count; // 首個事件不重啟計數。
  } // 結束保存路徑。
  hxdiagTick(s, record.end_us); // 保存本筆後檢查已經過的時間。
  if (s.mode == HXD_POST_CAPTURE && s.post_count >= HXD_POST) hxdiagFreeze(s, record.end_us, HXD_POST_FULL); // 滿額後不再寫入。
} // 結束逐次摘要。
inline void hxdiagObserve(HxDiagState &s, HxDiagRecord record) { // 舊主機狀態機測試入口；正式雙槽使用Shared介面。
  record.seq = hxdiagDelta(s.attempts + 1, 0); hxdiagObserveShared(s, record); // 保留舊單槽測試契約，不用於正式雙配號。
} // 結束單槽相容入口。
inline uint16_t hxdiagIndex(const HxDiagState &s, uint16_t ordinal) { // 將時間順序映射到固定ring。
  return (s.next + HXD_CAPACITY - s.count + ordinal) % HXD_CAPACITY; // 呼叫端只列印ordinal<count。
} // 結束索引。
inline uint32_t hxdiagCrcWord(uint32_t crc, uint32_t value) { // canonical little-endian word，僅匯出時計算。
  for (uint8_t byte = 0; byte < 4; ++byte) { // 不依賴struct padding或主機端序。
    crc ^= value & 255; value >>= 8; // 一個word低位元組先行。
    for (uint8_t bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320UL : 0); // 標準CRC32檢查輸出紀錄。
  } // 結束四位元組。
  return crc; // 初末反相由完整紀錄函式處理。
} // 結束word CRC。
inline uint32_t hxdiagCrc(const HxDiagRecord &r) { // 不在GPIO讀取或保存時呼叫。
  uint32_t crc = 0xFFFFFFFFUL; // 匯出資料形成後的完整性，不是ADC自帶CRC。
  const uint32_t words[] = {r.seq, r.start_us, r.end_us, r.attempt_gap_us, r.last_accept_ms_before, r.bits24, static_cast<uint32_t>(r.raw), r.rx_before, r.rx_after, r.err_before, r.err_after, r.flags, r.cal_count_after, r.phase_after, r.cal_mode_after, r.accepted, r.dout_after25, r.pulses_issued, r.context_flags}; // 固定十九個word。
  for (uint8_t i = 0; i < 19; ++i) crc = hxdiagCrcWord(crc, words[i]); // canonical欄位順序與CSV一致。
  return crc ^ 0xFFFFFFFFUL; // CRC只支持匯出傳輸，不保證取得字正確。
} // 結束完整紀錄CRC。
#endif // 結束逐次摘要定義。
