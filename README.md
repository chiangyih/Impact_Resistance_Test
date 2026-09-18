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

## NodeMCU-32S 腳位總表
<img width="742" height="710" alt="image" src="https://github.com/user-attachments/assets/39465e60-bec3-457f-a59a-a3e007bff801" />

這是目前採用的單一腳位配置。未來程式、接線圖與測試紀錄都應以此表為準。

| NodeMCU-32S | 對應元件端子 | 線材顏色（ADXL375／HX711） | 方向 | 用途／備註 |
|---|---|---|---|---|
| 3V3 | ADXL375 VIN、HX711 VIN、OS25B10 電路 | 黑（VCC） | 電源輸出 | 感測器邏輯電源；不可接 12 V |
| GND | ADXL375 GND、SDO、OS25B10 Emitter／LED Cathode、按鈕、Relay GND | 白（GND）；綠（SDO） | 電源回路 | 邏輯側共地，SDO 接地時使用 I²C 位址 `0x53` |
| GPIO21 | ADXL375 SDA | 橘（SDA） | I/O | I²C SDA |
| GPIO22 | ADXL375 SCL | 棕（SCL） | I/O | I²C SCL |
| GPIO16 | ADXL375 INT | 淡咖啡（INT） | 輸入（選配） | Data Ready／FIFO／事件中斷；基本輪詢可不接 |
| GPIO17 | ADXL375 I2 | — | 輸入（選配） | 第二組中斷；目前可不接 |
| GPIO32 | HX711 DATA | 紫（HX711 DATA） | 輸入 | HX711 serial data output |
| GPIO33 | HX711 SCK | 紫（HX711 SCK） | 輸出 | HX711 serial clock |
| GPIO34 | OS25B10 光閘 1 Collector 節點 | — | 輸入 | ESP32 輸入專用，沒有內建上拉 |
| GPIO35 | OS25B10 光閘 2 Collector 節點 | — | 輸入 | ESP32 輸入專用，沒有內建上拉 |
| GPIO26 | Relay IN | — | 輸出 | 電磁鐵釋放控制；程式需設定高／低觸發 |
| GPIO27 | 釋放按鈕另一端 | — | 輸入 | 按鈕另一端接 GND，使用 `INPUT_PULLUP` |

HX711 模組目前直接插在麵包板；DATA 與 SCK 分別以兩條紫線接至 ESP32 的 GPIO32 與 GPIO33。
GPIO34、GPIO35 只能作輸入，且沒有軟體內建上拉／下拉；目前麵包板的 OS25B10 輸出使用外接 6.8 kΩ 電阻。ADXL375 與 HX711 都使用 3.3 V 邏輯。NodeMCU-32S 不可把 12 V 電磁鐵電源接到 3V3、5V 或 VIN。

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

OS25B10 是四腳槽型光電開關／光遮斷器，不是具有 `VCC/GND/OUT` 的數位模組。專案中的實物參考照片為 [OS25B10-紅外線對射光電開關.png](圖片/OS25B10-紅外線對射光電開關.png)。目前整組雙光閘已拉出 4 條線，均接於麵包板：

| 線色 | 接線／用途 |
|---|---|
| 紅 | 接 180 Ω 電阻（色環：棕灰棕） |
| 黑 | GND |
| 紫 | 接 6.8 kΩ 電阻（色環：藍灰紅），對應上方 OS25B10 |
| 橘 | 接 6.8 kΩ 電阻（色環：藍灰紅），對應下方 OS25B10 |

掉落測試的計時流程：

1. 掉落物通過上方 OS25B10 時開始計時，記錄 `t_upper`。
2. 掉落物到達下方 OS25B10 時停止計時，記錄 `t_lower`。
3. 計算兩點間時間：`Δt = t_lower - t_upper`。
4. 若已知上下光閘距離 `d`，可再計算兩點間平均速度：`v = d / Δt`。

本次紀錄未自行推定電阻另一端的接法、供電電壓或輸出邏輯；OS25B10 的實際輸出波形與 ESP32 觸發邊緣仍需後續量測確認。

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

## 量測與資料品質限制

- 雙光閘應盡量靠近撞擊面；第二光閘到撞擊面的距離需固定或做修正。
- 雙光閘計算的是兩閘間平均速度，不能直接當作撞擊瞬間速度。
- HX711 的取樣率有限，主要用於不同材料組別的相對比較。
- Load Cell 應一端固定、一端受力，並在上方配置剛性上壓板。
- Load Cell 線使用雙絞線，遠離 ESP32、USB 與高電流／繼電器線路。
- ADXL375、HX711 與光閘電源附近建議配置 0.1 µF 去耦電容；HX711 電源可再加 10 µF。
- OS25B10 已完成麵包板初步接線；尚未完成 ADXL375、HX711 等其他感測器實體接線、OS25B10 輸出波形／讀值確認、Load Cell 校正或正式落下測試。
- ADXL375 應固定牢靠，避免感測器本體晃動造成假峰值。
- Load Cell 線與 I²C 線遠離 USB、Relay 與電磁鐵高電流線路。

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

### 01-irt.ino 第一版簡易測試程式

程式位置為 `01-irt/01-irt.ino`，用途是先確認 ESP32 能否收到下列元件資料：

- Adafruit ADXL375：I²C 掃描、初始化與 X／Y／Z 加速度（m/s²）。
- Adafruit HX711：A 通道增益 128 的原始 ADC 值；未校正、未換算重量。
- OS25B10 光閘一／二：GPIO34／GPIO35 的 HIGH／LOW 狀態。

本測試程式需要安裝 `Adafruit ADXL375`、`Adafruit HX711` 及其相依函式庫；Arduino CLI 可使用下列指令編譯：

```powershell
$cli = 'C:\Program Files\Arduino CLI\arduino-cli.exe'
& $cli lib install 'Adafruit ADXL375' 'Adafruit HX711'
& $cli compile --fqbn esp32:esp32:nodemcu-32s .\01-irt
& $cli upload --port COMx --fqbn esp32:esp32:nodemcu-32s .\01-irt
& $cli monitor --port COMx --config baudrate=115200
```

燒錄前先以 `board list` 確認 `COMx`；序列埠監控視窗應設定為 115200 baud。若 HX711 未就緒，程式等待 1 秒後輸出逾時並繼續顯示其他感測器；OS25B10 必須依目前接線使用外接 6.8 kΩ 電阻，不能只靠程式設定內建上拉。

## 目前檔案

- `01-irt/01-irt.ino`：第一版簡易感測器接線測試程式，所有程式碼行均附正體中文註解。
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
