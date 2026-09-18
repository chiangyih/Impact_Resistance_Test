// ============================================================================ // 本檔案是 01-irt.ino 感測器通訊測試程式。
// 本程式只測試 ADXL375、HX711 與兩組 OS25B10 是否能由 ESP32 讀取。 // 本行說明測試目的。
// 本程式不包含落下計時、加速度計算、校正、資料儲存或 Relay 控制。 // 本行說明目前未實作的功能。
// 開啟序列埠監控視窗並設定為 115200 baud，即可查看通訊結果。 // 本行說明操作方式。
// ============================================================================ // 本行是檔案標題結束線。

#include <Arduino.h> // 載入 Arduino 基本功能、GPIO、時間與 Serial。
#include <Wire.h> // 載入 I²C 通訊功能，供 ADXL375 使用。
#include <Adafruit_Sensor.h> // 載入 Adafruit 感測器事件資料型別。
#include <Adafruit_ADXL375.h> // 載入 ADXL375 感測器函式庫。
#include <Adafruit_HX711.h> // 載入 HX711 ADC 函式庫。

const uint8_t ADXL375_SDA_PIN = 21; // ADXL375 橘線 SDA 接到 ESP32 GPIO21。
const uint8_t ADXL375_SCL_PIN = 22; // ADXL375 棕線 SCL 接到 ESP32 GPIO22。
const uint8_t HX711_DATA_PIN = 32; // HX711 DATA 紫線接到 ESP32 GPIO32。
const uint8_t HX711_SCK_PIN = 33; // HX711 SCK 紫線接到 ESP32 GPIO33。
const uint8_t OS25B10_UPPER_PIN = 34; // 上方 OS25B10 紫線訊號接到 ESP32 GPIO34。
const uint8_t OS25B10_LOWER_PIN = 35; // 下方 OS25B10 橘線訊號接到 ESP32 GPIO35。
const uint8_t ADXL375_I2C_ADDRESS = 0x53; // 使用 SDO 接低電位時的 ADXL375 位址。
const uint32_t SERIAL_BAUD_RATE = 115200; // 設定序列埠傳輸速率為 115200 baud。
const uint32_t REPORT_INTERVAL_MS = 1000; // 設定每 1000 毫秒輸出一次測試資料。

Adafruit_ADXL375 adxl375(12345, &Wire); // 建立使用 ESP32 硬體 I²C 的 ADXL375 物件。
Adafruit_HX711 hx711(HX711_DATA_PIN, HX711_SCK_PIN); // 建立使用 GPIO32 與 GPIO33 的 HX711 物件。
bool adxl375_ready = false; // 保存 ADXL375 是否初始化成功。
uint32_t last_report_ms = 0; // 保存上一次輸出資料的時間。

bool readHx711(int32_t &raw_value); // 宣告不阻塞等待的 HX711 原始值讀取函式。
void printSensorData(); // 宣告感測器資料輸出函式。

void setup() { // Arduino 開機後只執行一次的初始化函式。
  Serial.begin(SERIAL_BAUD_RATE); // 啟動 USB 序列埠以輸出測試結果。
  delay(500); // 等待序列埠與 USB 轉換器穩定。
  Wire.begin(ADXL375_SDA_PIN, ADXL375_SCL_PIN); // 以 GPIO21 與 GPIO22 啟動 ESP32 I²C。
  Wire.setClock(100000); // 使用 100 kHz 標準 I²C 速度進行接線測試。
  pinMode(OS25B10_UPPER_PIN, INPUT); // 將上方 OS25B10 訊號腳設定為輸入。
  pinMode(OS25B10_LOWER_PIN, INPUT); // 將下方 OS25B10 訊號腳設定為輸入。
  hx711.begin(); // 初始化 HX711 的 DATA 與 SCK 腳位。
  adxl375_ready = adxl375.begin(ADXL375_I2C_ADDRESS); // 嘗試以 0x53 初始化 ADXL375。
  if (adxl375_ready) { // 如果 ADXL375 有回應，就設定測試資料率。
    adxl375.setDataRate(ADXL343_DATARATE_100_HZ); // 將 ADXL375 資料率設定為 100 Hz。
  } // 結束 ADXL375 成功處理。
  Serial.println(); // 輸出空白行以方便閱讀。
  Serial.println("=== 01-irt 感測器通訊測試 ==="); // 顯示目前程式名稱。
  Serial.println("ADXL375: SDA=21、SCL=22、預期位址=0x53"); // 顯示 ADXL375 接線與位址。
  Serial.println("HX711: DATA=32、SCK=33，兩條線皆為紫線"); // 顯示 HX711 接線與線材顏色。
  Serial.println("OS25B10: 上方=34、下方=35"); // 顯示兩個 OS25B10 的訊號腳位。
  Serial.println(adxl375_ready ? "[ADXL375] 通訊成功" : "[ADXL375] 通訊失敗"); // 回報 ADXL375 初始化結果。
  Serial.println("[HX711] 已初始化，開始測試 DATA 是否能取得原始值"); // 回報 HX711 已開始測試。
  Serial.println("[OS25B10] 開始讀取上方與下方訊號電位"); // 回報兩個 OS25B10 已開始測試。
} // 結束 setup 初始化函式。

void loop() { // ESP32 會持續重複執行的主迴圈函式。
  const uint32_t now_ms = millis(); // 取得 ESP32 開機後經過的毫秒數。
  if (now_ms - last_report_ms < REPORT_INTERVAL_MS) { // 尚未到輸出時間時不讀取資料。
    return; // 立即回到主迴圈，避免輸出過於頻繁。
  } // 結束輸出時間判斷。
  last_report_ms = now_ms; // 記錄本次輸出的時間。
  printSensorData(); // 讀取並輸出全部感測器的目前狀態。
} // 結束 loop 主迴圈函式。

bool readHx711(int32_t &raw_value) { // 嘗試讀取 HX711 A 通道增益 128 的原始值。
  if (hx711.isBusy()) { // DATA 為 HIGH 時代表 HX711 尚未完成轉換。
    return false; // 尚未就緒時立即回報失敗，不讓程式卡住。
  } // 結束 HX711 忙碌狀態判斷。
  raw_value = hx711.readChannelRaw(CHAN_A_GAIN_128); // 讀取 HX711 A 通道增益 128 原始值。
  return true; // 回報 ESP32 已從 HX711 取得資料。
} // 結束 HX711 原始值讀取函式。

void printSensorData() { // 讀取並輸出 ADXL375、HX711 與 OS25B10 的資料。
  sensors_event_t event; // 建立存放 ADXL375 X/Y/Z 加速度的事件容器。
  int32_t hx711_raw = 0; // 建立存放 HX711 原始值的變數。
  const int upper_level = digitalRead(OS25B10_UPPER_PIN); // 讀取上方 OS25B10 的數位電位。
  const int lower_level = digitalRead(OS25B10_LOWER_PIN); // 讀取下方 OS25B10 的數位電位。
  Serial.print("[資料] t="); // 輸出資料列的時間欄位名稱。
  Serial.print(millis()); // 輸出 ESP32 開機後經過的毫秒數。
  Serial.print(" ms | ADXL375: "); // 輸出 ADXL375 欄位名稱。
  if (adxl375_ready && adxl375.getEvent(&event)) { // 初始化成功且能讀到事件時輸出三軸資料。
    Serial.print("X="); // 輸出 X 軸欄位名稱。
    Serial.print(event.acceleration.x, 2); // 輸出 X 軸加速度，單位為 m/s^2。
    Serial.print(" Y="); // 輸出 Y 軸欄位名稱。
    Serial.print(event.acceleration.y, 2); // 輸出 Y 軸加速度，單位為 m/s^2。
    Serial.print(" Z="); // 輸出 Z 軸欄位名稱。
    Serial.print(event.acceleration.z, 2); // 輸出 Z 軸加速度，單位為 m/s^2。
    Serial.print(" m/s^2"); // 輸出加速度的單位。
  } else { // ADXL375 沒有回應或讀取失敗時輸出錯誤狀態。
    Serial.print("讀取失敗"); // 回報 ADXL375 目前沒有可用資料。
  } // 結束 ADXL375 資料輸出判斷。
  Serial.print(" | HX711: "); // 輸出 HX711 欄位名稱。
  if (readHx711(hx711_raw)) { // DATA 就緒時讀取 HX711 原始值。
    Serial.print("A128_RAW="); // 輸出 HX711 A 通道欄位名稱。
    Serial.print(hx711_raw); // 輸出 ESP32 從 HX711 收到的原始值。
  } else { // DATA 尚未就緒時輸出等待狀態。
    Serial.print("DATA 未就緒"); // 提示檢查 HX711 供電、共地、DATA 與 SCK 接線。
  } // 結束 HX711 資料輸出判斷。
  Serial.print(" | OS25B10 上方="); // 輸出上方 OS25B10 欄位名稱。
  Serial.print(upper_level == HIGH ? "HIGH" : "LOW"); // 輸出上方 OS25B10 的實際電位。
  Serial.print(" | 下方="); // 輸出下方 OS25B10 欄位名稱。
  Serial.println(lower_level == HIGH ? "HIGH" : "LOW"); // 輸出下方 OS25B10 的實際電位並換行。
} // 結束感測器資料輸出函式。
