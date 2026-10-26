#pragma once // 僅納入一次網路輔助函式。
#include <WiFi.h> // 使用ESP32核心內建STA功能，沒有HTTP或遠端控制。
#include <string.h> // 只檢查設定是否存在，不列印憑證。
#if __has_include("wifi_secrets.h") // 本機私密檔不納入Git。
#include "wifi_secrets.h" // 憑證只提供給本機編譯，禁止寫入日誌。
#endif // 未提供私密檔時保持網路停用。
#ifndef IRT_WIFI_ENABLED // 可在沒有憑證的公開來源中正常編譯。
#define IRT_WIFI_ENABLED 0 // 預設不啟用無線電。
#endif // 結束啟用開關預設。
#ifndef IRT_WIFI_SSID // 未設定網名不嘗試連線。
#define IRT_WIFI_SSID "" // 不自動沿用NVS中的其他網路。
#endif // 結束網名預設。
#ifndef IRT_WIFI_PASSWORD // 空密碼僅用於明確設定的開放網路。
#define IRT_WIFI_PASSWORD "" // 不公開使用者實際密碼。
#endif // 結束密碼預設。

enum WifiPhase : uint8_t { NET_DISABLED, NET_WAIT, NET_CONNECTING, NET_CONNECTED, NET_SUSPENDED, NET_FAILED }; // 僅為連線狀態，非量測驗收。
const uint32_t WIFI_POLL_MS = 250, WIFI_TIMEOUT_MS = 15000, WIFI_RETRY_MS = 30000; // 有界待機輪詢、連線期限及重試間隔。
WifiPhase wifi_phase = NET_DISABLED; // 保存主迴圈更新的狀態。
bool wifi_user_enabled = false, wifi_radio_started = false, wifi_poll_seen = false, wifi_retry_seen = false; // 區分使用者意圖、無線電及時間有效性。
uint32_t wifi_poll_ms = 0, wifi_attempt_ms = 0, wifi_retry_ms = 0; // 使用無號差值處理millis溢位。
uint8_t wifi_ip[4] = {0, 0, 0, 0}; // 只快取位址，不快取或輸出密碼。
bool wifi_ip_valid = false; // 未連線或掉線時不得顯示舊IP為目前有效。

bool wifiConfigured() { // 檢查本機設定，不要求所有公開checkout具備憑證。
  return IRT_WIFI_ENABLED && strlen(IRT_WIFI_SSID) != 0; // 是否能上網仍待實機驗證。
} // 結束設定檢查。

const char *wifiPhaseName() { // 使用固定名稱避免動態字串。
  static const char *names[] = {"DISABLED", "WAIT", "CONNECTING", "CONNECTED", "SUSPENDED", "FAILED"}; // 不包含SSID或密碼。
  return names[wifi_phase]; // 列舉值僅由本檔更新。
} // 結束網路狀態名稱。

void printWifiStatus() { // 只列印主迴圈快取，活動記錄不呼叫。
  Serial.printf("# WIFI state=%s requested=%d radio=%d IP=", wifiPhaseName(), wifi_user_enabled, wifi_radio_started); // 不在查詢時觸發掃描或重連。
  if (wifi_ip_valid) Serial.printf("%u.%u.%u.%u", wifi_ip[0], wifi_ip[1], wifi_ip[2], wifi_ip[3]); else Serial.print("NA"); // 沒有有效位址時明確顯示未知。
  Serial.println("; STA only; radio paused for recording/calibration; not timing-validated"); // 連線不表示量測或網際網路已驗收。
} // 結束網路快取輸出。

void initializeWifi() { // 只設定策略，實際連線留給待機服務。
  WiFi.persistent(false); WiFi.setAutoReconnect(false); // 憑證留RAM且由本程式控制重試，不自動寫NVS。
  wifi_user_enabled = wifiConfigured(); wifi_phase = wifi_user_enabled ? NET_WAIT : NET_DISABLED; // 沒有私密檔保持原本離線量測路徑。
  wifi_radio_started = wifi_poll_seen = wifi_retry_seen = wifi_ip_valid = false; // 初始化快取，不改感測校正與試驗資料。
} // 結束網路策略初始化。

bool pauseWifiForMeasurement() { // 僅在ARM或校正開始前、尚未計時時呼叫。
  if (wifi_radio_started && !WiFi.mode(WIFI_OFF)) { // 停用失敗時保留radio標記，不能冒稱已安靜。
    wifi_phase = NET_FAILED; wifi_ip_valid = false; wifi_retry_seen = true; wifi_retry_ms = millis(); // 待機限速處理停止失敗。
    Serial.println("# WIFI stop failed; measurement start rejected"); return false; // 不帶著未停用的無線電開始記錄。
  } // 結束無線電停止檢查。
  wifi_radio_started = false; wifi_ip_valid = false; wifi_retry_seen = true; wifi_retry_ms = millis(); // 清舊IP並留下恢復重試間隔。
  wifi_phase = wifi_user_enabled && wifiConfigured() ? NET_SUSPENDED : NET_DISABLED; // 保留使用者ON意圖，但活動期間不重連。
  return true; // 無線電關閉不代表供電、熱或時序已實機穩定。
} // 結束量測前網路暫停。

bool setWifiEnabled(bool enabled) { // 呼叫端只在IDLE且無校正時接受命令。
  if (enabled && !wifiConfigured()) { Serial.println("# WIFI not configured; local settings required"); return false; } // 不印出缺失或已提供的憑證。
  if (!enabled) { wifi_user_enabled = false; return pauseWifiForMeasurement(); } // 明確OFF後不再自動重試。
  wifi_user_enabled = true; wifi_retry_seen = false; wifi_poll_seen = false; // ON只安排下一輪待機處理，不在命令內等待連線。
  if (!wifi_radio_started) wifi_phase = NET_WAIT; // 已連線時不重複啟動驅動。
  return true; // 是否取得IP由後續快取狀態提供。
} // 結束使用者網路開關。

void serviceWifi(bool allowed) { // allowed由量測狀態與校正狀態共同決定。
  if (!allowed || !wifi_user_enabled || !wifiConfigured()) return; // 活動、校正、DONE與FAULT皆不碰網路API。
  const uint32_t now = millis(); // 不在中斷中呼叫WiFi。
  if (wifi_poll_seen && static_cast<uint32_t>(now - wifi_poll_ms) < WIFI_POLL_MS) return; // 限制待機輪詢頻率。
  wifi_poll_seen = true; wifi_poll_ms = now; // 零時間也是有效的輪詢時間。
  if (wifi_radio_started && wifi_phase == NET_FAILED) { // 停止失敗採限速重試，不讓每圈卡住感測器。
    if (static_cast<uint32_t>(now - wifi_retry_ms) >= WIFI_RETRY_MS && pauseWifiForMeasurement()) wifi_phase = NET_WAIT; // 成功停用後再等待重試間隔。
    return; // 不在同一圈反覆啟停無線電。
  } // 結束停止失敗處理。
  if (wifi_radio_started) { // 只有待機時讀取核心的連線狀態。
    const wl_status_t status = WiFi.status(); // 此狀態不證明DNS或網際網路可達。
    if (status == WL_CONNECTED) { // 取得有效區網位址才標示已連線。
      const IPAddress ip = WiFi.localIP(); // 僅待機更新IP快取。
      bool valid = false, changed = !wifi_ip_valid; // 不把零IP當成連線成功。
      for (uint8_t i = 0; i < 4; ++i) { valid = valid || ip[i] != 0; changed = changed || wifi_ip[i] != ip[i]; } // 判斷新位址而不建立動態字串。
      if (valid) { // DHCP尚未有位址時繼續使用原連線期限。
        for (uint8_t i = 0; i < 4; ++i) wifi_ip[i] = ip[i]; // 保存本次有效位址。
        wifi_ip_valid = true; wifi_phase = NET_CONNECTED; // 清楚區分連線與等待。
        if (changed) printWifiStatus(); // 僅狀態或IP變更時通知，不連續洗版。
        return; // 不用阻塞等待函式。
      } // 結束有效IP處理。
    } // 結束已連線判斷。
    if (wifi_phase == NET_CONNECTED || static_cast<uint32_t>(now - wifi_attempt_ms) >= WIFI_TIMEOUT_MS) { // 掉線或逾時均先停止，再限速重連。
      if (pauseWifiForMeasurement()) { wifi_phase = NET_WAIT; Serial.println("# WIFI unavailable; idle retry scheduled"); } // 不更動RAW、校正、FIFO或GPIO。
    } // 結束待機失聯處理。
    return; // 連線進行中不使用while或delay等待。
  } // 結束已啟動驅動處理。
  if (wifi_retry_seen && static_cast<uint32_t>(now - wifi_retry_ms) < WIFI_RETRY_MS) return; // 沒有AP也不持續快速重試。
  wifi_radio_started = true; // 啟動前先保守標記可能開啟，部分失敗亦必須成功關閉後才能量測。
  if (!WiFi.mode(WIFI_STA)) { // 只作為用戶指定網路的station，不建立AP。
    wifi_phase = NET_FAILED; wifi_retry_seen = true; wifi_retry_ms = now; Serial.println("# WIFI start failed; idle retry scheduled"); return; // 啟動失敗也保留感測主迴圈。
  } // 結束STA啟動檢查。
  WiFi.setAutoReconnect(false); // 再確認不由事件回呼自動重連。
  wifi_attempt_ms = now; wifi_phase = NET_CONNECTING; wifi_ip_valid = false; // 連線期限只由待機服務檢查。
  WiFi.begin(IRT_WIFI_SSID, IRT_WIFI_PASSWORD); // 不輸出網名或密碼，不呼叫waitForConnectResult。
  Serial.println("# WIFI connecting in background; credentials not logged"); // 僅表示已提出連線要求。
} // 結束待機網路服務。
