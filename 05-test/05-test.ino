// 05-test.ino：第五版整合量測程式，整合 ADXL375、HX711、雙 Tracker 與 SH1106 OLED。
// 本版為量測流程第一版；尚未完成荷重校正、高速計時、完整衝擊與長時間驗證。
// 實板為NMK99v1；HX711改接SCK=25、DATA=33，避開GPIO32板載WS2812，改線須先斷電。
// 荷重元仍黑接 E+、白接 E-、綠接 A+、紅接 A-；舊版SCK32程式不可直接套用本版接線。
// HX711 VIN、VIO 與其餘模組沿用同一個 3V3，GND 共地，B+ 與 B- 不接。
// OLED 與 ADXL375 共用 SDA=21、SCL=22；上方 Tracker OUT=34，下方 OUT=35。
// GPIO34、GPIO35 無內建上下拉；模組 OUT 必須為三點三伏相容，不可輸入五伏。
// 不控制 GPIO26、GPIO27、繼電器或電磁鐵；ARM 只開始記錄，絕不釋放落體。
// 預設不自動去皮、不假造校正係數；距離採使用者回報二百二十五毫米，極性仍待序列設定。
// ADXL375 使用一百赫茲及串流 FIFO；時間戳為主機讀出時間，不是感測器精準取樣時刻。
// HX711 RATE 沿用實體開關，十或八十筆每秒均不能保證捕捉毫秒衝擊峰值。
// ARM 期間暫停 OLED 與一般序列輸出；完成後以 DUMP 匯出分開計時的原始 CSV。

#include <Arduino.h> // 載入 ESP32 基本腳位、序列埠與計時功能。
#include <Wire.h> // 載入共用 I²C 匯流排功能。
#include <Adafruit_ADXL375.h> // 沿用已安裝的 ADXL375 驅動與晶片初始化。
#include <U8g2lib.h> // 載入已確認正常的 SH1106 OLED 驅動。
#include <math.h> // 載入合成加速度、絕對值與有限數判斷。
#include <stdlib.h> // 載入具完整字尾檢查的數值解析函式。
#include <string.h> // 載入固定緩衝區命令比對功能。

const uint8_t HX_SCK = 25, HX_DATA = 33, UP_PIN = 34, DN_PIN = 35; // NMK99的GPIO32留給板載RGB，HX711時脈須實際移到GPIO25。
const uint8_t BOARD_RGB_PIN = 32; // NMK99教學標示單顆WS2812使用GPIO32，不以LED_BUILTIN推測RGB接腳。
static_assert(BOARD_RGB_PIN != HX_SCK && BOARD_RGB_PIN != HX_DATA && BOARD_RGB_PIN != UP_PIN && BOARD_RGB_PIN != DN_PIN, "RGB must not share sensor pins"); // 編譯時拒絕RGB與任一感測訊號重疊。
const uint8_t SDA_PIN = 21, SCL_PIN = 22, ADXL_ADDR = 0x53; // 沿用共用 I²C 與 SDO 接地的加速度計位址。
const uint32_t I2C_HZ = 100000, HX_TIMEOUT_MS = 1000; // 沿用已正常顯示的一百千赫匯流排及一秒荷重逾時。
const uint32_t ARM_TIMEOUT_US = 10000000, TRANSIT_TIMEOUT_US = 2000000; // 最多等上閘十秒、上下通過最多兩秒。
const uint32_t POST_CAPTURE_US = 2000000, MIN_TRANSIT_US = 200; // 下閘後再記錄兩秒，小於二百微秒視為可疑計時。
const uint16_t ACC_CAPACITY = 2048, HX_CAPACITY = 1536, CAL_SAMPLES = 32; // 有界 RAM 容量涵蓋設定的最長十四秒視窗。
const double CAL_MAX_SPAN = 2000.0, CAL_MIN_DELTA = 100.0; // 校正收集的粗略穩定門檻，不是精度或承重合格標準。
enum Phase : uint8_t { IDLE, ARMED, WAIT_DN, CAPTURE, DONE, FAULT }; // 定義待機、等上閘、等下閘、後錄、完成及中止狀態。
enum Reason : uint8_t { NONE, LOWER_FIRST, TOO_FAST, UPPER_TIMEOUT, LOWER_TIMEOUT, USER_STOP, BUFFER_FULL }; // 定義不依賴字串的中斷結束原因。
struct AccRecord { uint32_t us; int16_t x, y, z; }; // 保存讀出時間與三軸原始計數，並非同步荷重資料。
struct HxRecord { uint32_t us; int32_t raw; uint8_t sat; }; // 保存每個通過握手檢查的荷重框與飽和標記。
AccRecord acc_records[ACC_CAPACITY]; // 配置加速度原始資料緩衝，試驗後由使用者明確清除。
HxRecord hx_records[HX_CAPACITY]; // 配置荷重原始資料緩衝，不以低速畫面代替擷取。
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE, SCL_PIN, SDA_PIN); // 沿用八行無缺邊的面板設定。
Adafruit_ADXL375 adxl(12345, &Wire); // 建立既有 I²C 加速度計驅動。
portMUX_TYPE gate_mux = portMUX_INITIALIZER_UNLOCKED; // 保護中斷與主迴圈共同存取的試驗狀態。
portMUX_TYPE hx_mux = portMUX_INITIALIZER_UNLOCKED; // 只保護每個 HX711 高脈衝，不長時間禁止光閘中斷。
volatile uint8_t phase = IDLE, reason = NONE; // 中斷可更新狀態，文字及顯示留在主迴圈處理。
volatile uint32_t armed_us = 0, upper_us = 0, lower_us = 0, ignored_edges = 0; // 保存同一次試驗的微秒時間戳及重複有效緣數。
volatile int upper_active = -1, lower_active = -1; // 預設未設定觸發電位，避免猜測 Tracker 極性。
double distance_mm = 225.0, tare_raw = 0, counts_per_g = 0; // 上下間距依使用者回報二十二點五公分換為毫米，校正仍無效。
bool tare_valid = false, calibration_valid = false; // 去皮不代表已完成比例校正。
uint8_t oled_address = 0; // 零代表未找到候選 OLED 位址。
bool adxl_ready = false, acc_seen = false, hx_seen = false, hx_frame_ok = true; // 分開保存各感測器的連線與資料有效性。
int16_t acc_x = 0, acc_y = 0, acc_z = 0; // 保存最新三軸原始計數。
int32_t hx_raw = 0; // 保存原始荷重值，包含正負滿量程。
uint32_t hx_start_ms = 0, hx_last_ms = 0, acc_last_ms = 0; // 保存資料年齡及首筆等待起點。
uint32_t hx_rx = 0, hx_errors = 0, acc_errors = 0, fifo_full = 0; // 累計接收、握手、I²C 與 FIFO 滿載警示。
uint32_t trial_hx_errors = 0, trial_acc_errors = 0, trial_fifo_full = 0; // 保存試驗起點計數，完成時固定其差值。
uint32_t trial_hx_end = 0, trial_acc_end = 0, trial_fifo_end = 0; // 結束後不讓即時診斷改寫已完成試驗的錯誤統計。
uint16_t acc_count = 0, hx_count = 0, trial_sat = 0; // 保存本次緩衝資料筆數及滿量程框數。
bool trial_stale = false, trial_acc_stale = false, completion_reported = false; // 以黏著警示保留試驗中曾發生的失聯。
uint32_t trial_id = 0, last_fifo_ms = 0, last_display_ms = 0, last_live_ms = 0, last_retry_ms = 0; // 保存試驗識別與不同輸出的排程。
uint8_t cal_mode = 0; // 零表示無收集，一表示去皮，二表示已知重量比例校正。
uint16_t cal_count = 0; // 只累計新到且未飽和的資料，不重複使用畫面快取。
double cal_sum = 0, cal_min = 0, cal_max = 0, cal_mass_g = 0; // 保存三十二筆收集統計與使用者提供的已知克數。
uint32_t cal_start_ms = 0; // 限制收集最長八秒，避免缺席時永遠等待。
char command_line[80]; // 固定命令長度，避免使用持續增長的字串。
uint8_t command_length = 0; // 保存目前命令長度。
bool command_overflow = false; // 過長命令整行捨棄，不執行被截斷的 ARM 等命令。

bool isRecording() { // 判斷是否正在保存試驗原始資料。
  return phase == ARMED || phase == WAIT_DN || phase == CAPTURE; // 三個活動狀態都暫停慢速畫面與一般輸出。
} // 結束活動狀態判斷。

int32_t decodeHx(uint32_t bits) { // 明確轉換二十四位元補數，與第四版相同。
  bits &= 0x00FFFFFFUL; // 遮除非資料位元。
  return static_cast<int32_t>(bits) - ((bits & 0x00800000UL) ? 16777216L : 0L); // 保留真實正負滿量程而不濾除。
} // 結束補數轉換。

const char *phaseName() { // 將共享狀態轉成 OLED 與序列埠可讀文字。
  static const char *names[] = {"IDLE", "ARMED", "WAIT_DN", "CAPTURE", "DONE", "FAULT"}; // 各狀態使用固定短名稱。
  return names[phase]; // 狀態值只由本檔定義的列舉指定。
} // 結束狀態名稱。

const char *reasonName() { // 將結束原因轉成固定短文字。
  static const char *names[] = {"NONE", "LOWER_FIRST", "TOO_FAST", "UPPER_TIMEOUT", "LOWER_TIMEOUT", "USER_STOP", "BUFFER_FULL"}; // 所有中止均保留原因。
  return names[reason]; // 不在中斷內輸出字串。
} // 結束原因名稱。

const char *hxStatus() { // 完整保留第四版的異常辨識。
  if (!hx_frame_ok) return "LINK_ERR"; // 握手錯誤優先顯示，不冒充正常資料。
  if (!hx_seen) return millis() - hx_start_ms >= HX_TIMEOUT_MS ? "TIMEOUT" : "WAIT"; // 首筆缺席與正常等待分開。
  if (millis() - hx_last_ms >= HX_TIMEOUT_MS) return "STALE"; // 舊快取保持可見但明確標記過期。
  if (hx_raw == -8388608L || hx_raw == 8388607L) return "SAT"; // 正負滿量程均如實顯示。
  return "OK"; // 只表示近期有效框，不代表已校正或供電完全合格。
} // 結束荷重狀態。

void ARDUINO_ISR_ATTR upperInterrupt() { // 上方只接受設定極性的進入緣，不在中斷做通訊。
  if (digitalRead(UP_PIN) != upper_active) return; // 忽略退出緣及尚未設定極性的所有邊緣。
  const uint32_t now = micros(); // 使用微秒時間戳，實際誤差仍受中斷延遲影響。
  portENTER_CRITICAL_ISR(&gate_mux); // 原子地更新狀態與時間。
  if (phase == ARMED) { if (now - armed_us >= ARM_TIMEOUT_US) { reason = UPPER_TIMEOUT; phase = FAULT; } else { upper_us = now; phase = WAIT_DN; } } // 只鎖定期限內第一次上方有效觸發。
  else if (phase == WAIT_DN || phase == CAPTURE) ++ignored_edges; // 不讓反射抖動覆寫第一次上閘時間。
  portEXIT_CRITICAL_ISR(&gate_mux); // 立即結束短暫保護。
} // 結束上方中斷。

void ARDUINO_ISR_ATTR lowerInterrupt() { // 下方中斷執行順序及最短間隔檢查。
  if (digitalRead(DN_PIN) != lower_active) return; // 只接受使用者確認的觸發電位。
  const uint32_t now = micros(); // 記錄中斷處理時刻，不宣稱光學瞬時位置精度。
  portENTER_CRITICAL_ISR(&gate_mux); // 保護上下閘共享狀態。
  if (phase == ARMED) { reason = LOWER_FIRST; phase = FAULT; } // 下閘先到時不輸出虛假速度。
  else if (phase == WAIT_DN) { // 只有上閘已到才接受下閘。
    lower_us = now; // 保留可疑觸發時間供追溯。
    const uint32_t dt = now - upper_us; // 無號減法可處理單次微秒計時溢位。
    if (dt < MIN_TRANSIT_US) { reason = TOO_FAST; phase = FAULT; } // 過短間隔不計為有效速度。
    else if (dt > TRANSIT_TIMEOUT_US) { reason = LOWER_TIMEOUT; phase = FAULT; } // 逾時到達亦不補算速度。
    else phase = CAPTURE; // 通過順序檢查後繼續記錄撞擊後視窗。
  } else if (phase == CAPTURE) ++ignored_edges; // 忽略下閘重複觸發，不重設後錄終點。
  portEXIT_CRITICAL_ISR(&gate_mux); // 立即返回主流程。
} // 結束下方中斷。

bool i2cAck(uint8_t address) { // 檢查位址是否存在，不等同畫面或感測器驗證。
  Wire.beginTransmission(address); // 使用七位元位址。
  return Wire.endTransmission() == 0; // 只把收到 ACK 視為有裝置。
} // 結束位址檢查。

bool writeAdxl(uint8_t reg, uint8_t value) { // 使用可檢查結果的寄存器寫入設定 FIFO。
  Wire.beginTransmission(ADXL_ADDR); // 對實際加速度計位址開始傳送。
  Wire.write(reg); Wire.write(value); // 依序指定寄存器及內容。
  return Wire.endTransmission() == 0; // 將匯流排錯誤回報呼叫端。
} // 結束寄存器寫入。

bool readAdxl(uint8_t reg, uint8_t *data, uint8_t length) { // 有界讀取寄存器與完整三軸資料。
  Wire.beginTransmission(ADXL_ADDR); Wire.write(reg); // 指定起始寄存器。
  if (Wire.endTransmission(false) != 0) return false; // 指標傳送失敗不沿用舊資料。
  if (Wire.requestFrom(ADXL_ADDR, length) != length) return false; // 短讀明確判為失敗。
  for (uint8_t i = 0; i < length; ++i) data[i] = Wire.read(); // 固定長度讀出所有請求位元組。
  return true; // 只有完整交易才回報成功。
} // 結束寄存器讀取。

bool resetFifo() { // 在待機或啟動試驗前清除舊 FIFO，活動期間不重設。
  return writeAdxl(0x38, 0x00) && writeAdxl(0x38, 0x9F); // 先旁路清空，再啟用三十二層串流與三十一筆水位。
} // 結束 FIFO 重設。

void initializeAdxl() { // 使用既有函式庫確認晶片身分，再設定可讀回的量測參數。
  const bool ack_expected = i2cAck(ADXL_ADDR), ack_alternate = i2cAck(0x1D); // 額外列出預期與替代位址，不自動更改接線或裝置位址。
  adxl_ready = adxl.begin(ADXL_ADDR); // 函式庫檢查裝置識別碼 0xE5 並設定固定量程。
  const bool identified = adxl_ready; // 區分晶片識別與後續設定失敗，避免一律歸因於接線。
  if (adxl_ready) adxl_ready = writeAdxl(0x2C, 0x0A) && resetFifo(); // 設一百赫茲，低於一百千赫 I²C 的建議上限。
  uint8_t rate = 0, mode = 0; // 保存寄存器讀回結果，不只相信寫入呼叫。
  if (adxl_ready) adxl_ready = readAdxl(0x2C, &rate, 1) && readAdxl(0x38, &mode, 1) && rate == 0x0A && mode == 0x9F; // 核對實際設定。
  Serial.printf("# ADXL375=%s address=0x53 ODR=100Hz FIFO=STREAM I2C=100kHz\n", adxl_ready ? "READY" : "ERROR"); // 回報啟動或待機重試結果。
  Serial.printf("# ADXL_DIAG ACK53=%d ACK1D=%d IDENTIFIED=%d RATE=0x%02X FIFO=0x%02X\n", ack_expected, ack_alternate, identified, rate, mode); // 沒有ACK或寄存器讀回不符均留下證據，不猜測硬體故障。
} // 結束加速度計初始化。

void initializeOled() { // 沿用第四版已正常工作的控制器與位址。
  oled_address = i2cAck(0x3C) ? 0x3C : (i2cAck(0x3D) ? 0x3D : 0); // 沒有裝置時保持其他感測器工作。
  if (!oled_address) { Serial.println("# OLED MISSING: no ACK at 0x3C/0x3D"); return; } // 不對缺席面板傳送命令，並明確回報匯流排結果。
  oled.setI2CAddress(oled_address << 1); oled.setBusClock(I2C_HZ); // 設定位址與既有速度。
  oled.begin(); oled.setPowerSave(0); oled.setContrast(255); // 初始化面板，不以 ACK 代替目視確認。
  oled.setDisplayRotation(U8G2_R0); oled.setFontDirection(0); // 明確維持橫向及正常字向，不更換已確認的SH1106控制器。
  oled.setFont(u8g2_font_4x6_tr); oled.setFontPosBaseline(); // 六像素英數字型搭配七像素行距，讓八行上下都保留空白。
  oled.setFontMode(1); oled.setDrawColor(1); // 明確使用透明字型及亮點繪字，畫面每次仍先完整清除。
  Wire.setClock(I2C_HZ); Wire.setTimeOut(25); // 函式庫初始化後恢復共用匯流排設定。
  Serial.printf("# OLED ACK=0x%02X SH1106 128x64 FONT=4x6 SAFE=x4..123,y4..59; visual confirmation needed\n", oled_address); // ACK與版面界線不等於實際畫面已驗證。
} // 結束顯示初始化。

void drawOledLine(uint8_t row, const char *text) { // 固定座標及有限字數繪字，不使用可能換行的串流游標。
  if (row >= 8 || !text) return; // 拒絕第九行及空指標，避免超出面板。
  char clipped[31]; // 四像素字寬最多容納三十個ASCII字元及終止字元。
  strncpy(clipped, text, sizeof(clipped) - 1); clipped[sizeof(clipped) - 1] = '\0'; // 有界複製，不讓長狀態字串影響下一行。
  size_t length = strlen(clipped); // 保存有限字串的實際長度。
  while (length > 0 && oled.getStrWidth(clipped) > 120) clipped[--length] = '\0'; // 依驅動的實際字寬縮短，不只依字數猜測。
  oled.setClipWindow(4, 4, 124, 60); // 四邊保留至少四像素，排版不使用面板邊界像素。
  oled.drawStr(4, 9 + row * 7, clipped); // 八行基線為九至五十八，讓字型上緣與下緣都落在四像素留白內。
  oled.setMaxClipWindow(); // 恢復全畫布設定，不讓局部裁切殘留給其他畫面。
} // 結束單行安全繪字。

void abortTrial(uint8_t why) { // 主迴圈安全中止試驗並保留已收資料。
  portENTER_CRITICAL(&gate_mux); // 防止同時到達的 Tracker 中斷覆寫結果。
  if (isRecording()) { reason = why; phase = FAULT; } // 已完成的資料不因 STOP 再被改寫。
  portEXIT_CRITICAL(&gate_mux); // 中止不操控繼電器或電磁鐵。
} // 結束試驗中止。

void collectCalibration(int32_t raw) { // 去皮及比例校正只消費新收到的未飽和框。
  if (!cal_mode) return; // 沒有明確命令時完全不去皮。
  if (raw == -8388608L || raw == 8388607L) { cal_mode = 0; Serial.println("# CAL_ABORT SAT; old calibration unchanged"); return; } // 異常資料不寫入係數。
  if (!cal_count) cal_min = cal_max = raw; // 首筆初始化最小最大值。
  cal_min = fmin(cal_min, raw); cal_max = fmax(cal_max, raw); cal_sum += raw; ++cal_count; // 累加有限數目的新框。
  if (cal_count < CAL_SAMPLES) return; // 未達三十二筆就交回主迴圈，不阻塞等候。
  const double mean = cal_sum / cal_count; // 計算收集平均，原始資料仍不被替換。
  if (cal_max - cal_min > CAL_MAX_SPAN) Serial.println("# CAL_ABORT unstable span>2000 counts; old calibration unchanged"); // 粗略穩定檢查不通過就保留舊值。
  else if (cal_mode == 1) { tare_raw = mean; tare_valid = true; calibration_valid = false; counts_per_g = 0; Serial.printf("# TARE_OK offset=%.3f; UNCAL; use known mass with CAL grams\n", tare_raw); } // 新去皮使舊比例失效。
  else if (fabs(mean - tare_raw) < CAL_MIN_DELTA) Serial.println("# CAL_ABORT signal<100 counts; old calibration unchanged"); // 不用接近雜訊的差值建立係數。
  else { counts_per_g = (mean - tare_raw) / cal_mass_g; calibration_valid = true; Serial.printf("# CAL_USER counts_per_g=%.9f; static user calibration, not independently validated\n", counts_per_g); } // 接受正負方向但明確標為使用者靜態校正。
  cal_mode = 0; // 每次命令只收集一組，不持續自動校正。
} // 結束校正收集。

void pollHx() { // 延續第四版的固定二十五脈衝與讀後回高檢查。
  if (digitalRead(HX_DATA) != LOW) return; // 資料未就緒立即返回，不等待轉換。
  uint32_t bits = 0; // 保存本輪原始二十四位元。
  delayMicroseconds(2); // 保留資料下降到第一個時脈的間隔。
  for (uint8_t i = 0; i < 25; ++i) { // 最後一脈衝選擇下一筆 A128。
    portENTER_CRITICAL(&hx_mux); // 只在時脈高電位期間暫停本核心中斷。
    digitalWrite(HX_SCK, HIGH); delayMicroseconds(2); // 高電位設定兩微秒，實際仍須示波器驗證。
    if (i < 24) bits = (bits << 1) | (digitalRead(HX_DATA) == HIGH ? 1UL : 0UL); // 只收資料脈衝，不把增益位算入。
    digitalWrite(HX_SCK, LOW); // 離開保護區前確保時脈已低，避免中斷導致誤斷電。
    portEXIT_CRITICAL(&hx_mux); delayMicroseconds(2); // 低電位允許 Tracker 中斷，不長時間封鎖計時。
  } // 結束固定脈衝。
  hx_frame_ok = digitalRead(HX_DATA) == HIGH; // 握手不通過就不覆寫最近有效框。
  if (!hx_frame_ok) { ++hx_errors; if (cal_mode) { cal_mode = 0; Serial.println("# CAL_ABORT LINK_ERR"); } return; } // 保留握手異常計數。
  hx_raw = decodeHx(bits); hx_seen = true; hx_last_ms = millis(); ++hx_rx; // 即使滿量程也保留真實原始值與時間。
  if (isRecording()) { // 活動視窗內保存每個收到的框。
    if (hx_count >= HX_CAPACITY) abortTrial(BUFFER_FULL); // 不默默覆寫舊資料。
    else { const bool sat = hx_raw == -8388608L || hx_raw == 8388607L; hx_records[hx_count++] = {micros() - armed_us, hx_raw, static_cast<uint8_t>(sat)}; if (sat) ++trial_sat; } // 保存接收時間而非晶片精確取樣時間。
  } // 結束荷重記錄。
  collectCalibration(hx_raw); // 只有明確校正命令才收集資料。
} // 結束荷重輪詢。

void pollAdxl() { // 排空 FIFO，避免 OLED 傳輸期間的中間樣本直接遺失。
  if (!adxl_ready || millis() - last_fifo_ms < 2) return; // 每兩毫秒檢查，未連線時不反覆讀不存在裝置。
  last_fifo_ms = millis(); // 記錄主機輪詢時間。
  uint8_t entries = 0, source = 0; // 保存 FIFO 狀態與滿載警示。
  if (!readAdxl(0x39, &entries, 1) || !readAdxl(0x30, &source, 1)) { ++acc_errors; return; } // 短讀明確計數，不產生假零。
  entries &= 0x3F; // 只取 FIFO 筆數欄位。
  if (entries >= 32 || (source & 1)) ++fifo_full; // 滿載僅記為可能漏樣警示，不假造遺失筆數。
  if (entries > 32) { ++acc_errors; return; } // 拒絕不合法的 FIFO 長度。
  for (uint8_t n = 0; n < entries; ++n) { // 固定上限三十二筆，匯流排錯誤不進入無限等待。
    uint8_t bytes[6]; // 一次讀完整三軸，符合 FIFO 彈出規則。
    if (!readAdxl(0x32, bytes, 6)) { ++acc_errors; break; } // 讀取失敗不存入上次快取。
    acc_x = static_cast<int16_t>((static_cast<uint16_t>(bytes[1]) << 8) | bytes[0]); // 組合 X 軸有正負號計數。
    acc_y = static_cast<int16_t>((static_cast<uint16_t>(bytes[3]) << 8) | bytes[2]); // 組合 Y 軸有正負號計數。
    acc_z = static_cast<int16_t>((static_cast<uint16_t>(bytes[5]) << 8) | bytes[4]); // 組合 Z 軸有正負號計數。
    acc_seen = true; acc_last_ms = millis(); // 更新成功交易的資料年齡。
    if (isRecording()) { // 只在明確 ARM 後保存試驗資料。
      if (acc_count >= ACC_CAPACITY) abortTrial(BUFFER_FULL); // 緩衝區滿時中止而不覆寫。
      else acc_records[acc_count++] = {micros() - armed_us, acc_x, acc_y, acc_z}; // FIFO 舊樣本仍標為讀出時間，不能誤當精確取樣時刻。
    } // 結束單筆加速度保存。
    delayMicroseconds(5); // 滿足 FIFO 兩次資料讀取間至少五微秒要求。
  } // 結束本輪 FIFO 排空。
} // 結束加速度輪詢。

void printStatus() { // 待機或試驗後輸出可供接線診斷的完整狀態。
  Serial.printf("# STATUS phase=%s reason=%s CAL=%s HX=%s RAW=", phaseName(), reasonName(), calibration_valid ? "CAL_USER" : "UNCAL", hxStatus()); // 不以原始數字冒充克數。
  if (hx_seen) Serial.print(hx_raw); else Serial.print("NA"); // 未有資料明確顯示缺值。
  Serial.printf(" RX=%lu ERR=%lu AGE_ms=%lu ADXL=%s AERR=%lu FIFO_FULL=%lu UP=%d DN=%d POL=%d/%d DIST_mm=%.3f OLED=0x%02X SCK=%u DATA=%u RGB=%u:OFF_REQ LAYOUT=2\n", static_cast<unsigned long>(hx_rx), static_cast<unsigned long>(hx_errors), static_cast<unsigned long>(hx_seen ? millis() - hx_last_ms : 0), acc_seen && millis() - acc_last_ms < 1000 ? "OK" : "MISSING", static_cast<unsigned long>(acc_errors), static_cast<unsigned long>(fifo_full), digitalRead(UP_PIN), digitalRead(DN_PIN), upper_active, lower_active, distance_mm, oled_address, HX_SCK, HX_DATA, BOARD_RGB_PIN); // 列出真實常數與版次，RGB要求關閉不等於目視已驗證。
} // 結束即時診斷。

void updateOled() { // 僅在非記錄期間刷新八行，避免阻塞試驗採樣。
  if (!oled_address || isRecording()) return; // 活動時保留 ARM 前的提示畫面。
  if (!i2cAck(oled_address)) { oled_address = 0; return; } // 掉線時等待待機重試，不阻塞所有功能。
  oled.clearBuffer(); // 清除上一頁，避免舊字殘留。
  char line[80]; // 先格式化成有界字串，再由單行函式按實際像素裁切。
  snprintf(line, sizeof(line), "05 TEST %s", calibration_valid ? "CAL_USER" : "UNCAL"); drawOledLine(0, line); // 第一行永遠保留校正狀態。
  if (acc_seen && millis() - acc_last_ms < 1000) snprintf(line, sizeof(line), "X%+.1f Y%+.1f g", acc_x * 0.049, acc_y * 0.049); else snprintf(line, sizeof(line), "ADXL375 MISSING/STALE"); // 過期三軸不顯示為即時值。
  drawOledLine(1, line); // 第二行X與Y各保留符號及g單位。
  if (acc_seen && millis() - acc_last_ms < 1000) snprintf(line, sizeof(line), "Z%+.1f A%.1f g", acc_z * 0.049, sqrt(static_cast<double>(acc_x) * acc_x + static_cast<double>(acc_y) * acc_y + static_cast<double>(acc_z) * acc_z) * 0.049); else snprintf(line, sizeof(line), "Z:-- A:--"); // 第三行合成值包含重力。
  drawOledLine(2, line); // 用固定行座標顯示Z與合成值。
  if (hx_seen) snprintf(line, sizeof(line), "HX RAW:%ld", static_cast<long>(hx_raw)); else snprintf(line, sizeof(line), "HX RAW:--"); drawOledLine(3, line); // 第四行保留未濾波計數。
  snprintf(line, sizeof(line), "HX:%s U%d D%d", hxStatus(), digitalRead(UP_PIN), digitalRead(DN_PIN)); drawOledLine(4, line); // 第五行保留真實電位與荷重異常。
  snprintf(line, sizeof(line), "%s A:%u H:%u", phaseName(), acc_count, hx_count); drawOledLine(5, line); // 第六行顯示狀態與記錄筆數。
  if (phase == DONE && lower_us - upper_us >= MIN_TRANSIT_US && distance_mm > 0) snprintf(line, sizeof(line), "DT:%.2fms V:%.2fm/s", (lower_us - upper_us) / 1000.0, distance_mm * 1000.0 / (lower_us - upper_us)); else snprintf(line, sizeof(line), "DIST:%.1fmm POL:%d/%d", distance_mm, upper_active, lower_active); // 未知距離或未完成時不顯示假速度。
  drawOledLine(6, line); // 第七行過長時只裁本行，不移動下方提示。
  if (phase == FAULT) snprintf(line, sizeof(line), "%s", reasonName()); else if (cal_mode) snprintf(line, sizeof(line), "CAL %u/%u HOLD STILL", cal_count, CAL_SAMPLES); else snprintf(line, sizeof(line), "%s", phase == DONE ? "DUMP via USB / CLEAR" : "HELP via USB; no relay"); // 第八行保留中止原因及操作提示。
  drawOledLine(7, line); // 最後一行仍在下方四像素安全區內。
  oled.sendBuffer(); // 僅非記錄時傳送完整畫面，FIFO 保存其間加速度資料。
} // 結束畫面更新。

void armTrial() { // 只啟動資料記錄，不觸發釋放機構。
  if (phase != IDLE || cal_mode) { Serial.println("# ARM_REJECT use CLEAR after saving data; finish calibration first"); return; } // 防止覆蓋完成的試驗或中斷校正。
  if (upper_active < 0 || lower_active < 0) { Serial.println("# ARM_REJECT set POL upper lower to verified 0/1 levels"); return; } // 極性不明時拒絕啟動。
  if (digitalRead(UP_PIN) == upper_active || digitalRead(DN_PIN) == lower_active) { Serial.println("# ARM_REJECT both markers must be away from sensors"); return; } // 已觸發的感測頭不可當作新進入緣。
  if (strcmp(hxStatus(), "OK") || !adxl_ready || !acc_seen || millis() - acc_last_ms >= 1000) { Serial.println("# ARM_REJECT sensors must be recent and non-SAT"); return; } // 不帶著已知失聯或飽和開始試驗。
  Serial.println("# ARM: recording only; NO RELEASE; OLED/live output paused; STOP aborts; speed NA if DIST=0"); // 在正式計時前輸出操作限制。
  if (oled_address) { oled.clearBuffer(); drawOledLine(1, "05 ARM - OLED PAUSED"); drawOledLine(3, "NO RELAY / USB STOP"); drawOledLine(5, "Recording <=14 seconds"); oled.sendBuffer(); } // ARM提示也沿用安全邊界，不在活動期間送畫面。
  Serial.flush(); // 在起點前送完提示，活動視窗不排入一般診斷文字。
  if (!resetFifo()) { ++acc_errors; Serial.println("# ARM_REJECT FIFO reset failed"); return; } // 慢速提示全部完成後才清空 FIFO，防止畫面傳輸期間的舊樣本混入。
  acc_count = hx_count = trial_sat = 0; trial_stale = trial_acc_stale = completion_reported = false; // 初始化本次統計，不變更校正值。
  trial_hx_errors = hx_errors; trial_acc_errors = acc_errors; trial_fifo_full = fifo_full; ++trial_id; // 保存試驗錯誤計數起點。
  portENTER_CRITICAL(&gate_mux); // 原子地建立同一個時間原點。
  upper_us = lower_us = ignored_edges = 0; reason = NONE; armed_us = micros(); phase = ARMED; // 等待第一個有效上閘緣，不自動釋放落體。
  portEXIT_CRITICAL(&gate_mux); // 讓 Tracker 中斷立即開始計時。
} // 結束 ARM。

void serviceTrial() { // 主迴圈負責有界逾時、後錄終止與完成通知。
  portENTER_CRITICAL(&gate_mux); // 與上下閘中斷協調終止條件。
  const uint32_t now = micros(); // 進入保護區後才讀時鐘，避免中斷新時間戳比舊快照更晚而誤判溢位。
  if (phase == ARMED && now - armed_us >= ARM_TIMEOUT_US) { reason = UPPER_TIMEOUT; phase = FAULT; } // 未到上閘十秒就中止。
  else if (phase == WAIT_DN && now - upper_us > TRANSIT_TIMEOUT_US) { reason = LOWER_TIMEOUT; phase = FAULT; } // 下閘逾時不持續記錄無限資料。
  else if (phase == CAPTURE && now - lower_us >= POST_CAPTURE_US) phase = DONE; // 成功通過後固定保存兩秒資料。
  portEXIT_CRITICAL(&gate_mux); // 不在保護區內做感測器或序列通訊。
  if (isRecording()) { // 保存試驗過程曾出現的過期警示。
    if (millis() - hx_last_ms >= HX_TIMEOUT_MS) trial_stale = true; // 曾失去荷重更新就保留，不因恢復被抹掉。
    if (millis() - acc_last_ms >= 1000) trial_acc_stale = true; // 加速度失聯同樣保留。
  } // 結束持續診斷。
  if ((phase == DONE || phase == FAULT) && !completion_reported) { // 每次試驗只通知一次完成或中止。
    trial_hx_end = hx_errors - trial_hx_errors; trial_acc_end = acc_errors - trial_acc_errors; trial_fifo_end = fifo_full - trial_fifo_full; // 固定本次錯誤差值。
    completion_reported = true; // 防止每輪重印。
    Serial.printf("# TRIAL %lu %s reason=%s A=%u H=%u SAT=%u HX_STALE=%d ACC_STALE=%d; DUMP to export; CLEAR before next ARM\n", static_cast<unsigned long>(trial_id), phaseName(), reasonName(), acc_count, hx_count, trial_sat, trial_stale, trial_acc_stale); // 保留錯誤而不宣告所有資料有效。
  } // 結束完成通知。
} // 結束試驗排程。

void dumpTrial() { // 試驗結束後才輸出原始 CSV，避免序列傳輸拖慢採樣。
  if (phase != DONE && phase != FAULT) { Serial.println("# DUMP_REJECT complete/STOP trial first"); return; } // 活動或未曾記錄時拒絕匯出。
  double peak_g = 0, peak_axis_g = 0; int32_t minimum = 8388607, maximum = -8388608; uint16_t valid_hx = 0; // 建立只依實測樣本計算的摘要。
  for (uint16_t i = 0; i < acc_count; ++i) { const auto &a = acc_records[i]; peak_g = fmax(peak_g, sqrt(static_cast<double>(a.x) * a.x + static_cast<double>(a.y) * a.y + static_cast<double>(a.z) * a.z) * 0.049); peak_axis_g = fmax(peak_axis_g, fmax(fabs(a.x), fmax(fabs(a.y), fabs(a.z))) * 0.049); } // 合成峰值包含重力，沒有假扣一個 g。
  for (uint16_t i = 0; i < hx_count; ++i) if (!hx_records[i].sat) { minimum = hx_records[i].raw < minimum ? hx_records[i].raw : minimum; maximum = hx_records[i].raw > maximum ? hx_records[i].raw : maximum; ++valid_hx; } // 滿量程框保留於原始列但不當作可信峰值。
  Serial.printf("# BEGIN trial=%lu state=%s reason=%s CAL=%s distance_mm=%.6f polarity=%d/%d\n", static_cast<unsigned long>(trial_id), phaseName(), reasonName(), calibration_valid ? "CAL_USER_STATIC_NOT_VALIDATED" : "UNCAL", distance_mm, upper_active, lower_active); // 匯出量測設定及未校正狀態。
  Serial.printf("# HX_SCK=%u HX_DATA=%u RGB_PIN=%u A128 RATE=physical_L10_or_H80; ACC_ODR=100Hz; timestamps=host_readout_us_from_ARM; rows_grouped_by_sensor\n", HX_SCK, HX_DATA, BOARD_RGB_PIN); // 從實際常數輸出接腳，避免移腳後CSV仍印舊設定。
  Serial.printf("# tare_valid=%d offset=%.6f counts_per_g=%.9f HX_ERR=%lu ACC_ERR=%lu FIFO_FULL=%lu SAT=%u HX_STALE=%d ACC_STALE=%d ignored_edges=%lu\n", tare_valid, tare_raw, counts_per_g, static_cast<unsigned long>(trial_hx_end), static_cast<unsigned long>(trial_acc_end), static_cast<unsigned long>(trial_fifo_end), trial_sat, trial_stale, trial_acc_stale, static_cast<unsigned long>(ignored_edges)); // 匯出校正與品質旗標。
  if (phase == DONE) { const uint32_t dt = lower_us - upper_us; Serial.printf("# TRANSIT upper_us=%lu lower_us=%lu dt_us=%lu speed_mps=", static_cast<unsigned long>(upper_us - armed_us), static_cast<unsigned long>(lower_us - armed_us), static_cast<unsigned long>(dt)); if (distance_mm > 0) Serial.println(distance_mm * 1000.0 / dt, 6); else Serial.println("NA"); } // 只有成功通過才輸出時間差，距離未知則速度缺值。
  if (acc_count) Serial.printf("# ACC_PEAK magnitude_g=%.6f axis_abs_g=%.6f includes_gravity=1 NEAR_RANGE=%d window=whole_ARM_trial\n", peak_g, peak_axis_g, peak_axis_g >= 190); else Serial.println("# ACC_PEAK NA"); // 峰值限整個已記錄視窗，缺資料不輸出假零。
  if (valid_hx) Serial.printf("# HX_RANGE raw_min=%ld raw_max=%ld sampled_values_only_not_impact_peak_force\n", static_cast<long>(minimum), static_cast<long>(maximum)); else Serial.println("# HX_RANGE NA"); // 原始極值不是公斤或牛頓。
  Serial.println("kind,t_read_us,x_raw,y_raw,z_raw,x_g,y_g,z_g,mag_g,hx_raw,hx_status,mass_g_static"); // 不同感測器使用相同欄位，缺值以 NA 表示。
  for (uint16_t i = 0; i < acc_count; ++i) { const auto &a = acc_records[i]; const double magnitude = sqrt(static_cast<double>(a.x) * a.x + static_cast<double>(a.y) * a.y + static_cast<double>(a.z) * a.z) * 0.049; Serial.printf("ACC,%lu,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,NA,NA,NA\n", static_cast<unsigned long>(a.us), a.x, a.y, a.z, a.x * 0.049, a.y * 0.049, a.z * 0.049, magnitude); } // 不插值、不把荷重舊值複製到每筆加速度。
  for (uint16_t i = 0; i < hx_count; ++i) { const auto &h = hx_records[i]; Serial.printf("HX,%lu,NA,NA,NA,NA,NA,NA,NA,%ld,%s,", static_cast<unsigned long>(h.us), static_cast<long>(h.raw), h.sat ? "SAT" : "OK"); if (calibration_valid && !h.sat) Serial.println((h.raw - tare_raw) / counts_per_g, 6); else Serial.println("NA"); } // 只有明確比例校正後才輸出靜態等效克數。
  Serial.println("# END; RAM data retained until CLEAR or power loss; no file automatically saved on computer"); // 匯出不等同電腦已保存檔案，原始 RAM 仍保留。
} // 結束 CSV 匯出。

bool parseNumber(const char *text, double &value) { // 拒絕空字串、NaN、無窮及帶垃圾字尾的設定。
  if (!text || !*text) return false; // 缺參數不可被解讀為零。
  char *end = nullptr; value = strtod(text, &end); // 取得完整數值解析終點。
  return end != text && *end == '\0' && isfinite(value); // 所有字元都必須屬於合法有限數值。
} // 結束數值解析。

void processCommand(char *line) { // 命令區分活動安全操作與待機設定。
  char *save = nullptr, *cmd = strtok_r(line, " \t", &save); // 以空白分詞，不要求 Arduino 動態字串。
  if (!cmd) return; // 空行不執行任何動作。
  char *one = strtok_r(nullptr, " \t", &save), *two = strtok_r(nullptr, " \t", &save), *extra = strtok_r(nullptr, " \t", &save); // 最多接受命令及兩個參數。
  if (!strcmp(cmd, "STOP") && !one) { cal_mode = 0; abortTrial(USER_STOP); return; } // STOP 隨時可取消收集或試驗，但不代表物理停止落體。
  if (isRecording()) return; // 活動期間除 STOP 外不通訊、不更動量測條件。
  if (!strcmp(cmd, "HELP") && !one) { Serial.println("# HELP STATUS | DIST mm (0=no speed) | POL upper lower (0/1) | TARE | CAL known_grams | UNCAL | ARM | STOP | DUMP | CLEAR; UPPERCASE + newline; no relay"); return; } // 提供可直接操作的命令列表。
  if (!strcmp(cmd, "STATUS") && !one) { printStatus(); return; } // 狀態查詢不改動資料。
  if (!strcmp(cmd, "DUMP") && !one) { dumpTrial(); return; } // 匯出只在已完成或中止時執行。
  if (!strcmp(cmd, "CLEAR") && !one && !cal_mode) { phase = IDLE; reason = NONE; acc_count = hx_count = 0; completion_reported = false; Serial.println("# CLEARED trial RAM; settings/calibration retained"); return; } // 明確命令才捨棄舊試驗，校正設定仍保留。
  if (phase != IDLE || cal_mode) { Serial.println("# BUSY use STOP for calibration or DUMP/CLEAR for retained trial"); return; } // 固定已完成試驗的設定與校正中參數。
  double number = 0; // 建立經驗證的命令數值容器。
  if (!strcmp(cmd, "DIST") && !two && parseNumber(one, number) && number >= 0 && number <= 10000) { distance_mm = number; Serial.printf("# DIST_mm=%.6f; speed is two-point mean, not impact velocity\n", distance_mm); return; } // 限制設定範圍並保留毫米單位。
  if (!strcmp(cmd, "POL") && one && two && !extra && (!strcmp(one, "0") || !strcmp(one, "1")) && (!strcmp(two, "0") || !strcmp(two, "1"))) { upper_active = one[0] - '0'; lower_active = two[0] - '0'; Serial.printf("# POL=%d/%d verified entry levels required\n", upper_active, lower_active); return; } // 不預設兩顆反射極性必然相同。
  if (!strcmp(cmd, "UNCAL") && !one) { calibration_valid = tare_valid = false; tare_raw = counts_per_g = 0; Serial.println("# UNCAL; raw values preserved"); return; } // 明確清除 RAM 校正，而非篡改原始值。
  if (!strcmp(cmd, "ARM") && !one) { armTrial(); return; } // ARM 完成所有感測與電位檢查後才記錄。
  const bool do_tare = !strcmp(cmd, "TARE") && !one; // 去皮命令不得帶多餘參數。
  const bool do_cal = !strcmp(cmd, "CAL") && !two && parseNumber(one, number) && number > 0 && number <= 100000; // 使用者須提供真實已知克數，而非額定容量。
  if (do_tare || do_cal) { // 收集校正資料前檢查當前品質。
    if (strcmp(hxStatus(), "OK") || (do_cal && !tare_valid)) { Serial.println("# CAL_REJECT HX must be OK; CAL also needs a successful TARE"); return; } // 飽和或未去皮時不能建立校正。
    cal_mode = do_tare ? 1 : 2; cal_count = 0; cal_sum = 0; cal_mass_g = do_cal ? number : 0; cal_start_ms = millis(); // 設定非阻塞三十二筆收集。
    Serial.println(do_tare ? "# TARE_START remove external load and hold still; 32 fresh frames; STOP cancels" : "# CAL_START hold the stated known mass still; 32 fresh frames; STOP cancels"); return; // 不因開始校正就提前宣告成功。
  } // 結束校正命令。
  Serial.println("# INVALID command/arguments; use HELP"); // 未知、截斷或不合法命令不改設定。
} // 結束命令處理。

void serviceSerial() { // 限制每輪讀入量，讓持續輸入也不餓死感測器。
  uint8_t budget = 32; // 每輪最多讀取三十二個字元。
  while (budget-- && Serial.available()) { // 不使用等待整行的阻塞函式。
    const char ch = Serial.read(); // 取得目前已到達的字元。
    if (ch == '\r') continue; // 相容 Windows CRLF 及單獨 LF。
    if (ch == '\n') { // 收到完整行才執行，避免半條 ARM 被執行。
      command_line[command_length] = '\0'; // 明確終止命令字串。
      if (!command_overflow) processCommand(command_line); else if (!isRecording()) Serial.println("# INVALID command too long"); // 過長行整行捨棄。
      command_length = 0; command_overflow = false; // 為下一行重置固定緩衝區。
    } else if (command_length < sizeof(command_line) - 1) command_line[command_length++] = ch; // 僅保存有界內容。
    else command_overflow = true; // 不執行被截斷的合法命令前綴。
  } // 結束本輪序列服務。
} // 結束輸入排程。

void setup() { // 安全初始化，不啟動任何釋放輸出。
  digitalWrite(HX_SCK, LOW); pinMode(HX_SCK, OUTPUT); pinMode(HX_DATA, INPUT_PULLUP); // 沿用第四版時脈方向與空接資料弱上拉。
  pinMode(UP_PIN, INPUT); pinMode(DN_PIN, INPUT); // GPIO34／35 只有外部決定的電位，不使用不存在的內建上拉。
  Serial.begin(115200); // 保持既有 COM3 序列速率。
  rgbLedWrite(BOARD_RGB_PIN, 0, 0, 0); // 只在開機送一次全黑資料，使用核心既有RMT功能，不另裝函式庫。
  delayMicroseconds(300); // 留三百微秒低電位讓燈珠鎖存全黑，實際熄燈仍須目視確認。
  pinMode(BOARD_RGB_PIN, OUTPUT); digitalWrite(BOARD_RGB_PIN, LOW); // 關燈資料送完後將獨立GPIO32保持低，後續不再傳RGB資料。
  Serial.printf("# NMK99 RGB GPIO%u OFF requested; HX SCK=%u DATA=%u; verify wiring and LED visually\n", BOARD_RGB_PIN, HX_SCK, HX_DATA); // 不將軟體要求當作已確認實際熄燈。
  portENTER_CRITICAL(&hx_mux); digitalWrite(HX_SCK, HIGH); delayMicroseconds(80); digitalWrite(HX_SCK, LOW); portEXIT_CRITICAL(&hx_mux); // 開機只作一次 HX711 斷電重置，不在故障時反覆重置。
  hx_start_ms = millis(); // 首筆資料允許正常穩定時間。
  Wire.begin(SDA_PIN, SCL_PIN); Wire.setClock(I2C_HZ); Wire.setTimeOut(25); // 共用已驗證接腳與匯流排速度。
  initializeOled(); initializeAdxl(); // 分開初始化，缺一裝置仍保留診斷。
  attachInterrupt(digitalPinToInterrupt(UP_PIN), upperInterrupt, CHANGE); // 用變化中斷並在處理內篩選已確認極性。
  attachInterrupt(digitalPinToInterrupt(DN_PIN), lowerInterrupt, CHANGE); // 下閘獨立記時，不以畫面輪詢估通過時間。
  Serial.println("# 05-test NMK99 MEASUREMENT v1-layout2; UNCAL; SCK25 DATA33; RGB32 OFF requested; OLED SH1106 safe margins; no relay/no automatic release"); // 明確識別NMK99修訂、移腳與安全範圍。
  Serial.println("# Not fully validated. Set POL to measured entry levels; DIST in mm; ARM records only; HELP lists commands."); // 開機不猜測缺失參數。
  pollHx(); pollAdxl(); updateOled(); printStatus(); // 顯示首筆或等待狀態，不阻塞等待感測器。
} // 結束開機。

void loop() { // 以感測及試驗終止優先，顯示與匯出降到背景。
  serviceTrial(); pollHx(); pollAdxl(); serviceTrial(); serviceSerial(); // 先檢查逾時，再讀資料及處理 STOP。
  if (cal_mode && (millis() - cal_start_ms >= 8000 || strcmp(hxStatus(), "OK"))) { cal_mode = 0; Serial.println("# CAL_ABORT timeout/stale/link/SAT; old calibration unchanged"); } // 不靠等待或舊資料完成校正。
  if (!isRecording()) { // 非記錄期間才重試裝置及輸出畫面。
    if (millis() - last_retry_ms >= 3000) { last_retry_ms = millis(); if (!oled_address) initializeOled(); if (!adxl_ready) initializeAdxl(); } // 活動期間不重新初始化感測器。
    if (millis() - last_display_ms >= 250) { last_display_ms = millis(); updateOled(); } // 每四分之一秒更新八行。
    if (phase == IDLE && !cal_mode && millis() - last_live_ms >= 1000) { last_live_ms = millis(); printStatus(); } // 即時快照不混入活動擷取或 DUMP 內。
  } // 結束背景輸出。
  delay(1); // 讓系統排程執行，實際採樣間隔及中斷延遲仍待驗證。
} // 結束主迴圈。
