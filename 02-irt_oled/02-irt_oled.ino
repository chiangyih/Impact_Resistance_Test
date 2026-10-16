// 02-irt_oled.ino：以 01-irt 為基礎的第二版 OLED 簡易測試程式。 // 說明程式名稱與用途。
// OLED 使用 SH1106 128×64 設定，使用者已確認此診斷畫面正常且無缺邊。 // 說明目前採用的顯示設定。
// 若更換為 SSD1306 128×64 模組，可將下方 OLED_USE_SH1106 設為 0。 // 說明替代顯示控制器的切換方法。
// OLED VCC 接 ESP32 3V3、GND 共地；本次已依使用者回報確認改接。 // 記錄目前供電接法。
// OLED 的 I²C 回應只能證明通訊，實際畫面仍須目視驗證。 // 區分通訊結果與畫面驗證。
// 實際編譯、燒錄及感測器驗證狀態請見 README 與交接紀錄。 // 指向持續更新的驗證結果。
// 本程式顯示三軸加速度、未校正的荷重原始值與兩路光閘電位。 // 說明輸出資料。
// OLED 與 ADXL375 共用 SDA=GPIO21、SCL=GPIO22 的 I²C 匯流排。 // 說明共用接線。
// GPIO34 與 GPIO35 需外接上拉電阻；本程式不控制電磁鐵。 // 說明光閘接線與測試範圍。

#include <Arduino.h> // 載入 ESP32 使用的 Arduino 基本功能。
#include <Wire.h> // 載入 OLED 與 ADXL375 共用的 I²C 通訊功能。
#include <Adafruit_Sensor.h> // 載入標準重力加速度常數與感測器基礎型別。
#include <Adafruit_ADXL375.h> // 載入 Adafruit ADXL375 加速度計驅動。
#include <Adafruit_HX711.h> // 載入 Adafruit HX711 類比數位轉換器驅動。
#include <U8g2lib.h> // 載入 OLED 畫面緩衝、字型與繪圖功能。

#ifndef OLED_USE_SH1106 // 允許編譯參數覆寫 OLED 控制器選項。
#define OLED_USE_SH1106 1 // 設為一使用已通過完整診斷畫面觀察的 SH1106，設為零使用 SSD1306。
#endif // 結束 OLED 控制器預設值設定。

const uint8_t I2C_SDA_PIN = 21; // ADXL375 SDA 與 OLED SDA 同接 GPIO21。
const uint8_t I2C_SCL_PIN = 22; // ADXL375 SCL 與 OLED SCK 同接 GPIO22。
const uint8_t ADXL375_I2C_ADDRESS = 0x53; // ADXL375 的 SDO 接地時使用此位址。
const uint8_t HX711_DATA_PIN = 32; // HX711 的 DATA 紫線接到 GPIO32。
const uint8_t HX711_SCK_PIN = 33; // HX711 的 SCK 紫線接到 GPIO33。
const uint8_t OS25B10_UPPER_PIN = 34; // 上方 OS25B10 紫線訊號接到 GPIO34。
const uint8_t OS25B10_LOWER_PIN = 35; // 下方 OS25B10 橘線訊號接到 GPIO35。
const uint32_t I2C_CLOCK_HZ = 100000; // 使用一百千赫的共用 I²C 時脈。
const uint32_t SCREEN_INTERVAL_MS = 250; // 每二百五十毫秒更新一次畫面與感測器快照。
const uint32_t SERIAL_INTERVAL_MS = 1000; // 每一秒輸出一次序列埠診斷資料。
const uint32_t HX711_STALE_MS = 1000; // 超過一秒沒有新資料時標示 HX711 資料逾時。
const uint32_t OLED_RETRY_MS = 3000; // OLED 未連線時每三秒重新檢查位址。

#if OLED_USE_SH1106 // 依編譯選項建立 SH1106 的硬體 I²C 顯示物件。
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE, I2C_SCL_PIN, I2C_SDA_PIN); // 使用整頁緩衝與既定腳位。
const char *OLED_DRIVER_NAME = "SH1106"; // 保存目前選用的控制器名稱供序列埠顯示。
#else // 未啟用 SH1106 時使用 SSD1306。
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE, I2C_SCL_PIN, I2C_SDA_PIN); // 使用整頁緩衝與既定腳位。
const char *OLED_DRIVER_NAME = "SSD1306"; // 保存目前選用的控制器名稱供序列埠顯示。
#endif // 結束 OLED 控制器選擇。

Adafruit_ADXL375 adxl375(12345, &Wire); // 建立使用共用 I²C 的 ADXL375 物件。
Adafruit_HX711 hx711(HX711_DATA_PIN, HX711_SCK_PIN); // 建立 HX711 讀取物件。
portMUX_TYPE hx711_mux = portMUX_INITIALIZER_UNLOCKED; // 保護 HX711 時脈脈衝不被中斷拉長。
bool adxl375_ready = false; // 保存 ADXL375 開機初始化是否成功。
bool acceleration_valid = false; // 保存本次三軸讀取是否成功，避免將錯誤當成零。
float acceleration_x = 0.0f; // 保存 X 軸加速度，單位為公尺每平方秒。
float acceleration_y = 0.0f; // 保存 Y 軸加速度，單位為公尺每平方秒。
float acceleration_z = 0.0f; // 保存 Z 軸加速度，單位為公尺每平方秒。
int32_t hx711_raw = 0; // 保存最近一筆 HX711 A 通道增益一百二十八的原始值。
bool hx711_seen = false; // 記錄是否至少收過一筆 HX711 資料。
uint32_t last_hx711_ms = 0; // 記錄最近一筆 HX711 資料的時間。
int upper_level = LOW; // 保存上方光閘的實際數位電位。
int lower_level = LOW; // 保存下方光閘的實際數位電位。
uint8_t oled_address = 0; // 保存 OLED 七位元位址，零表示沒有找到候選裝置。
uint32_t last_screen_ms = 0; // 保存最近一次更新畫面的時間。
uint32_t last_serial_ms = 0; // 保存最近一次輸出序列埠的時間。
uint32_t last_oled_retry_ms = 0; // 保存最近一次嘗試初始化 OLED 的時間。
uint32_t frame_count = 0; // 累計送出畫面緩衝的次數，供診斷程式是否持續執行。

bool i2cResponds(uint8_t address); // 宣告檢查 I²C 裝置回應的函式。
void scanI2C(); // 宣告列出 I²C 位址的函式。
void initializeOled(); // 宣告搜尋並初始化 OLED 的函式。
void pollHx711(); // 宣告只在資料就緒時讀取 HX711 的函式。
const char *hx711Status(); // 宣告產生 HX711 最新狀態文字的函式。
void sampleSensors(); // 宣告更新三軸與兩個光閘快照的函式。
void updateOled(); // 宣告更新 OLED 畫面的函式。
void printSensorData(); // 宣告輸出相同感測器快照到序列埠的函式。

void setup() { // ESP32 開機時執行一次初始化。
  Serial.begin(115200); // 使用每秒十一萬五千二百位元的序列埠速率。
  delay(500); // 等待 USB 序列埠穩定。
  Serial.println("\n=== 02-irt_oled OLED 簡易測試程式 ==="); // 輸出本版韌體識別文字。
  Serial.println("OLED 畫面請目視確認；HX711 為未校正原始值，光閘為實際 HIGH/LOW。"); // 說明資料意義。
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); // 在 GPIO21 與 GPIO22 啟動共用 I²C。
  Wire.setClock(I2C_CLOCK_HZ); // 將 I²C 時脈設定為一百千赫。
  Wire.setTimeOut(25); // 將單次 I²C 通訊逾時設定為二十五毫秒。
  pinMode(OS25B10_UPPER_PIN, INPUT); // 使用上方光閘外接電阻決定電位。
  pinMode(OS25B10_LOWER_PIN, INPUT); // 使用下方光閘外接電阻決定電位。
  hx711.begin(); // 初始化 HX711，預設讀取 A 通道增益一百二十八。
  scanI2C(); // 列出接線後實際回應的 I²C 裝置位址。
  initializeOled(); // 搜尋常見 OLED 位址並初始化選定驅動。
  adxl375_ready = adxl375.begin(ADXL375_I2C_ADDRESS); // 檢查 ADXL375 位址與晶片識別碼。
  if (adxl375_ready) { // ADXL375 初始化成功時設定資料率。
    adxl375.setDataRate(ADXL343_DATARATE_100_HZ); // 使用每秒一百筆的感測器輸出資料率。
  } // 結束加速度計設定。
  Serial.println(adxl375_ready ? "[ADXL375] 通訊成功，位址 0x53" : "[ADXL375] 初始化失敗，檢查供電與 I2C 接線"); // 回報初始化結果。
  pollHx711(); // 若首筆荷重資料已就緒就先讀取。
  sampleSensors(); // 取得第一筆加速度與光閘快照。
  updateOled(); // 顯示第一頁感測器資料或等待狀態。
  printSensorData(); // 輸出第一筆診斷資料。
} // 結束開機初始化。

void loop() { // 持續擷取資料並按週期更新輸出。
  pollHx711(); // 每次主迴圈都檢查 HX711，保存最新一筆原始值。
  const uint32_t now_ms = millis(); // 取得本次主迴圈的時間。
  if (oled_address == 0 && now_ms - last_oled_retry_ms >= OLED_RETRY_MS) { // 未找到 OLED 時定期重試。
    last_oled_retry_ms = now_ms; // 記錄這次 OLED 搜尋時間。
    initializeOled(); // 重新檢查 OLED 是否已接上或恢復供電。
  } // 結束 OLED 重試判斷。
  if (now_ms - last_screen_ms >= SCREEN_INTERVAL_MS) { // 到達畫面更新時間時擷取新快照。
    last_screen_ms = now_ms; // 更新畫面排程時間。
    sampleSensors(); // 更新三軸讀值與兩個光閘電位。
    updateOled(); // 將快照與 HX711 最近讀值顯示在 OLED。
  } // 結束畫面更新排程。
  if (now_ms - last_serial_ms >= SERIAL_INTERVAL_MS) { // 到達診斷輸出時間時回報相同快照。
    last_serial_ms = now_ms; // 更新序列埠排程時間。
    printSensorData(); // 透過序列埠輸出感測器與 OLED 通訊狀態。
  } // 結束序列埠輸出排程。
  delay(1); // 讓 ESP32 排程器有機會處理其他系統工作。
} // 結束主迴圈。

bool i2cResponds(uint8_t address) { // 以裝置回應檢查 I²C 接線。
  Wire.beginTransmission(address); // 對指定的七位元位址開始通訊。
  return Wire.endTransmission() == 0; // 回傳是否收到裝置的確認回應。
} // 結束 I²C 回應檢查。

void scanI2C() { // 在序列埠列出實際有回應的裝置。
  uint8_t count = 0; // 記錄找到的裝置數量。
  for (uint8_t address = 1; address < 127; ++address) { // 逐一掃描可用的七位元位址。
    if (i2cResponds(address)) { // 只輸出有回應的位址。
      Serial.printf("[I2C] ACK 0x%02X\n", address); // 使用十六進位顯示位址。
      ++count; // 累計有回應的裝置。
    } // 結束單一位址判斷。
  } // 結束 I²C 掃描。
  Serial.printf("[I2C] 共 %u 個裝置；ADXL375 預期 0x53，OLED 候選 0x3C/0x3D\n", count); // 提供接線診斷提示。
} // 結束位址掃描。

void initializeOled() { // 搜尋常見位址後啟動所選的 OLED 驅動。
  oled_address = i2cResponds(0x3C) ? 0x3C : (i2cResponds(0x3D) ? 0x3D : 0); // 兩者皆回應時優先使用較低位址。
  if (oled_address == 0) { // 若候選位址都沒有回應就保留其他感測器測試。
    Serial.println("[OLED] 未找到 0x3C/0x3D；三秒後重試，感測器仍持續讀取"); // 回報缺少 OLED 回應。
    return; // 本次不對不存在的顯示器送出初始化命令。
  } // 結束 OLED 缺席處理。
  oled.setI2CAddress(oled_address << 1); // U8g2 要求將七位元位址向左移一位。
  oled.setBusClock(I2C_CLOCK_HZ); // 保持 OLED 與 ADXL375 使用相同的匯流排速度。
  oled.begin(); // 送出選定控制器的初始化指令，此函式不能驗證實際畫面。
  oled.setPowerSave(0); // 明確開啟面板輸出，與成功的專用診斷設定一致。
  oled.setContrast(255); // 使用與成功診斷相同的對比度，方便觀察簡易測試讀值。
  Wire.setTimeOut(25); // 在顯示函式庫初始化匯流排後維持通訊逾時設定。
  oled.setFont(u8g2_font_5x7_tr); // 選用七像素高的英數字型，讓八行文字各占八像素。
  oled.setFontPosBaseline(); // 以字型基線定位八行文字。
  Serial.printf("[OLED] I2C_ACK 0x%02X，驅動 %s 128x64；畫面待目視確認\n", oled_address, OLED_DRIVER_NAME); // 回報位址與驅動假設。
} // 結束 OLED 初始化。

void pollHx711() { // 在資料已就緒時讀取一筆荷重原始值。
  int32_t new_raw = 0; // 建立本次讀值容器。
  portENTER_CRITICAL(&hx711_mux); // 暫停本核心中斷，避免時脈高電位意外延長。
  const bool ready = !hx711.isBusy(); // 在同一保護區內確認 DATA 已拉低。
  if (ready) { // DATA 就緒時才呼叫函式庫，避免等待轉換。
    new_raw = hx711.readChannelRaw(CHAN_A_GAIN_128); // 讀取有正負號的二十四位元 A 通道原始值。
  } // 結束就緒讀取。
  portEXIT_CRITICAL(&hx711_mux); // 立即恢復中斷與排程。
  if (ready) { // 只有取得新資料時才更新快取與時間。
    hx711_raw = new_raw; // 保存本次原始值供畫面與序列埠共用。
    hx711_seen = true; // 標示已取得至少一筆資料。
    last_hx711_ms = millis(); // 記錄最近資料時間供逾時判斷。
  } // 結束最新資料保存。
} // 結束 HX711 輪詢。

const char *hx711Status() { // 將 HX711 資料狀態轉成短字串。
  if (!hx711_seen) { // 尚未收到第一筆資料時顯示等待。
    return "WAIT"; // 表示目前沒有可用原始值。
  } // 結束首筆資料判斷。
  if (millis() - last_hx711_ms >= HX711_STALE_MS) { // 超過一秒未更新時標記快取已過期。
    return "STALE"; // 避免將舊數值誤當成即時數值。
  } // 結束資料新鮮度判斷。
  if (hx711_raw == 8388607 || hx711_raw == -8388608) { // 檢查二十四位元轉換器的正負極限值。
    return "SAT"; // 提示輸入飽和或荷重元接線可能有問題。
  } // 結束飽和判斷。
  return "OK"; // 表示最近一秒內有收到未達極限的原始值。
} // 結束 HX711 狀態文字。

void sampleSensors() { // 擷取同一輪的加速度與光閘電位快照。
  int16_t raw_x = 0, raw_y = 0, raw_z = 0; // 建立三軸原始資料容器。
  acceleration_valid = adxl375_ready && adxl375.getXYZ(raw_x, raw_y, raw_z); // 一次讀取三軸並檢查交易是否成功。
  if (acceleration_valid) { // 成功讀取時才更新三軸數值。
    const float scale = ADXL375_MG2G_MULTIPLIER * SENSORS_GRAVITY_STANDARD; // 以每計數零點零四九重力加速度換算單位。
    acceleration_x = raw_x * scale; // 將 X 軸換算為公尺每平方秒。
    acceleration_y = raw_y * scale; // 將 Y 軸換算為公尺每平方秒。
    acceleration_z = raw_z * scale; // 將 Z 軸換算為公尺每平方秒。
  } // 結束加速度換算。
  upper_level = digitalRead(OS25B10_UPPER_PIN); // 保存上方光閘的實際電位。
  lower_level = digitalRead(OS25B10_LOWER_PIN); // 保存下方光閘的實際電位。
} // 結束感測器快照更新。

void updateOled() { // 將各元件讀值排成八行並送至 OLED。
  if (oled_address == 0) { // 沒有 OLED 時仍可繼續其他測試。
    return; // 略過本次畫面更新。
  } // 結束未連線判斷。
  if (!i2cResponds(oled_address)) { // 每次更新前檢查 OLED 是否仍在匯流排上。
    oled_address = 0; // 標示中斷，讓主迴圈安排重新初始化。
    Serial.println("[OLED] 回應中斷，等待重新連線"); // 輸出顯示器斷線狀態。
    return; // 略過送出本次畫面。
  } // 結束斷線檢查。
  oled.clearBuffer(); // 清除記憶體中的上一個畫面。
  oled.setCursor(0, 7); // 定位第一行的文字基線。
  oled.print("02-irt OLED TEST"); // 顯示簡易測試程式識別文字。
  oled.setCursor(0, 15); // 定位第二行的文字基線。
  oled.print(acceleration_valid ? "ADXL375 m/s^2" : "ADXL375 ERROR"); // 顯示加速度單位或讀取錯誤。
  oled.setCursor(0, 23); // 定位第三行的文字基線。
  if (acceleration_valid) oled.printf("X:%9.2f", acceleration_x); else oled.print("X: --"); // 顯示 X 軸數值或缺值。
  oled.setCursor(0, 31); // 定位第四行的文字基線。
  if (acceleration_valid) oled.printf("Y:%9.2f", acceleration_y); else oled.print("Y: --"); // 顯示 Y 軸數值或缺值。
  oled.setCursor(0, 39); // 定位第五行的文字基線。
  if (acceleration_valid) oled.printf("Z:%9.2f", acceleration_z); else oled.print("Z: --"); // 顯示 Z 軸數值或缺值。
  oled.setCursor(0, 47); // 定位第六行的文字基線。
  if (hx711_seen) oled.printf("HX RAW:%ld", static_cast<long>(hx711_raw)); else oled.print("HX RAW: --"); // 顯示未校正的荷重原始值。
  oled.setCursor(0, 55); // 定位第七行的文字基線。
  oled.printf("UP:%s DN:%s", upper_level == HIGH ? "HIGH" : "LOW", lower_level == HIGH ? "HIGH" : "LOW"); // 同行顯示上下光閘電位。
  oled.setCursor(0, 63); // 定位第八行的文字基線。
  oled.printf("HX:%s OLED:%02X", hx711Status(), oled_address); // 顯示荷重資料狀態與 OLED 位址。
  oled.sendBuffer(); // 將整個畫面緩衝送給 OLED，實際點亮結果仍待目視。
  ++frame_count; // 累計畫面傳送次數。
} // 結束 OLED 畫面更新。

void printSensorData() { // 輸出與畫面共用的數據供燒錄後診斷。
  Serial.printf("[資料] t=%lu ms | ADXL375: ", static_cast<unsigned long>(millis())); // 顯示時間與感測器名稱。
  if (acceleration_valid) { // 本輪三軸讀取成功時輸出數值。
    Serial.printf("X=%.2f Y=%.2f Z=%.2f m/s^2", acceleration_x, acceleration_y, acceleration_z); // 顯示三軸加速度。
  } else { // 本輪讀取失敗時使用明確錯誤文字。
    Serial.print("ERROR"); // 不顯示上次快取以免誤判。
  } // 結束三軸輸出。
  Serial.print(" | HX711 A128_RAW="); // 顯示荷重資料欄位名稱。
  if (hx711_seen) Serial.print(hx711_raw); else Serial.print("--"); // 顯示最近原始值或缺值。
  Serial.printf(" [%s] | OS25B10 UP=%s DOWN=%s", hx711Status(), upper_level == HIGH ? "HIGH" : "LOW", lower_level == HIGH ? "HIGH" : "LOW"); // 輸出荷重狀態與雙光閘電位。
  Serial.printf(" | OLED=0x%02X frames=%lu\n", oled_address, static_cast<unsigned long>(frame_count)); // 顯示 OLED 位址與畫面傳送次數。
} // 結束序列埠診斷輸出。
