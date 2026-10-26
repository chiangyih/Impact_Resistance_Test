#pragma once // 獨立光學狀態，不讀感測器、不控制釋放機構。
#include <stdint.h> // 明確使用三十二位無號時間差。

enum OpticalState : uint8_t { OPT_OFF, OPT_WAIT_UP, OPT_WAIT_DN, OPT_DONE, OPT_FAULT }; // 不借用一般ARM狀態。
enum OpticalReason : uint8_t { OPT_NONE, OPT_LOWER_FIRST, OPT_TOO_FAST, OPT_LOWER_TIMEOUT, OPT_USER_STOP }; // 失敗原因與缺值分開。
const uint32_t OPTICAL_MIN_US = 200, OPTICAL_MAX_US = 30000000; // 初步防錯範圍，不是精度驗收。
volatile uint8_t optical_state = OPT_OFF, optical_reason = OPT_NONE; // 呼叫端用gate_mux保護更新。
volatile uint32_t optical_upper_us = 0, optical_lower_us = 0, optical_ignored = 0; // 只保留首個有效緣，不配置連續資料陣列。
volatile bool optical_upper_seen = false, optical_lower_seen = false; // 零時間也可有效。
uint32_t optical_id = 0; // 本次開機光學識別，不當全域試驗ID。
double optical_distance_mm = 0; // 開始時凍結距離。
int optical_upper_active = -1, optical_lower_active = -1; // 保存已驗極性。
bool optical_reported = false; // 結果只通知一次並保留。

bool ARDUINO_ISR_ATTR opticalMode() { // 可由ISR或主迴圈讀取單一位元組狀態。
  return optical_state != OPT_OFF; // 完成後亦保留模式直到CLEAR。
} // 結束模式判斷。

void opticalBeginState() { // 呼叫端須已檢查開始條件且持鎖。
  optical_upper_us = optical_lower_us = optical_ignored = 0; // 只改光學欄位。
  optical_upper_seen = optical_lower_seen = false; optical_reason = OPT_NONE; // 明確區分未觸發與零時間。
  ++optical_id; optical_reported = false; optical_state = OPT_WAIT_UP; // 等上方沒有倒數。
} // 結束獨立等待初始化。

void ARDUINO_ISR_ATTR opticalUpperEdge(uint32_t now) { // 呼叫端已確認進入電位並持ISR鎖。
  if (optical_state == OPT_WAIT_UP) { optical_upper_us = now; optical_upper_seen = true; optical_state = OPT_WAIT_DN; } // 首次上緣才開始計時。
  else if (optical_state == OPT_WAIT_DN) ++optical_ignored; // 重複上緣不覆寫起點。
} // 結束上緣處理。

void ARDUINO_ISR_ATTR opticalLowerEdge(uint32_t now) { // 不做Serial、WiFi、I2C或機構控制。
  if (optical_state == OPT_WAIT_UP) { optical_lower_us = now; optical_lower_seen = true; optical_reason = OPT_LOWER_FIRST; optical_state = OPT_FAULT; return; } // 下方先到無速度。
  if (optical_state != OPT_WAIT_DN) return; // 終值不被重複觸發改寫。
  optical_lower_us = now; optical_lower_seen = true; // 可疑到達時間仍留存。
  const uint32_t dt = now - optical_upper_us; // 無號差處理微秒溢位。
  if (dt < OPTICAL_MIN_US) { optical_reason = OPT_TOO_FAST; optical_state = OPT_FAULT; } // 過短不假造可信速度。
  else if (dt > OPTICAL_MAX_US) { optical_reason = OPT_LOWER_TIMEOUT; optical_state = OPT_FAULT; } // 下方超過三十秒中止。
  else { optical_reason = OPT_NONE; optical_state = OPT_DONE; } // 有效上至下完成，不自動重啟。
} // 結束下緣處理。

void opticalTick(uint32_t now) { // 呼叫端持鎖，只管上方已到後的期限。
  if (optical_state == OPT_WAIT_DN && static_cast<uint32_t>(now - optical_upper_us) > OPTICAL_MAX_US) { optical_reason = OPT_LOWER_TIMEOUT; optical_state = OPT_FAULT; } // WAIT_UP可持續等待。
} // 結束期限檢查。

void opticalStopState() { // STOP只取消正在等待或計時的光學輪次。
  if (optical_state == OPT_WAIT_UP || optical_state == OPT_WAIT_DN) { optical_reason = OPT_USER_STOP; optical_state = OPT_FAULT; } // 已完成資料不被STOP改寫。
} // 結束光學取消。

void opticalClearState() { // 僅在DONE或FAULT時由呼叫端持鎖執行。
  optical_state = OPT_OFF; optical_reason = OPT_NONE; optical_upper_seen = optical_lower_seen = false; // 不改一般trial或校正。
  optical_upper_us = optical_lower_us = optical_ignored = 0; optical_reported = false; // 保留光學ID累計。
} // 結束結果清除。
