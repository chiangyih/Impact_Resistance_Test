// ============================================================================ // 本檔案為 01-irt.ino 第一版簡易測試程式（簡易測試版）。
// 本程式只驗證 ADXL375、HX711 與兩組 OS25B10 是否能由 ESP32 讀到資料。 // 本行說明測試範圍。
// 本程式不包含正式落下測試、速度計算、校正、資料記錄或 Relay 控制。 // 本行說明目前未包含的功能。
// 本程式目前僅完成編譯檢查，尚未在實體 NodeMCU-32S 上實際燒錄驗證。 // 本行明確標示目前驗證狀態。
// 開啟序列埠監控視窗並設定為 115200 baud，即可查看測試結果。 // 本行說明操作方式。
// ============================================================================ // 本行為檔案標題結束線。

#include <Arduino.h> // 載入 Arduino 基本型別、GPIO、時間與 Serial 功能。
#include <Wire.h> // 載入 I²C 通訊功能，供 ADXL375 使用。
#include <Adafruit_Sensor.h> // 載入 Adafruit Unified Sensor 的事件資料型別。
#include <Adafruit_ADXL375.h> // 載入 Adafruit ADXL375 感測器函式庫。
#include <Adafruit_HX711.h> // 載入 Adafruit HX711 ADC 函式庫。

const uint8_t ADXL375_SDA_PIN = 21; // 設定 ADXL375 SDA 接到 NodeMCU-32S GPIO21。
const uint8_t ADXL375_SCL_PIN = 22; // 設定 ADXL375 SCL 接到 NodeMCU-32S GPIO22。
const uint8_t HX711_DATA_PIN = 32; // 設定 HX711 DATA 接到 NodeMCU-32S GPIO32。
const uint8_t HX711_SCK_PIN = 33; // 設定 HX711 SCK 接到 NodeMCU-32S GPIO33。
const uint8_t OS25B10_GATE1_PIN = 34; // 設定 OS25B10 光閘一 Collector 節點接到 GPIO34。
const uint8_t OS25B10_GATE2_PIN = 35; // 設定 OS25B10 光閘二 Collector 節點接到 GPIO35。
const uint8_t ADXL375_I2C_ADDRESS = 0x53; // 設定 SDO 接地時 ADXL375 的預設 I²C 位址。
const uint32_t SERIAL_BAUD_RATE = 115200; // 設定序列埠傳輸速率為 115200 baud。
const uint32_t REPORT_INTERVAL_MS = 500; // 設定每 500 毫秒輸出一次感測器資料。
const uint32_t HX711_READY_TIMEOUT_MS = 1000; // 設定等待 HX711 DATA 就緒的最長時間。

Adafruit_ADXL375 g_adxl375(12345, &Wire); // 建立使用硬體 I²C 的 ADXL375 物件並指定識別碼。
Adafruit_HX711 g_hx711(HX711_DATA_PIN, HX711_SCK_PIN); // 建立使用 GPIO32 與 GPIO33 的 HX711 物件。
bool g_adxl375_ready = false; // 保存 ADXL375 初始化是否成功的狀態。
bool g_hx711_ready = false; // 保存 HX711 初始化是否完成的狀態。
uint32_t g_last_report_ms = 0; // 保存上一次輸出資料的時間戳。

void printI2CScan(); // 宣告 I²C 掃描函式，先確認 ADXL375 是否出現在匯流排上。
bool readHX711RawWithTimeout(int32_t &raw_value); // 宣告具逾時保護的 HX711 原始值讀取函式。
void printSensorValues(); // 宣告感測器資料輸出函式。

void setup() { // Arduino 開機後只執行一次的初始化函式。
  Serial.begin(SERIAL_BAUD_RATE); // 啟動 USB 序列埠，讓 ESP32 回傳測試結果。
  delay(500); // 等待序列埠與 USB 轉換器穩定。
  Serial.println(); // 在序列埠輸出一個空白行，方便閱讀。
  Serial.println("========================================"); // 輸出測試程式分隔線。
  Serial.println("01-irt.ino：第一版簡易測試程式"); // 明確標示本程式是第一版簡易測試程式。
  Serial.println("注意：本程式尚未實際燒錄驗證，以下輸出格式為預期測試結果。"); // 在開機訊息中明確標示尚未完成實機燒錄驗證。
  Serial.println("測試項目：ADXL375、HX711、OS25B10 光閘一與光閘二"); // 輸出本次測試的元件清單。
  Serial.println("腳位：SDA=21、SCL=22、HX711 DATA=32、SCK=33、光閘=34/35"); // 輸出目前採用的 GPIO 配置。
  Serial.println("========================================"); // 輸出測試程式分隔線。
  Wire.begin(ADXL375_SDA_PIN, ADXL375_SCL_PIN); // 以 GPIO21 與 GPIO22 啟動 ESP32 I²C 匯流排。
  Wire.setClock(100000); // 使用 100 kHz 標準 I²C 速度以利第一版接線測試。
  printI2CScan(); // 掃描 I²C 位址並在序列埠顯示找到的裝置。
  g_adxl375_ready = g_adxl375.begin(ADXL375_I2C_ADDRESS); // 嘗試以 0x53 初始化 ADXL375。
  if (g_adxl375_ready) { // 如果 ADXL375 回應正確，就進入成功處理區塊。
    g_adxl375.setDataRate(ADXL343_DATARATE_100_HZ); // 將 ADXL375 資料率設定為 100 Hz 供測試使用。
    Serial.println("[ADXL375] 初始化成功，可讀取 X/Y/Z 加速度。"); // 回報 ADXL375 已正確回應。
  } else { // 如果 ADXL375 沒有回應，就進入失敗處理區塊。
    Serial.println("[ADXL375] 初始化失敗，請檢查 3V3、GND、SDA、SCL、CS 與 SDO 接線。"); // 回報 ADXL375 接線或位址可能有問題。
  } // 結束 ADXL375 初始化結果判斷。
  g_hx711.begin(); // 初始化 HX711 的 DATA 與 SCK 腳位並喚醒晶片。
  g_hx711_ready = true; // 函式庫已完成初始化，實際資料是否就緒交由迴圈測試。
  Serial.println("[HX711] 腳位初始化完成，將以 A 通道增益 128 讀取原始值。"); // 回報 HX711 已開始等待資料。
  pinMode(OS25B10_GATE1_PIN, INPUT); // 設定光閘一 Collector 節點為輸入，使用外接 10 kΩ 上拉。
  pinMode(OS25B10_GATE2_PIN, INPUT); // 設定光閘二 Collector 節點為輸入，使用外接 10 kΩ 上拉。
  Serial.println("[OS25B10] GPIO34/GPIO35 已設定為輸入，請遮擋兩顆光閘觀察 HIGH/LOW 變化。"); // 回報兩組光閘已開始監看。
  Serial.println("開始週期性輸出測試數值；HX711 若未接妥會在 1 秒後顯示逾時。"); // 說明接線錯誤時不會讓程式永久卡住。
} // 結束 setup 初始化函式。

void loop() { // Arduino 會持續重複執行的主迴圈函式。
  const uint32_t now_ms = millis(); // 取得 ESP32 開機至今的毫秒時間。
  if (now_ms - g_last_report_ms >= REPORT_INTERVAL_MS) { // 到達輸出週期時才讀取並輸出一次資料。
    g_last_report_ms = now_ms; // 更新上一次輸出的時間戳。
    printSensorValues(); // 讀取三類元件並把目前結果輸出到序列埠。
  } // 結束週期輸出判斷。
} // 結束 loop 主迴圈函式。

void printI2CScan() { // 執行一次 I²C 匯流排掃描並輸出結果的函式。
  uint8_t found_count = 0; // 建立找到的 I²C 裝置數量計數器。
  Serial.println("[I2C] 開始掃描 GPIO21(SDA) 與 GPIO22(SCL)..."); // 輸出 I²C 掃描開始訊息。
  for (uint8_t address = 1; address < 127; address++) { // 逐一測試合法的 7-bit I²C 位址。
    Wire.beginTransmission(address); // 對目前位址開始一次 I²C 傳輸。
    const uint8_t error_code = Wire.endTransmission(); // 結束傳輸並取得裝置回應狀態碼。
    if (error_code == 0) { // 回應碼為零代表該位址有裝置回應。
      found_count++; // 將找到的裝置數量加一。
      Serial.print("[I2C] 找到裝置位址 0x"); // 輸出找到的 I²C 位址前綴。
      if (address < 16) { // 位址小於 0x10 時補一個前導零以保持格式一致。
        Serial.print("0"); // 輸出 I²C 位址的前導零。
      } // 結束前導零判斷。
      Serial.println(address, HEX); // 以十六進位輸出目前找到的 I²C 位址。
    } // 結束 I²C 裝置回應判斷。
  } // 結束全部 I²C 位址掃描迴圈。
  if (found_count == 0) { // 如果完全沒有找到 I²C 裝置，就輸出檢查提示。
    Serial.println("[I2C] 沒有找到裝置，ADXL375 可能未供電、未共地或 SDA/SCL 接錯。"); // 回報 I²C 匯流排沒有回應。
  } else if (found_count > 0) { // 如果找到至少一個裝置，就繼續提示預期的 ADXL375 位址。
    Serial.print("[I2C] 預期 ADXL375 位址為 0x53，目前請確認掃描結果包含 0x53。"); // 提示使用者核對 ADXL375 位址。
    Serial.println(); // 結束此行 I²C 提示訊息。
  } // 結束 I²C 掃描結果判斷。
} // 結束 I²C 掃描函式。

bool readHX711RawWithTimeout(int32_t &raw_value) { // 讀取 HX711 A 通道原始值並避免未接線時永久等待。
  const uint32_t start_ms = millis(); // 記錄開始等待 HX711 DATA 就緒的時間。
  while (g_hx711.isBusy()) { // HX711 DATA 為 HIGH 時代表轉換尚未完成。
    if (millis() - start_ms >= HX711_READY_TIMEOUT_MS) { // 等待超過設定時間就判定本次讀取逾時。
      return false; // 回傳失敗，讓主程式繼續測試其他元件。
    } // 結束 HX711 逾時判斷。
    delay(1); // 短暫等待後再次檢查 HX711 DATA 狀態。
  } // 結束 HX711 DATA 就緒等待迴圈。
  raw_value = g_hx711.readChannelRaw(CHAN_A_GAIN_128); // 讀取 A 通道增益 128 的 24-bit 原始值。
  return true; // 回傳成功，表示 ESP32 已收到一筆 HX711 數值。
} // 結束 HX711 具逾時讀取函式。

void printSensorValues() { // 讀取並輸出 ADXL375、HX711 與 OS25B10 的目前數值。
  sensors_event_t adxl375_event; // 建立 Adafruit Unified Sensor 使用的加速度事件資料容器。
  int32_t hx711_raw_value = 0; // 建立存放 HX711 A 通道原始值的變數。
  const int gate1_level = digitalRead(OS25B10_GATE1_PIN); // 讀取光閘一 Collector 節點的數位電位。
  const int gate2_level = digitalRead(OS25B10_GATE2_PIN); // 讀取光閘二 Collector 節點的數位電位。
  Serial.print("[資料] t="); // 輸出資料列的時間欄位前綴。
  Serial.print(millis()); // 輸出 ESP32 開機後經過的毫秒數。
  Serial.print(" ms | "); // 分隔時間欄位與下一個感測器欄位。
  Serial.print("ADXL375: "); // 輸出 ADXL375 欄位名稱。
  if (g_adxl375_ready) { // 只有初始化成功時才向 ADXL375 請求加速度資料。
    g_adxl375.getEvent(&adxl375_event); // 從 ADXL375 讀取最新的 X、Y、Z 加速度事件。
    Serial.print("X="); // 輸出 X 軸欄位名稱。
    Serial.print(adxl375_event.acceleration.x, 3); // 輸出 X 軸加速度，單位為 m/s^2。
    Serial.print(" Y="); // 輸出 Y 軸欄位名稱。
    Serial.print(adxl375_event.acceleration.y, 3); // 輸出 Y 軸加速度，單位為 m/s^2。
    Serial.print(" Z="); // 輸出 Z 軸欄位名稱。
    Serial.print(adxl375_event.acceleration.z, 3); // 輸出 Z 軸加速度，單位為 m/s^2。
    Serial.print(" m/s^2"); // 輸出 ADXL375 加速度的單位。
  } else { // ADXL375 初始化失敗時輸出錯誤狀態而不讀取資料。
    Serial.print("ERROR(未偵測到)"); // 回報 ADXL375 沒有可讀資料。
  } // 結束 ADXL375 資料輸出判斷。
  Serial.print(" | HX711: "); // 輸出 HX711 欄位名稱。
  if (g_hx711_ready && readHX711RawWithTimeout(hx711_raw_value)) { // 嘗試在逾時保護下讀取 HX711 原始值。
    Serial.print("A128_RAW="); // 輸出 HX711 A 通道增益 128 原始值欄位名稱。
    Serial.print(hx711_raw_value); // 輸出 ESP32 從 HX711 收到的原始 ADC 數值。
  } else { // HX711 未就緒或等待逾時時輸出錯誤狀態。
    Serial.print("ERROR(未就緒或 DATA 逾時)"); // 提示檢查 HX711 供電、共地、DATA、SCK 與荷重元接線。
  } // 結束 HX711 資料輸出判斷。
  Serial.print(" | OS25B10-1: "); // 輸出光閘一欄位名稱。
  Serial.print(gate1_level == HIGH ? "HIGH" : "LOW"); // 輸出光閘一的實際數位電位。
  Serial.print(gate1_level == HIGH ? "(預期遮光)" : "(預期未遮光)"); // 依目前電路預期極性說明光閘一狀態。
  Serial.print(" | OS25B10-2: "); // 輸出光閘二欄位名稱。
  Serial.print(gate2_level == HIGH ? "HIGH" : "LOW"); // 輸出光閘二的實際數位電位。
  Serial.println(gate2_level == HIGH ? "(預期遮光)" : "(預期未遮光)"); // 依目前電路預期極性說明光閘二狀態並換行。
} // 結束感測器資料輸出函式。
