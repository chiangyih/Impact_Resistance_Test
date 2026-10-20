// 04-HX711-test.ino：只使用 HX711 與 OLED 的簡易接線測試程式，不是正式衝擊量測程式。
// 本版 HX711 SCK 接 GPIO32、DATA 接 GPIO33，與第三版的兩腳設定相反，切換版本前必須斷電核對。
// HX711 VIN 與 VIO 都接同一個 ESP32 3V3，GND 共地；荷重元使用 E+、E-、A+、A-。
// OLED 沿用已正常顯示的 SH1106 一百二十八乘六十四設定，VCC 接 3V3、GND 共地。
// OLED SDA 接 GPIO21、SCK 即 SCL 接 GPIO22，位址自動搜尋十六進位 0x3C 或 0x3D。
// 本版不啟用 ADXL375、Tracker Sensor、繼電器、電磁鐵或無線通訊。
// 顯示有正負號的二十四位元原始計數，不去皮、不濾波、不換算公斤或牛頓，也不掩蓋滿量程。
// 採資料就緒才讀取的有界時序，每次固定二十五個脈衝，下一筆維持Ａ通道增益一百二十八。
// 讀取脈衝高低各保留兩微秒；開機只重置一次，不在故障時反覆自動重置。
// 編譯、燒錄及實際接回 HX711 後的驗證狀態請見 README 與交接紀錄，不以編譯成功代替實測。

#include <Arduino.h> // 載入 ESP32 的 Arduino 基本型別、腳位與計時功能。
#include <Wire.h> // 載入 OLED 使用的硬體 I²C 功能。
#include <U8g2lib.h> // 載入 SH1106 OLED 字型與完整畫面緩衝功能。

const uint8_t HX711_SCK_PIN = 32; // 依本次實際接法將 HX711 SCK 設為 GPIO32。
const uint8_t HX711_DATA_PIN = 33; // 依本次實際接法將 HX711 DATA 設為 GPIO33。
const uint8_t OLED_SDA_PIN = 21; // 沿用 OLED SDA 接 GPIO21。
const uint8_t OLED_SCL_PIN = 22; // 沿用 OLED SCK 即 SCL 接 GPIO22。
const uint32_t I2C_CLOCK_HZ = 100000; // 沿用已正常顯示的一百千赫 I²C 時脈。
const uint32_t HX711_TIMEOUT_MS = 1000; // 一秒內沒有首筆資料或最近資料超時即提出警示。
const uint32_t SCREEN_INTERVAL_MS = 250; // 每二百五十毫秒更新 OLED，不保證保留每一筆轉換。
const uint32_t SERIAL_INTERVAL_MS = 1000; // 每秒輸出一次同樣的診斷資訊。
const uint32_t OLED_RETRY_MS = 3000; // OLED 未連線時每三秒重試初始化。

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE, OLED_SCL_PIN, OLED_SDA_PIN); // 沿用 SH1106 方向與硬體 I²C。
portMUX_TYPE hx711_mux = portMUX_INITIALIZER_UNLOCKED; // 保護短暫 HX711 時脈，避免本核心中斷拉長高電位。
uint8_t oled_address = 0; // 保存找到的 OLED 七位元位址，零代表尚未找到。
bool hx711_seen = false; // 記錄是否取得過讀後 DATA 回高的完整資料框。
bool last_frame_ok = true; // 保存最近一次資料框的讀後 DATA 檢查結果，不代表荷重元已校正。
uint32_t hx711_bits = 0; // 保存最近一筆通過資料框檢查的二十四位元原始位元。
int32_t hx711_raw = 0; // 保存最近一筆有正負號的原始計數。
uint32_t received_count = 0; // 累計通過資料框檢查的筆數，包含滿量程資料。
uint32_t frame_error_count = 0; // 累計讀後 DATA 未回高的資料框錯誤次數。
uint32_t hx711_start_ms = 0; // 保存開機重置後的起始時間，供首筆等待逾時判斷。
uint32_t last_hx711_ms = 0; // 保存最近一次通過資料框檢查的接收時間。
uint32_t last_interval_ms = 0; // 保存最近兩次通過資料框檢查的接收時間差。
uint32_t last_screen_ms = 0; // 保存 OLED 上次更新的排程時間。
uint32_t last_serial_ms = 0; // 保存序列埠上次更新的排程時間。
uint32_t last_oled_retry_ms = 0; // 保存 OLED 上次重新搜尋的時間。

int32_t decodeHx711(uint32_t bits); // 宣告二十四位元補數轉換函式，與實際接線無關。
bool i2cResponds(uint8_t address); // 宣告檢查指定 I²C 位址是否回應的函式。
void initializeOled(); // 宣告沿用 SH1106 設定的 OLED 初始化函式。
void pollHx711(); // 宣告不等待 DATA 就緒的 HX711 輪詢函式。
const char *hx711Status(); // 宣告產生首筆等待、逾時、框錯誤及滿量程狀態的函式。
void updateOled(); // 宣告更新八行診斷資料的函式。
void printDiagnostics(); // 宣告輸出原始值及接收統計的函式。

void setup() { // 開機時只執行一次初始化。
  pinMode(HX711_DATA_PIN, INPUT_PULLUP); // 以弱上拉讓斷開 DATA 時保持未就緒，不能用來代替 VIO 供電。
  digitalWrite(HX711_SCK_PIN, LOW); // 先設定時脈輸出暫存值為低，避免切成輸出時產生多餘高電位。
  pinMode(HX711_SCK_PIN, OUTPUT); // GPIO32 只作為 HX711 時脈輸出。
  Serial.begin(115200); // 沿用每秒十一萬五千二百位元的序列埠速率。
  portENTER_CRITICAL(&hx711_mux); // 保護這次刻意執行的上電重置脈衝。
  digitalWrite(HX711_SCK_PIN, HIGH); // 將 HX711 時脈拉高以開始一次性的重置程序。
  delayMicroseconds(80); // 刻意超過六十微秒，讓 HX711 進入斷電狀態再重新啟動。
  digitalWrite(HX711_SCK_PIN, LOW); // 恢復低電位後重置，預設為Ａ通道增益一百二十八。
  portEXIT_CRITICAL(&hx711_mux); // 立即恢復本核心中斷與排程。
  hx711_start_ms = millis(); // 首筆資料可能需要約四百毫秒穩定，不阻塞等待。
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN); // 在 GPIO21 與 GPIO22 啟動 OLED 匯流排。
  Wire.setClock(I2C_CLOCK_HZ); // 使用既有 OLED 已正常工作的匯流排速度。
  Wire.setTimeOut(25); // 限制單次 I²C 通訊等待，避免 OLED 斷線時長期阻塞。
  initializeOled(); // 搜尋常用位址並送出 SH1106 初始化指令。
  Serial.println("\n=== 04-HX711-test HX711 OLED 簡易測試程式 ==="); // 印出可辨識第四版的啟動標題。
  Serial.println("HX711 SCK=GPIO32 DATA=GPIO33 A128；OLED SH1106 SDA=21 SCL=22。"); // 明確印出與第三版相反的 HX711 接腳。
  Serial.println("VIN、VIO 均須為同一個 3V3；未校正，不代表公斤或牛頓，接回感測器後仍待實測。"); // 提醒供電與量測限制。
  pollHx711(); // 如果首筆已就緒便讀取，否則留待主迴圈處理。
  updateOled(); // 顯示第一頁資料或首筆等待狀態。
  printDiagnostics(); // 輸出第一筆診斷資料，無 OLED 時也可排查。
} // 結束開機初始化。

void loop() { // 持續讀取 HX711，不啟動其他感測器。
  pollHx711(); // 每輪檢查資料是否就緒，不使用無期限等待迴圈。
  const uint32_t now_ms = millis(); // 取得本輪排程時間，使用無號差值處理計時溢位。
  if (oled_address == 0 && now_ms - last_oled_retry_ms >= OLED_RETRY_MS) { // OLED 缺席時定期重新搜尋。
    last_oled_retry_ms = now_ms; // 記錄本次搜尋時間。
    initializeOled(); // 嘗試恢復顯示但不重置 HX711。
  } // 結束 OLED 重試判斷。
  if (now_ms - last_screen_ms >= SCREEN_INTERVAL_MS) { // 到達顯示更新間隔時更新畫面。
    last_screen_ms = now_ms; // 記錄本次顯示排程時間。
    updateOled(); // 將最近資料、狀態與計數畫在八行畫面。
  } // 結束顯示排程。
  if (now_ms - last_serial_ms >= SERIAL_INTERVAL_MS) { // 每秒輸出一次診斷快照。
    last_serial_ms = now_ms; // 記錄本次輸出排程時間。
    printDiagnostics(); // 輸出原始位元、接收數及資料年齡。
  } // 結束序列埠輸出排程。
  delay(1); // 讓系統排程器有機會工作，不以忙碌等待阻塞。
} // 結束主迴圈。

int32_t decodeHx711(uint32_t bits) { // 將二十四位元二補數轉成有正負號的三十二位元整數。
  bits &= 0x00FFFFFFUL; // 只保留有效二十四位元，避免高位元干擾。
  return static_cast<int32_t>(bits) - ((bits & 0x00800000UL) != 0 ? 16777216L : 0L); // 負數減去二的二十四次方，避免無號轉負數的實作差異。
} // 結束補數轉換。

bool i2cResponds(uint8_t address) { // 只檢查 OLED 候選位址，不初始化其他 I²C 感測器。
  Wire.beginTransmission(address); // 對指定七位元位址開始通訊。
  return Wire.endTransmission() == 0; // 收到確認回應才視為目前有裝置。
} // 結束位址回應檢查。

void initializeOled() { // 搜尋十六進位 0x3C 或 0x3D 後初始化既有 OLED。
  oled_address = i2cResponds(0x3C) ? 0x3C : (i2cResponds(0x3D) ? 0x3D : 0); // 沿用先前已成功的候選位址順序。
  if (oled_address == 0) { // OLED 不在匯流排時不妨礙 HX711 診斷。
    Serial.println("[OLED] 未找到 0x3C/0x3D，保留序列埠診斷並每三秒重試。"); // 明確回報顯示器缺席。
    return; // 不對不存在的顯示器送初始化資料。
  } // 結束 OLED 缺席處理。
  oled.setI2CAddress(oled_address << 1); // U8g2 要求將七位元位址左移一位。
  oled.setBusClock(I2C_CLOCK_HZ); // 保持已確認的匯流排速度。
  oled.begin(); // 送出 SH1106 初始化命令，但此呼叫不能驗證實際畫面。
  oled.setPowerSave(0); // 明確開啟面板顯示。
  oled.setContrast(255); // 沿用既有正常畫面的對比設定。
  oled.setFont(u8g2_font_5x7_tr); // 沿用七像素高字型，八行各占八像素。
  oled.setFontPosBaseline(); // 使用基線座標定位文字。
  Wire.setTimeOut(25); // 在 U8g2 初始化後維持 I²C 逾時設定。
  Serial.printf("[OLED] ACK=0x%02X SH1106 128x64；第四版畫面仍待目視確認。\n", oled_address); // 不把位址回應當作畫面成功。
} // 結束 OLED 初始化。

void pollHx711() { // 只在 DATA 已低時讀一筆，不等待感測器轉換完成。
  if (digitalRead(HX711_DATA_PIN) != LOW) return; // 未就緒或 DATA 未接時立即返回。
  uint32_t bits = 0; // 建立本輪二十四位元資料容器。
  bool returned_high = false; // 記錄第二十五個脈衝後 DATA 是否回高。
  portENTER_CRITICAL(&hx711_mux); // 只在固定長度脈衝期間避免本核心中斷拉長時脈。
  const bool ready = digitalRead(HX711_DATA_PIN) == LOW; // 在保護區內再次確認資料就緒。
  if (ready) { // DATA 仍為低才進行完整讀取。
    delayMicroseconds(2); // 保留 DATA 下降到第一個時脈上升的間隔。
    for (uint8_t bit = 0; bit < 24; ++bit) { // 固定讀取二十四位元，由最高位元開始。
      digitalWrite(HX711_SCK_PIN, HIGH); // 送出一個資料位元的上升緣。
      delayMicroseconds(2); // 等待 HX711 更新輸出，維持明確的高電位時間。
      bits = (bits << 1) | (digitalRead(HX711_DATA_PIN) == HIGH ? 1UL : 0UL); // 依序收進一個資料位元。
      digitalWrite(HX711_SCK_PIN, LOW); // 每一位元立即將時脈拉低。
      delayMicroseconds(2); // 保留明確的低電位時間。
    } // 結束二十四個資料脈衝。
    digitalWrite(HX711_SCK_PIN, HIGH); // 第二十五個脈衝選擇下一筆為Ａ通道增益一百二十八。
    delayMicroseconds(2); // 增益脈衝也保留兩微秒高電位，不省略延遲。
    digitalWrite(HX711_SCK_PIN, LOW); // 保持空閒時脈為低，避免誤進入斷電模式。
    delayMicroseconds(2); // 保留低電位時間後再核對 DATA。
    returned_high = digitalRead(HX711_DATA_PIN) == HIGH; // 檢查資料表規定的讀後 DATA 回高。
  } // 結束固定長度讀取。
  portEXIT_CRITICAL(&hx711_mux); // 無論是否就緒都立即離開保護區。
  if (!ready) return; // 就緒狀態已改變時不計入任何新資料。
  last_frame_ok = returned_high; // 保存最近一次資料框握手結果。
  if (!returned_high) { // DATA 未回高可能是卡低、錯接或供電不完整。
    ++frame_error_count; // 累計資料框錯誤但不將全零或錯框當作有效計數。
    return; // 不覆寫上次原始值與接收時間。
  } // 結束資料框錯誤處理。
  const uint32_t received_ms = millis(); // 在脈衝完成後記錄本筆接收時間。
  if (hx711_seen) last_interval_ms = received_ms - last_hx711_ms; // 第二筆起記錄實際接收間隔，不宣稱等於晶片精準取樣率。
  hx711_bits = bits; // 保存通過框檢查的原始位元，滿量程仍如實保留。
  hx711_raw = decodeHx711(bits); // 使用有界補數轉換，保留真正的正負極限。
  last_hx711_ms = received_ms; // 更新最近接收時間以計算資料年齡。
  hx711_seen = true; // 標記已有資料，但不代表荷重元類比接法正確。
  ++received_count; // 增加通過框檢查的接收筆數。
} // 結束 HX711 輪詢。

const char *hx711Status() { // 依握手、首筆等待、資料年齡與滿量程產生狀態。
  if (!last_frame_ok) return "LINK_ERR"; // 最近一次讀後 DATA 未回高時優先標示連線資料框異常。
  if (!hx711_seen) return millis() - hx711_start_ms >= HX711_TIMEOUT_MS ? "TIMEOUT" : "WAIT"; // 首筆超過一秒未到就明確報逾時。
  if (millis() - last_hx711_ms >= HX711_TIMEOUT_MS) return "STALE"; // 有過資料但超過一秒未更新時保留快取並標記過期。
  if (hx711_raw == -8388608L || hx711_raw == 8388607L) return "SAT"; // 正負滿量程均標示飽和，不以濾波掩蓋。
  return "OK"; // 僅表示近期收到未滿量程且握手通過的框，不表示校正或電氣驗證完成。
} // 結束 HX711 狀態判斷。

void updateOled() { // 將 HX711 專用診斷排成與既有 OLED 相容的八行。
  if (oled_address == 0) return; // 沒有 OLED 時仍繼續讀值與序列輸出。
  if (!i2cResponds(oled_address)) { // 每次更新前先檢查 OLED 是否還有回應。
    oled_address = 0; // 標記失聯，由主迴圈安排重試。
    return; // 本次不送出顯示緩衝。
  } // 結束 OLED 斷線處理。
  oled.clearBuffer(); // 清除上一頁，避免數字長度變短時殘留。
  oled.setCursor(0, 7); // 定位第一行基線。
  oled.print("04 HX711 TEST"); // 顯示第四版簡易測試標題。
  oled.setCursor(0, 15); // 定位第二行基線。
  oled.print("A128 SCK32 DATA33"); // 顯示本版實際讀取接腳以避免與第三版混用。
  oled.setCursor(0, 23); // 定位第三行基線。
  if (hx711_seen) oled.printf("RAW:%ld", static_cast<long>(hx711_raw)); else oled.print("RAW: --"); // 顯示有正負號的計數，不以零代替未收到資料。
  oled.setCursor(0, 31); // 定位第四行基線。
  if (hx711_seen) oled.printf("HEX:%06lX", static_cast<unsigned long>(hx711_bits)); else oled.print("HEX: ------"); // 顯示六位十六進位原始位元供排查。
  oled.setCursor(0, 39); // 定位第五行基線。
  oled.printf("HX:%s D:%d", hx711Status(), digitalRead(HX711_DATA_PIN)); // 顯示狀態及此刻 DATA 數位電位。
  oled.setCursor(0, 47); // 定位第六行基線。
  if (hx711_seen) oled.printf("AGE:%lums", static_cast<unsigned long>(millis() - last_hx711_ms)); else oled.print("AGE: --"); // 顯示最近有效框距今多久，舊值不冒充即時值。
  oled.setCursor(0, 55); // 定位第七行基線。
  oled.printf("N:%lu E:%lu", static_cast<unsigned long>(received_count), static_cast<unsigned long>(frame_error_count)); // 顯示接收框數與錯框數，最大各十位時仍能放入畫面。
  oled.setCursor(0, 63); // 定位第八行基線。
  if (received_count >= 2) oled.printf("DT:%lums OLED:%02X", static_cast<unsigned long>(last_interval_ms), oled_address); else oled.printf("DT:-- OLED:%02X", oled_address); // 顯示最近接收間隔及 OLED 位址。
  oled.sendBuffer(); // 送出完整畫面，這段通訊會影響輪詢間隔，不能當正式高速擷取。
} // 結束 OLED 更新。

void printDiagnostics() { // 將同一組診斷資料輸出至序列埠以利記錄。
  Serial.printf("[HX711] t=%lu SCK=32 DATA=33 A128 ", static_cast<unsigned long>(millis())); // 印出時間與第四版接腳配置。
  if (hx711_seen) Serial.printf("RAW=%ld HEX=%06lX AGE=%lums ", static_cast<long>(hx711_raw), static_cast<unsigned long>(hx711_bits), static_cast<unsigned long>(millis() - last_hx711_ms)); else Serial.print("RAW=-- HEX=------ AGE=-- "); // 未有資料時明確標為缺值。
  Serial.printf("[%s] DOUT=%d RX=%lu ERR=%lu DT=%lums OLED=0x%02X\n", hx711Status(), digitalRead(HX711_DATA_PIN), static_cast<unsigned long>(received_count), static_cast<unsigned long>(frame_error_count), static_cast<unsigned long>(last_interval_ms), oled_address); // 印出握手狀態、框計數與間隔。
} // 結束序列埠診斷輸出。
