# Impact_Resistance_Test

STF、三浦摺疊與 EVA 落體衝擊緩衝材料試驗平台。

本專案以 NodeMCU-32S（ESP32-WROOM-32）作為控制與資料擷取核心，使用 Adafruit ADXL375 量測掉落平台加速度，使用 Adafruit HX711 搭配荷重元量測底部相對受力，並以兩組 OS25B10 四腳槽型光電開關量測撞擊前近似速度。

## 專案目的

建立可重複的垂直落下衝擊量測系統，比較不同緩衝結構對下列指標的影響：

- 掉落物撞擊前近似速度
- ADXL375 峰值加速度與撞擊作用時間
- Load Cell 傳遞至底座的相對受力
- 緩衝材料的壓縮與永久變形

預計比較的組別包括無緩衝、EVA、PP 三浦摺疊、STF，以及 STF＋三浦摺疊＋EVA 複合結構。

## 實際元件與目前狀態

| 元件 | 目前狀態與用途 |
|---|---|
| NodeMCU-32S／ESP32-WROOM-32 | 主控制器、時間戳與 USB Serial 資料傳輸 |
| Adafruit ADXL375 | ±200 g 三軸加速度計，使用 I²C；照片中的紫色開發板與此型號相符 |
| Adafruit HX711 24-bit ADC | 讀取荷重元；使用 A 通道，RATE 開關規劃使用 80 SPS |
| 四線式 Load Cell | 照片銘牌可讀到 `CAP: 180 kg`；正式校正前仍須以銘牌／資料表確認額定容量與線色 |
| OS25B10 × 2 | 四腳槽型光電開關／光遮斷器，內含紅外線 LED 與光電晶體管；需外接 LED 限流電阻與 Collector 上拉電阻 |
| Relay＋12 V 電磁鐵 | 釋放掉落平台；Relay 型號與觸發邏輯尚待依實物標示確認 |
| 釋放按鈕 | GPIO27，使用內建上拉，按下為 LOW |
| microSD | 尚未由目前照片確認，列為後續選配資料儲存裝置 |

## NodeMCU-32S 腳位總表

這是目前採用的單一腳位配置。未來程式、接線圖與測試紀錄都應以此表為準。

| NodeMCU-32S | 對應元件端子 | 方向 | 用途／備註 |
|---|---|---|---|
| 3V3 | ADXL375 VIN、HX711 VIN、OS25B10 電路 | 電源輸出 | 感測器邏輯電源；不可接 12 V |
| GND | ADXL375 GND、HX711 GND、OS25B10 Emitter／LED Cathode、按鈕、Relay GND | 電源回路 | 邏輯側共地，建議星狀接地 |
| GPIO21 | ADXL375 SDA | I/O | I²C SDA |
| GPIO22 | ADXL375 SCL | I/O | I²C SCL |
| GPIO16 | ADXL375 INT | 輸入（選配） | Data Ready／FIFO／事件中斷；基本輪詢可不接 |
| GPIO17 | ADXL375 I2 | 輸入（選配） | 第二組中斷；目前可不接 |
| GPIO32 | HX711 DATA | 輸入 | HX711 serial data output |
| GPIO33 | HX711 SCK | 輸出 | HX711 serial clock |
| GPIO34 | OS25B10 光閘 1 Collector 節點 | 輸入 | ESP32 輸入專用，沒有內建上拉 |
| GPIO35 | OS25B10 光閘 2 Collector 節點 | 輸入 | ESP32 輸入專用，沒有內建上拉 |
| GPIO26 | Relay IN | 輸出 | 電磁鐵釋放控制；程式需設定高／低觸發 |
| GPIO27 | 釋放按鈕另一端 | 輸入 | 按鈕另一端接 GND，使用 `INPUT_PULLUP` |
| GPIO18 | microSD SCK（選配） | 輸出 | VSPI clock |
| GPIO19 | microSD MISO（選配） | 輸入 | VSPI MISO |
| GPIO23 | microSD MOSI（選配） | 輸出 | VSPI MOSI |
| GPIO5 | microSD CS（選配） | 輸出 | VSPI CS；GPIO5 是啟動相關腳位，SD 模組 CS 應保持上拉 |

GPIO34、GPIO35 只能作輸入，且沒有軟體內建上拉／下拉；OS25B10 必須各自使用外接 10 kΩ 上拉電阻。ADXL375 與 HX711 都使用 3.3 V 邏輯。NodeMCU-32S 不可把 12 V 電磁鐵電源接到 3V3、5V 或 VIN。

## 電源與接地

- NodeMCU-32S 由 USB 供電；感測器由 NodeMCU-32S 的 3V3 供電。
- ADXL375、HX711、OS25B10、按鈕與 Relay 的邏輯側共用 GND。
- 電磁鐵使用獨立 12 V 電源；不要由 ESP32 腳位或 USB 直接供電。
- Relay 模組的 VCC 需依實物標示接額定電源；目前規劃以 5 V Relay 模組為基準。若為非隔離模組，Relay GND 與 ESP32 GND 共地。
- 12 V 高電流線、Relay 線與 Load Cell／I²C 線分開走線，並以星狀方式回到電源地，降低衝擊雜訊。

## ADXL375（Adafruit，I²C）

照片中的紫色板為 Adafruit ADXL375 高 G 加速度計。板上 I²C 預設位址為 `0x53`；I²C 的 SDA、SCL 已有板載上拉與電平轉換，NodeMCU-32S 使用 3.3 V 即可。

| ADXL375 端子 | NodeMCU-32S | 說明 |
|---|---|---|
| VIN | 3V3 | 供電；依板面標示使用 `VIN`，不是 `3Vo` |
| GND | GND | 共地 |
| SDA | GPIO21 | I²C 資料 |
| SCL | GPIO22 | I²C 時脈 |
| CS | 3V3 | I²C 模式；此板預設已拉高，若保持原板設定可不另接 |
| SDO | GND | 使用 I²C 位址 `0x53`；接 3V3 則為 `0x1D` |
| INT | GPIO16（選配） | Data Ready／FIFO／事件中斷 |
| I2 | GPIO17（選配） | 第二組中斷，目前可不接 |
| 3Vo | 不接 | 板上穩壓器的 3.3 V 輸出，不可當作電源輸入 |

ADXL375 應牢固固定在掉落平台上，盡量靠近重心。程式使用 I²C 輪詢時可不接 INT／I2；若要用 Data Ready 或 FIFO 中斷，再接 GPIO16。正式上電前仍應以 I²C scanner 確認 `0x53`。

## HX711（Adafruit）與 Load Cell

照片中的黑色板為 Adafruit HX711 24-bit ADC，板上端子名稱為 `E-`、`A-`、`A+`、`B+`、`B-`、`E+`，邏輯端子為 `VIN`、`GND`、`DATA`、`SCK`、`RATE`。單一四線式荷重元使用 A 通道，B 通道留空。

### HX711 與 NodeMCU-32S

| HX711 端子 | NodeMCU-32S | 說明 |
|---|---|---|
| VIN | 3V3 | 供電；依實物標示使用 `VIN` |
| GND | GND | 共地 |
| DATA | GPIO32 | 資料輸出 |
| SCK | GPIO33 | 時脈輸入 |
| RATE | 不接 GPIO | 使用板上滑動開關；切到 `H` 為 80 SPS，`L` 為 10 SPS |
| VIO（若板上有引出） | 不接 | 板上數位電源穩壓輸出，不是 NodeMCU 電源輸入 |

### Load Cell 與 HX711 端子

| Load Cell 功能／目前照片線色 | HX711 端子 | 備註 |
|---|---|---|
| 激勵正／紅線（暫定） | E+ | 以實際規格或電阻量測確認 |
| 激勵負／黑線（暫定） | E- | 以實際規格或電阻量測確認 |
| 訊號正／綠線（暫定） | A+ | HX711 A 通道 |
| 訊號負／白線（暫定） | A- | HX711 A 通道 |
| B 通道 | B+、B- | 單一荷重元不接 |

紅／黑／綠／白只可作目前接線起點，正式上電前須依荷重元資料表或電阻量測確認。照片銘牌可讀到 `CAP: 180 kg`，因此校正與機構安全檢查暫以 180 kg 額定容量作為待確認資料，不把它當作已完成規格驗證。

Load Cell 機械上應一端固定、一端受力，上方設置剛性壓板；不可將整支樑完全夾死。HX711 最高 80 SPS，適合組別間相對受力比較，不應宣稱為毫秒級真實瞬時峰值力。

## OS25B10 雙光閘

OS25B10 是四腳槽型光電開關／光遮斷器，不是具有 `VCC/GND/OUT` 的數位模組。專案中的實物參考照片為 [OS25B10-紅外線對射光電開關.png](圖片/OS25B10-紅外線對射光電開關.png)。每顆元件需要一個 LED 限流電阻與一個光電晶體管上拉電阻：

```text
每一顆 OS25B10 各自使用一組電阻；兩組輸出節點不可互相短接：

光閘 1：
3.3 V ── 220 Ω ── IR LED Anode
IR LED Cathode ── GND
3.3 V ── 10 kΩ ──┬── Collector
                 └── 輸出節點 ── GPIO34
Emitter ── GND

光閘 2：
3.3 V ── 220 Ω ── IR LED Anode
IR LED Cathode ── GND
3.3 V ── 10 kΩ ──┬── Collector
                 └── 輸出節點 ── GPIO35
Emitter ── GND
```

- 光閘 1 的 Collector 輸出節點接 GPIO34。
- 光閘 2 的 Collector 輸出節點接 GPIO35。
- GPIO34／GPIO35 沒有內建上拉，10 kΩ 必須外接，而且只能拉到 3.3 V。
- 預期未遮光時光電晶體管導通、輸出 LOW；遮光時輸出 HIGH。實際極性仍須在單獨測試時確認。
- OS25B10 四根腳的實體順序目前尚未完成確認。焊接前先用萬用電表二極體檔找出 IR LED 的正負腳，再確認 Collector／Emitter；不可只依封裝外觀猜測。
- 不可將 LED 直接接到 3.3 V，也不可把任何光閘輸出上拉至 5 V。

程式以 `micros()` 記錄兩個 GPIO 的相同邊緣，計算 `v_gate = d / (t2 - t1)`。這是兩閘間平均速度，應稱為「撞擊前近似速度」，不是精確撞擊瞬間速度。

## Relay、電磁鐵與釋放按鈕

### Relay 與 12 V 電磁鐵

Relay 模組的實際型號與觸發邏輯尚待從實物標示確認；目前預留接法如下：

| Relay 端子 | NodeMCU-32S／電源 | 說明 |
|---|---|---|
| IN | GPIO26 | 程式以 `RELAY_ACTIVE_LOW` 選擇高／低觸發 |
| GND | GND | 非隔離模組需共地 |
| VCC | 5 V／VIN 或模組額定電源 | 以 Relay 實物標示為準，不由 GPIO 供電 |

電磁鐵負載側：

```text
獨立 12 V+ ── Relay COM
Relay NO ── 電磁鐵正極
電磁鐵負極 ── 獨立 12 V−
```

電磁鐵兩端並聯飛輪二極體：陰極（有色環端）接電磁鐵正極，陽極接電磁鐵負極。電磁鐵應固定在上方支架，不裝在移動掉落平台上。

### 釋放按鈕

```text
GPIO27 ── 釋放按鈕 ── GND
```

程式設定 `pinMode(27, INPUT_PULLUP)`，按下時讀值為 LOW，並加入按鍵去彈跳。

## microSD（後續選配）

目前尚未從照片確認 microSD 模組，因此不列入已驗證硬體。若後續加入 3.3 V 邏輯相容的 SPI 模組，預留：

| microSD 端子 | NodeMCU-32S |
|---|---:|
| SCK | GPIO18 |
| MISO | GPIO19 |
| MOSI | GPIO23 |
| CS | GPIO5 |
| VCC | 3V3（依模組規格） |
| GND | GND |

GPIO5 是 ESP32 啟動相關腳位，microSD 的 CS 必須在開機時保持適當的高電位；正式接入前要確認模組沒有把 CS 強制拉低。若模組接受 5 V 供電，仍必須確認其輸出到 ESP32 的邏輯不超過 3.3 V。

## 量測與資料品質限制

- 雙光閘應盡量靠近撞擊面；第二光閘到撞擊面的距離需固定或做修正。
- 雙光閘計算的是兩閘間平均速度，不能直接當作撞擊瞬間速度。
- HX711 的取樣率有限，主要用於不同材料組別的相對比較。
- ADXL375 應固定牢靠，避免感測器本體晃動造成假峰值。
- Load Cell 線與 I²C 線遠離 USB、Relay 與電磁鐵高電流線路。
- 目前尚未完成整體實體接線、感測器讀值、Load Cell 校正或正式落下測試。

## Arduino CLI 編譯與燒錄

本專案使用下列 Arduino CLI 執行檔：

```text
C:\Program Files\Arduino CLI\arduino-cli.exe
```

PowerShell 範例：

```powershell
$cli = 'C:\Program Files\Arduino CLI\arduino-cli.exe'
& $cli board list
& $cli compile --fqbn esp32:esp32:nodemcu-32s <sketch-folder>
& $cli upload --port COMx --fqbn esp32:esp32:nodemcu-32s <sketch-folder>
```

正式編譯前先以 `board list` 確認實際 COM 埠，並確認已安裝 ESP32 Arduino core 與 `nodemcu-32s` FQBN。

## 目前檔案

- `handoff.md`：完整專題交接紀錄、設計背景與接線修訂。
- `auto-git-watch.ps1`：檔案變更自動 commit／push 監看器。
- `圖片/`：目前取得的元件與機構照片。
- `零件1.stp`：STEP 機械模型。
- `零件1.stl`：STL 網格模型。

## Git 自動同步

遠端儲存庫：<https://github.com/chiangyih/Impact_Resistance_Test>

目前已設定目前 Windows 使用者登入時啟動自動監看器。監看器每 5 秒檢查變更，等待 3 秒後自動建立 commit 並推送至 `origin/main`；不使用 force push。紀錄位於：

```text
%LOCALAPPDATA%\Impact_Resistance_Test\auto-git-watch.log
```

所有新增或修改的檔案都可能被自動提交，請勿將密碼、Token、私鑰或其他秘密放入此專案資料夾。

## 後續工作

1. 用萬用電表確認兩顆 OS25B10 的 LED／Collector／Emitter 實體腳位。
2. 完成雙光閘輸出波形與必要的比較器／施密特觸發器評估。
3. 以 I²C scanner 與 Adafruit Arduino library 完成 ADXL375 單獨讀值測試。
4. 依 Load Cell 規格確認線色、額定容量，完成 HX711 靜態校正。
5. 確認 Relay 模組額定電壓與 HIGH／LOW trigger，完成電磁鐵空載釋放測試。
6. 完成 ESP32 時間同步、雙光閘中斷與資料記錄程式。
7. 先進行低高度、低重量測試，再進入緩衝材料比較實驗。

## 參考資料

- [Adafruit HX711 24-bit ADC](https://learn.adafruit.com/adafruit-hx711-24-bit-adc)
- [Adafruit HX711 Pinouts](https://learn.adafruit.com/adafruit-hx711-24-bit-adc/pinouts)
- [Adafruit ADXL375 Pinouts](https://learn.adafruit.com/adafruit-adxl375/pinouts)
- [Seeed Studio OS25B10 Photo Interrupter](https://wiki.seeedstudio.com/ja/Photo_interrupter_OS25B10/)
- [Espressif Arduino-ESP32 NodeMCU-32S variant](https://github.com/espressif/arduino-esp32/blob/master/variants/nodemcu-32s/pins_arduino.h)
- [Espressif ESP32-WROOM-32 datasheet](https://documentation.espressif.com/esp32-wroom-32_datasheet_en.html)
