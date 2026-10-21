# 落體衝擊量測平台（Impact_Resistance_Test）

本專案是以ESP32為核心的垂直落體衝擊量測平台，用來比較STF、三浦摺疊、EVA及複合緩衝結構的衝擊反應。**2026-10-20使用者確認實際主板為NMK99v1二合一開發板**；早期NodeMCU-32S稱呼保留供歷史追溯，`esp32:esp32:nodemcu-32s`仍是沿用的ESP32編譯設定，不代表已完整對應NMK99所有板載周邊。

系統目前規劃以三類感測資料互相對照：

- Adafruit ADXL375：量測掉落平台的三軸高 G 加速度。
- HX711 搭配四線式 Load Cell：量測底部相對受力。
- 上、下兩顆 Tracker Sensor：取代 OS25B10，已通過使用者在 3V3 下的兩路獨立慢速切換測試；以兩點時間差估算平均速度的正式程式與高速驗證尚未完成。

四腳 I²C OLED 顯示上述感測器讀值；`02-irt_oled` 與新版 `03-irt_tracker` 使用 SH1106 128×64 設定，使用者已分別確認兩版八行資料正常且無缺邊。第三版已編譯、燒錄到 COM3，使用者確認兩顆 Tracker 改接 3V3 後可各自使 UP／DN 切換，慢速反射功能通過目視測試；高速通過及正式計時仍待驗證。程式自動檢查 `0x3C`／`0x3D`，本機實測位址為 `0x3C`。

## 目前狀態

> **NMK99版第五版重要移腳：**`05-test/05-test.ino`最新原始碼使用 **HX711 SCK＝GPIO25、DATA＝GPIO33**，GPIO32留給板載WS2812，開機要求全黑並保持低電位。使用者已確認未接相機／TFT／SD，且已斷電改好SCK25後重新接USB。間距維持225mm，觸發極性仍未設定、重量尚未校正；不自動釋放落體，不啟用Relay。**舊01～04與初版05的HX接腳不適用目前接線，不可直接燒錄。**

> **最新狀態：2026-10-20（NMK99版05已燒錄；HX711與ADXL375十三筆快照均OK，使用者確認WS2812已熄滅、OLED八行正常無缺邊；尚未校正）**
>
> **2026-10-20 荷重元接線重要更正：**本案實測為 **黑→E+、白→E−、綠→A+、紅→A−**，B+／B− 不接。舊版暫定的紅→E+、黑→E−、綠→A+、白→A−不再作為本案接線指引。改接後約15秒、15筆序列快照全部為 `OK`，RAW＝`-161472～-161287`、RX＝`1748→1879`、ERR＝`0`，未見 SAT／STALE／TIMEOUT。**尚未校正，RAW 不是公斤或牛頓；也尚未完成長時間與已知荷重驗證。**這是本案荷重元的實測接法，不是可套用其他型號的通用線色。
>
> **2026-10-20 第四版獨立測試：**`04-HX711-test/04-HX711-test.ino` 已編譯、COM3 燒錄、OLED 八行目視及空接基準驗證，之後完成上述 HX711 接回測試。**SCK＝GPIO32、DATA＝GPIO33，與 `01`～`03` 相反**；OLED 維持 SDA＝GPIO21、SCL＝GPIO22、SH1106 `0x3C`。本次 SAT 排查只調整硬體接線，沒有修改程式、去皮、濾波或重燒。
>
> **VIO 供電更正與後續量測：**VIO 接 HX711 DVDD，是數位供電輸入，不是可懸空的輸出；VIN、VIO 均接同一個 ESP32 3V3。使用者後續回報 VIN／VIO／GND 三條完整連接路徑各為0Ω，ESP32 3V3＝3.29V、HX711 VIN＝3.20V、VIO＝3.20V；SCK→GPIO32、DATA→GPIO33 完整路徑重測亦各為0Ω。這些更新取代下方較早的待測狀態，但不等於供電波形、瞬態或長期可靠性已驗證。
>
> `03-irt_tracker` 的 OLED、三軸與雙 Tracker 慢速切換已有歷史驗證；當時 HX711 曾出現正負 SAT、STALE。最新解除 SAT 的觀察是在第四版及上述新荷重元接法下取得，尚未切回第三版做整合回歸測試。
>
> **2026-10-17 硬體更新：**使用者已購買兩顆相同 Tracker Sensor，回報 VCC 改接 3V3、上方 OUT 接 GPIO34、下方 OUT 接 GPIO35，並確認兩路可各自慢速反射切換。正式型號、額定供電範圍、實際 OUT 電壓與觸發極性仍待補查；不能把慢速功能測試當成完整電氣或高速驗證。舊 `01-irt`／`02-irt_oled` 保留 OS25B10 命名與韌體。
>
> **2026-10-17 第三版程式：**由 `02-irt_oled` 另存，保留 OLED、ADXL375、HX711 並改讀兩路 Tracker Sensor 原始 HIGH／LOW。初次僅編譯，之後依使用者新的要求完成自動編譯及燒錄；Flash 312,612 bytes（23%）、RAM 24,924 bytes（7%）。目前完成燒錄、OLED 與兩路慢速反射目視驗證；模組額定規格、實際 OUT 電壓與高速計時精度仍未驗證。

目前完成：

- ADXL375、HX711、舊版 OS25B10 接線，以及新版 Tracker Sensor 的待驗證接線規劃已記錄。
- `01-irt` 簡易程式已成功編譯、燒錄並以 115200 baud 讀取序列輸出。
- ADXL375 已讀到 X／Y／Z 加速度，HX711 已讀到 A128 原始值。
- 兩顆 Tracker Sensor 已由使用者確認接到上方 GPIO34、下方 GPIO35。
- `02-irt_oled` 已加入 OLED 同頁讀值、逐行正體中文註解與資料狀態，並完成編譯、燒錄與序列埠檢查。
- 使用者已目視確認 `02-irt_oled` 完整版的八行資料、底部狀態列均正常顯示且無缺邊。
- `03-irt_tracker` 已逐行附正體中文註解，完成編譯、COM3 燒錄、啟動輸出與 OLED 八行目視驗證；上方 OUT 使用 GPIO34，下方 OUT 使用 GPIO35，保留實際 HIGH／LOW、不預設反射極性。
- 使用者已確認兩顆 Tracker VCC 改接 3V3，緩慢靠近／移開反射標記時，上方只使 UP 切換、下方只使 DN 切換。
- `04-HX711-test` 已編譯、COM3 燒錄校驗、八行 OLED 目視與空接基準確認，並在實測荷重元接法下完成約15秒觀察，15筆快照全為 OK、ERR＝0，未見 SAT／STALE／TIMEOUT；尚未校正。
- NMK99版`05-test`已改為SCK25／DATA33、RGB32，18組主機檢查與編譯／COM3燒錄通過。最新約12秒、13筆快照HX711及ADXL375均OK；使用者確認RGB已熄滅、OLED八行正常無缺邊。仍未ARM或校正，Tracker極性未設定。

尚待完成：

- 補查 Tracker Sensor 額定供電範圍、輸出形式、OUT 電壓與反射觸發極性；兩路已接妥且通過使用者慢速切換測試，不等於完整電氣或高速性能驗證。
- HX711 已於第四版15秒觀察中解除 SAT；接續需固定安裝、已知小荷重反應測試、靜態校正與長時間穩定性驗證，並另做第三版整合回歸。
- 上、下反射偵測點的安裝、觸發順序、重複性與兩點時間差正式測試。
- Load Cell 原廠型號／額定容量、受力正負方向與 HX711 靜態校正的確認；本案實測線色接法已記錄，不等於取得原廠通用配線定義。
- Relay、12 V 電磁鐵與釋放按鈕的實機安全測試。
- 正式低高度、低重量測試，以及後續資料記錄程式。

## 目錄

1. [專案目的](#專案目的)
2. [檔案結構](#檔案結構)
3. [系統架構](#系統架構)
4. [硬體清單與狀態](#硬體清單與狀態)
5. [NodeMCU-32S 接腳配置表](#nodemcu-32s-接腳配置表)
6. [電源與接地](#電源與接地)
7. [ADXL375 接線](#adxl375-接線)
8. [HX711 與 Load Cell 接線](#hx711-與-load-cell-接線)
9. [Tracker Sensor 雙偵測點](#tracker-sensor-雙偵測點)
10. [Relay、電磁鐵與釋放按鈕](#relay電磁鐵與釋放按鈕)
11. [量測設計與限制](#量測設計與限制)
12. [01-irt 通訊測試程式](#01-irt-通訊測試程式)
13. [02-irt_oled OLED 簡易測試程式](#02-irt_oled-oled-簡易測試程式)
14. [03-irt_tracker Tracker Sensor 簡易測試程式](#03-irt_tracker-tracker-sensor-簡易測試程式)
15. [04-HX711-test 獨立簡易測試程式](#04-hx711-test-獨立簡易測試程式)
16. [05-test 整合量測程式](#05-test-整合量測程式)
17. [後續工作](#後續工作)
18. [參考資料](#參考資料)

## 專案目的

建立可重複的垂直落下衝擊量測系統，比較不同緩衝結構對下列指標的影響：

- 掉落物通過上下偵測點的平均速度。
- ADXL375 的峰值加速度與撞擊作用時間。
- Load Cell 傳遞至底座的相對受力。
- 緩衝材料的壓縮與永久變形。

預計比較的組別包括無緩衝、EVA、PP 三浦摺疊、STF，以及 STF＋三浦摺疊＋EVA 複合結構。

## 檔案結構

```text
Impact_Resistance_Test/
├── 01-irt/
│   └── 01-irt.ino                         # ESP32 感測器通訊測試程式
├── 02-irt_oled/
│   └── 02-irt_oled.ino                    # OLED 與序列埠感測器讀值測試程式
├── 03-irt_tracker/
│   └── 03-irt_tracker.ino                 # 已燒錄；雙 Tracker 慢速切換與 OLED 已目視確認
├── 04-HX711-test/
│   └── 04-HX711-test.ino                  # HX711 單獨測試與 OLED；SCK32／DATA33
├── 05-test/
│   └── 05-test.ino                        # 整合量測、Tracker 中斷計時、手動校正及 CSV
├── tests/
│   └── 05-test.test.cjs                   # 從第五版來源轉譯的主機邏輯模擬，不是硬體時序測試
├── 圖片/
│   ├── OS25B10-紅外線對射光電開關.png       # OS25B10 元件參考圖
│   ├── oled.png                            # 四腳 I²C OLED 實物照片
│   ├── Snipaste_2026-10-17_22-18-33.png     # Tracker Sensor 接腳標示面
│   ├── Snipaste_2026-10-17_22-18-47.png     # Tracker Sensor 元件面
│   ├── PXL_*.jpg                           # 元件、麵包板與機構照片
│   └── PXL_*.MP.jpg                        # 元件、麵包板與機構照片
├── .gitignore                               # Git 忽略規則；排除本機自動同步腳本
├── README.md                               # 專案入口說明、接線與操作方式
├── handoff.md                              # 依日期追加的完整交接與驗證紀錄
├── auto-git-watch.ps1                      # 本機限定；不納入 GitHub
├── 零件1.stp                               # 機構 STEP 模型
└── 零件1.stl                               # 機構 STL 網格模型
```

### 檔案使用原則

- `README.md`：閱讀專案目前架構、接線、測試指令與已知限制的入口文件。
- `handoff.md`：保留時間順序與歷史狀態；新的量測、燒錄或接線結果應追加在檔案末端，不覆寫舊紀錄。
- `01-irt/01-irt.ino`：目前只用於確認感測器與 ESP32 的基本通訊，不是正式落下試驗程式。
- `02-irt_oled/02-irt_oled.ino`：以 `01-irt` 為基礎另存的 OLED 簡易測試程式，同時顯示 ADXL375、HX711 與兩路 OS25B10；每個非空程式碼行均附正體中文註解。
- `03-irt_tracker/03-irt_tracker.ino`：由 `02-irt_oled` 另存，保留其他感測器與 OLED 功能，改為兩顆 Tracker Sensor；每個非空行均有正體中文註解。已完成燒錄、OLED 與 Tracker 兩路慢速切換目視驗證；原 `01`、`02` 保留不動。
- `auto-git-watch.ps1`：僅供本機自動同步使用，已加入 `.gitignore`，不應提交到 GitHub。
- `圖片/`：保存目前取得的實物與機構照片；照片不能取代電表量測或資料表確認。
- `零件1.stp`、`零件1.stl`：保存機構設計模型，與 ESP32 韌體分開管理。

## 系統架構

```text
USB 供電／序列埠
        │
        ▼
NMK99v1（ESP32；相機／TFT／SD未接）
   ├─ I²C GPIO21／GPIO22 ── ADXL375、OLED
   ├─ GPIO25／GPIO33 ─────── HX711 SCK／DATA ── Load Cell
   ├─ GPIO32 ─────────────── 板載WS2812（開機要求全黑）
   ├─ GPIO34 ─────────────── 上方 Tracker Sensor OUT（慢速切換已確認）
   ├─ GPIO35 ─────────────── 下方 Tracker Sensor OUT（慢速切換已確認）
   ├─ GPIO26（預留）──────── Relay ── 獨立 12 V 電磁鐵
   └─ GPIO27（預留）──────── 釋放按鈕
```

資料可由 ESP32 透過 USB Serial 輸出；第二至第五版均提供 OLED 畫面。`01`～`04` 保留為各階段簡易測試；第五版加入中斷計時與試驗原始記錄，仍未完成高速／動態量測驗證或荷重校正，也不控制 Relay。裝置最新燒錄與實測狀態依各版驗證紀錄為準。

## 硬體清單與狀態

| 元件 | 用途 | 目前狀態 |
|---|---|---|
| NMK99v1／ESP32 | 主控制器、時間戳與USB Serial | 使用者已確認實際板型及未接相機／TFT／SD；沿用nodemcu-32s編譯設定，最新05將HX SCK移25避開RGB32 |
| Adafruit ADXL375 | ±200 g 三軸加速度計，I²C | NMK99新版05近期三軸交易成功、13筆STATUS均OK、AERR＝0；未完成加速度校正與動態驗證 |
| 四腳 I²C OLED | 顯示整合資料或 HX711 診斷 | 新版位址`0x3C`、SH1106 128×64、4x6字型；使用者確認八行正常、無錯位或缺邊 |
| NMK99板載WS2812 | 全彩燈 | GPIO32已與HX時脈分開，新版開機送全黑；使用者確認已熄滅 |
| Adafruit HX711 24-bit ADC | 讀取 Load Cell | 第四版以黑E+／白E−／綠A+／紅A−接回，15筆快照全OK、ERR＝0，未見SAT／STALE／TIMEOUT；尚未校正與長時間驗證 |
| 四線式 Load Cell | 底部相對受力 | 本案實測配對已通過第四版15秒SAT解除觀察；照片可讀到`CAP: 180 kg`，原廠型號／額定容量與受力方向仍待確認 |
| Tracker Sensor × 2 | 新版上、下反射偵測點 | 使用者確認 OUT 接 GPIO34／35、VCC 已改接 3V3，兩路可各自反射切換；實際 OUT 電壓與高速性能仍待量測 |
| OS25B10 × 2（舊版） | 舊版上、下光閘 | 保留歷史接線與程式；曾讀到兩路 LOW，不等於 Tracker Sensor 已通過測試 |
| Relay＋12 V 電磁鐵 | 釋放掉落平台 | 接線與觸發邏輯尚待依實物確認 |
| 釋放按鈕 | 啟動釋放流程 | 預留 GPIO27，尚未納入 `01-irt` |

## NodeMCU-32S 接腳配置表

下表是 `01`～`03` 的整合接線及 2026-10-17 Tracker Sensor 換裝回報；兩路在 3V3 下已通過使用者慢速切換測試，實際 OUT 電壓等電氣資料仍待補充。新 `03-irt_tracker` 已採用 Tracker 命名，舊 `01`、`02` 仍保留 OS25B10 名稱。**本表 HX711 DATA＝32／SCK＝33 不適用於第四版；`04-HX711-test` 改為 DATA＝33／SCK＝32，詳見第四版專用接線表，切換前須斷電核對。**

**此表與下方NodeMCU腳位圖均為歷史參考，不是NMK99最新第五版接線。**新版HX711為SCK25／DATA33、RGB32；使用者已改線，需用新版05。不得只換回舊韌體而不核對實線，尤其舊SCK33會與目前HX711 DATA33輸出相衝突。

| NodeMCU-32S | 對應元件端子 | 線材顏色／接線 | 方向 | 用途與備註 |
|---|---|---|---|---|
| 3V3 | ADXL375 VCC、OLED VCC、HX711 VIN 與 VIO、兩顆 Tracker Sensor VCC | ADXL375 黑：VCC；新模組線色未指定 | 電源輸出 | HX711 VIN／VIO 同接 3V3，不依賴背面焊橋；Tracker 額定範圍與實際電壓待補查；不可接 12 V |
| GND | ADXL375 GND、SDO、OLED GND、HX711 GND、兩顆 Tracker Sensor GND | ADXL375 白：GND；綠：SDO；新模組線色未指定 | 電源回路 | 新模組規劃共地；SDO 接低電位時，程式預期 I²C 位址為 `0x53` |
| GPIO21 | ADXL375 SDA、OLED SDA | ADXL375 橘：SDA；OLED：— | I/O | I²C SDA，共用匯流排 |
| GPIO22 | ADXL375 SCL、OLED SCK | ADXL375 棕：SCL；OLED：— | I/O | I²C SCL；OLED 實物端子標示為 SCK |
| GPIO16 | ADXL375 INT | ADXL375 淡咖啡：INT | 輸入（選配） | Data Ready／FIFO／事件中斷；目前程式使用輪詢 |
| GPIO17 | ADXL375 I2 | — | 輸入（選配） | 第二組中斷；目前不接 |
| GPIO32 | HX711 DATA | 紫：HX711 DATA | 輸入 | HX711 serial data output；HX711 直接插在麵包板 |
| GPIO33 | HX711 SCK | 紫：HX711 SCK | 輸出 | HX711 serial clock；HX711 直接插在麵包板 |
| GPIO34 | 上方 Tracker Sensor OUT | 新線色未指定；舊版 OS25B10 為紫線 | 輸入 | 使用者確認 UP 可獨立切換；OUT 電壓待量測，沒有內建上拉 |
| GPIO35 | 下方 Tracker Sensor OUT | 新線色未指定；舊版 OS25B10 為橘線 | 輸入 | 使用者確認 DN 可獨立切換；OUT 電壓待量測，沒有內建上拉 |
| GPIO26 | Relay IN | — | 輸出（預留） | 電磁鐵釋放控制；觸發高低電位尚待確認 |
| GPIO27 | 釋放按鈕另一端 | — | 輸入（預留） | 按鈕另一端接 GND，規劃使用 `INPUT_PULLUP` |

### NodeMCU-32S 腳位圖

![NodeMCU-32S 腳位參考圖](https://github.com/user-attachments/assets/39465e60-bec3-457f-a59a-a3e007bff801)

![NodeMCU-32S 腳位配置參考圖](https://github.com/user-attachments/assets/d8f0d139-59c4-497e-bd9b-5dd688afef42)

## 電源與接地

- NodeMCU-32S 目前由 USB 供電；既有 ADXL375、OLED、HX711 使用 NodeMCU-32S 的 3V3。新 Tracker Sensor 須先確認支援 3.3 V，不能直接套用其他型號規格。
- HX711 的 VIN 與 VIO 都須有適當供電；本案統一接同一個 3V3。VIO 不是可隨意懸空的穩壓器輸出，背面焊橋是否閉合仍須核對。
- ADXL375、OLED、HX711、Tracker Sensor、按鈕與 Relay 邏輯側應共用 GND。
- 本次已由使用者確認 OLED VCC 接 3V3、GND 共地；更換模組前仍須確認其實際供電範圍。
- 電磁鐵使用獨立 12 V 電源，不可由 ESP32 GPIO、USB 或 3V3 直接供電。
- Relay 的 VCC 必須依實物標示接額定電源；非隔離模組才需要依模組電路與 ESP32 共地。
- 12 V 高電流線、Relay 線與 Load Cell／I²C 線分開走線，降低衝擊雜訊。
- GPIO34、GPIO35 沒有內建上拉／下拉。新模組是否需要外接上拉，取決於 OUT 電路；若確認為開集極且無板載合適上拉，才加上拉到 3V3。不可沿用舊版電阻網路而未核對。

## ADXL375 接線

ADXL375 使用 I²C。程式目前以 `0x53` 初始化，這代表 SDO 應為低電位；若 SDO 接 3V3，位址會變成 `0x1D`，需同步修改程式或接線。

| ADXL375 端子 | NodeMCU-32S | 說明 |
|---|---|---|
| VIN | 3V3 | 供電；依板面標示使用 `VIN`，不是 `3Vo` |
| GND | GND | 共地；線色為白 |
| SDA | GPIO21 | I²C 資料；線色為橘 |
| SCL | GPIO22 | I²C 時脈；線色為棕 |
| CS | 3V3 | 選擇 I²C 模式；實物接法仍應確認 |
| SDO | GND | 使用 I²C 位址 `0x53`；線色為綠 |
| INT | GPIO16（選配） | Data Ready／FIFO／事件中斷；線色為淡咖啡 |
| I2 | GPIO17（選配） | 第二組中斷，目前不接 |
| 3Vo | 不接 | 板上穩壓器的 3.3 V 輸出，不作為電源輸入 |

ADXL375 應牢固固定在掉落平台上，盡量靠近重心。正式上電前，應以 I²C scanner 或目前測試程式確認 `0x53` 有回應。

## OLED 顯示模組（I²C，規格待確認）

實物照片：[oled.png](圖片/oled.png)。

![四腳 I²C OLED 實物照片](圖片/oled.png)

照片可辨識四個端子標示為 `1 VCC`、`2 GND`、`3 SCK`、`4 SDA`。此處將 `SCK` 視為 I²C SCL 時脈腳位；目前不能只由照片確認 OLED 控制器、尺寸、解析度、供電範圍或 I²C 位址。

| OLED 端子 | NodeMCU-32S | 備註 |
|---|---|---|
| `VCC` | `3V3` | 使用者已確認改接，SH1106 完整版八行畫面正常 |
| `GND` | `GND` | 與 ESP32、ADXL375、HX711 共地 |
| `SCK` | GPIO22 | 視為 I²C SCL，與 ADXL375 共用 |
| `SDA` | GPIO21 | I²C SDA，與 ADXL375 共用 |

OLED 與 ADXL375 共用 GPIO21／GPIO22 的 I²C 匯流排。`02-irt_oled` 會列出回應位址，並在 `0x3C`／`0x3D` 中選擇 OLED 候選位址（兩者都有回應時優先 `0x3C`）；ADXL375 維持 `0x53`。I²C 回應不會識別 OLED 控制器或解析度，實際畫面仍須目視確認。原有 `01-irt` 保留感測器測試功能，沒有 OLED 驅動。

本次排查時，使用者確認 SDA 接 GPIO21、SCK 接 GPIO22、GND 共地，並已斷電將 VCC 從 5V 改接 3V3；之後確認 SH1106 專用診斷畫面正常且無缺邊。供電、斷電重啟與驅動設定均有變化，尚不能判定最初黑畫面的單一根因。模組完整規格與板上上拉接法仍未知，不可只因模組可由 5V 供電就推定其 I²C 訊號適合直連 ESP32。ESP32 輸入高電位規格上限為所屬電源域的 `VDD + 0.3 V`，詳見 [Espressif 電氣特性表](https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf)。

## HX711 與 Load Cell 接線

HX711模組插在麵包板，第四版歷史接法為SCK32／DATA33；**目前NMK99版05已改SCK25／DATA33**，GPIO32留給板載WS2812。荷重元仍沿用本節實測接法，先前第四版15秒觀察解除SAT；各版使用A128。新版05預設保留RAW，只有手動完成CAL_USER才輸出靜態等效克數；目前仍尚未校正。

### HX711 與 NodeMCU-32S

以下為`01`～`03`歷史接法；第四版歷史上將DATA／SCK對調。**兩者均非目前NMK99版05的SCK25／DATA33接法**，最新完整對照見第五版小節。VIN／VIO仍同接3V3。

| HX711 端子 | NodeMCU-32S | 說明 |
|---|---|---|
| VIN | 3V3 | 供電；依實物標示使用 `VIN` |
| GND | GND | 共地 |
| DATA | GPIO32 | 資料輸出；紫線 |
| SCK | GPIO33 | 時脈輸入；紫線 |
| RATE | 不接 GPIO | 使用板上開關；`H` 為 80 SPS、`L` 為 10 SPS |
| VIO | 3V3 | 數位供電 DVDD 輸入；與 VIN 同接 3V3，不保持懸空 |

### VIO 數位供電更正與優先檢查

先前本文件將 VIO 誤列為輸出且不接，現已更正。HX711 原廠資料表將第 16 腳 DVDD 定義為數位電源；Adafruit 官方原理圖顯示 `VDDIO` 連至 HX711 的 DVDD、排針 JP1 第 2 腳、去耦 C2 與焊橋 SJ1，沒有連到類比穩壓器輸出。SJ1 的另一端連 VIN，官方說明也指出背面焊橋預設開路。

因此，**VIO 不接且 SJ1 開路時，DVDD 缺少正常供電**，曾列為數位輸出不可靠的優先疑點。使用者後續已補接 VIO 並量得約3.20V；實物焊橋狀態仍未確認，本案不依賴焊橋而直接將 VIN、VIO 同接3V3，也不將 VIO 缺接認定為全部歷史異常的唯一根因。

使用者已斷電補接 VIO 到3V3，後續量得 ESP32 3V3＝3.29V、HX711 VIN–GND＝3.20V、VIO–GND＝3.20V。以下保留較早的四項電壓；使用者已釐清是在**補接 VIO 之前**量測，不能當作補接後或新荷重元接法的狀態：

| 量測點 | 使用者回報 |
|---|---:|
| VIN–GND | 4.3 V |
| E+–E− | 4.1 V |
| A+–E− | 1.85 V |
| A−–E− | 3.05 V |

依這組歷史值相減，`A+ − A− = −1.20 V`。以 `E+ ≈ AVDD ≈ 4.1 V` 估算，A128差動滿量程約±16.0mV，已足以造成負SAT；不能靠只交換 A+／A−、去皮或濾波修復。後續重新量測及橋路配對試接的結果見第四版「2026-10-20 實測荷重元接線與 SAT 解除」小節，不再以這組舊值描述現況。

RATE 回報為 L（10 SPS），正常轉換週期約100ms；它本身不能解釋持續超過一秒無新資料的 STALE。開機／重置穩定時間可達400ms，須與持續失去資料分開判斷。本輪第四版只作接線與原始讀值排查，未完成受控荷重或長期穩定性驗證；OLED傳輸會影響實際接收間隔，不以 DT 當作晶片精準取樣率。

舊 VIN＝4.3V、E+＝4.1V 與規劃3V3不符，當時先停止帶 HX711 通電，改在完全隔離下燒錄第四版。後續三條供電連接路徑均重測為0Ω，VIN／VIO均量得3.20V，才接回訊號線繼續測試；舊電壓矛盾的具體量測／接觸原因未定位，不能直接判定穩壓器損壞。任何再次換線仍須拔除USB與外接電源，通電時不可用電阻檔。

HX711 的類比 VIN 與數位 VIO 在 SJ1 開路時可以分開供電，但本案維持兩者同接3V3；若 SJ1 閉合，兩者不得接不同電壓，更不能將5V與ESP32 3V3硬接。下方第四版結果區分補接 VIO 後、荷重元改接前後的電壓與 UART，不將不同階段量測混為一組。

若重新接線，先拔除USB與外接電源，VIN、VIO同接ESP32 3V3，不必焊接SJ1；第四版 DATA＝GPIO33、SCK＝GPIO32，`01`～`03`則相反。**不可在焊橋閉合時讓 VIN 接5V、VIO接3V3**，也不可把 VIO 接5V後直連ESP32訊號。

依據：[HX711 原廠資料表](https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf)、[Adafruit 官方原理圖（固定版本）](https://github.com/adafruit/Adafruit-HX711-24-bit-ADC-PCB/blob/4444b42fc47d665fcd6694097690cfbf6c609b43/Adafruit%20HX711%20ADC%20Breakout.sch)、[VIO 焊橋說明](https://learn.adafruit.com/adafruit-hx711-24-bit-adc/pinouts#vio-jumper)。Pinouts 頁的 Power Pins 段落與其 VIO Jumper 段落、正式原理圖存在不一致，接線以晶片資料表及 PCB 線網為準。

### 供電與訊號路徑電阻：2026-10-18 歷史值與 2026-10-20 重測

以下先保留較早的原接線與獨立跳線讀值；之後使用者補回完整路徑結果，見下表。均為現場使用者回報，非遠端直接電表量測：

| 量測方式／路徑 | 使用者回報 |
|---|---:|
| 相同電阻檔，兩支表筆金屬尖端相碰 | 0 Ω |
| HX711 VIN ↔ ESP32 3V3，原接線狀態 | 6 Ω |
| HX711 GND ↔ ESP32 GND，原接線狀態 | 7 Ω |
| VIN 跳線，兩端拔下後單獨量測 | 0 Ω |
| GND 跳線，兩端拔下後單獨量測 | 0 Ω |

相較於表筆短接 0 Ω，兩條預期直連路徑的 6 Ω／7 Ω 值得優先複查，不能直接歸因於表筆阻值。可能涉及表筆與測點接觸、跳線、麵包板或排針／焊點；在電路中的讀值也可能受其他路徑影響，尚不能指認哪一個零件故障，更不能證明它是 SAT／STALE 的唯一根因。單純被動供電線上的壓降，也不足以解釋先前由 3V3 腳取得 VIN＝4.3 V 的矛盾，仍需後續重新確認量測點及電壓。

兩條線單獨均為0Ω，未見線材本身阻值偏高的證據；這不等於間歇接觸或帶載供電長期穩定已通過。後續復接流程採先斷電量完整供電路徑、再量直流供電、最後接訊號線，沒有改程式掩蓋異常。

復接時採用的安全檢查流程如下；目前已取得下表結果，不再標為待回報：

1. USB 與所有外接電源保持拔除；將已量 0 Ω 的兩條跳線插牢，核對 VIN → ESP32 3V3、GND → ESP32 GND，VIO 亦接同一個 3V3。荷重元四條線先不動。
2. 以電阻檔量下表三條完整路徑。表筆輕觸兩塊板上對應端子的金屬焊盤／排針，不以麵包板電源軌代替模組端測點，避免漏掉板子與麵包板的接觸問題；不是量 VIN 與 GND 之間的電阻。測點被遮住時先告知，不勉強拆板。
3. 核對目前接點及完整走線；舊近照不足以確認最新配線，不能將照片疑點當成已確認故障。
4. 供電路徑與電壓確認後，先斷電接回第四版 SCK32／DATA33，再確認完整訊號路徑。不能只憑導通蜂鳴、獨立跳線0Ω或一次供電讀值就宣稱 SAT／STALE 已修復。

| 復接後的板端量測路徑 | 狀態 |
|---|---|
| HX711 VIN ↔ ESP32 3V3 | 後續重測0Ω |
| HX711 GND ↔ ESP32 GND | 後續重測0Ω |
| HX711 VIO ↔ ESP32 3V3 | 後續重測0Ω |
| HX711 SCK ↔ ESP32 GPIO32 | 最終完整路徑重測0Ω；不是僅量拆下的線材 |
| HX711 DATA ↔ ESP32 GPIO33 | 最終完整路徑重測0Ω；不是僅量拆下的線材 |

訊號路徑曾回報18.2MΩ／16.3MΩ，後續重測0Ω／0Ω並明確確認是接線插好、板端到板端的完整路徑；先前MΩ讀值的具體測點／接觸原因未定位，不據此宣告線材或模組損壞。完整路徑確認後，舊荷重元接法仍持續負SAT；改為黑E+／白E−／綠A+／紅A−後才取得下方15筆OK快照。

量測方法依據：[Fluke 電阻量測指南](https://www.fluke.com/en-us/learn/blog/digital-multimeters/how-to-measure-resistance)；電阻檔須在斷電狀態使用，並確認金屬接觸良好、顯示單位確為 Ω，隔離待測線材以避免其他電路路徑干擾。

### Load Cell 與 HX711 端子

| Load Cell 本案實測線色 | HX711 端子 | 備註 |
|---|---|---|
| **黑線** | **E+** | 激勵正端；此線不是直接接 ESP32 GND |
| **白線** | **E−** | 激勵負端 |
| **綠線** | **A+** | A通道正輸入 |
| **紅線** | **A−** | A通道負輸入 |
| B 通道 | B+、B- | 單一荷重元先不接 |

此表取代舊版暫定線色，是2026-10-20在第四版下解除SAT的實測接法，不是其他荷重元的通用線色。**尚未校正**，受力正負方向、額定容量、重複性與長期穩定性仍待確認。改線前先拔除USB及所有外接電源，不可依「黑色一定是地線」接線。

照片銘牌可讀到 `CAP: 180 kg`，但在完成規格與線色確認前，只能視為待確認資料。Load Cell 機械上應一端固定、一端受力，上方配置剛性壓板，不可將整支樑完全夾死。

### HX711 固定負滿量程的排查結果（2026-10-16）

本小節保留當時排查與舊照片，**不是最新接線指引**；2026-10-20已改用上方實測接法，最新SAT解除結果見第四版測試紀錄。

當時近照：[HX711-接線近照-20261016.png](圖片/HX711-接線近照-20261016.png)。使用者當時已確認荷重元四條線接入HX711，因此不能沿用更早照片認定端子未接。2026-10-16回報VIN→ESP32 3V3、GND共地、E−→黑線、A−→白線，僅符合當時暫定規劃；該線色配置已由2026-10-20實測表取代。

`-8388608` 是 24-bit 補數 `0x800000`。依 [HX711 原廠資料表](https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf)，這是最小飽和值；本程式直接讀取原始值，未進行去皮或重量係數換算。

已暫時燒錄獨立診斷程式，以明確的 2 µs 高／低脈衝與另一個補數轉換算式核對，結果如下：

| 讀取方法 | A 通道增益 | 原始位元／數值 | 讀完後 DATA |
|---|---:|---|---|
| Adafruit HX711 1.0.2 | 128 | `0x800000`／`-8388608` | HIGH |
| 獨立逐位元讀取 | 128 | `0x800000`／`-8388608` | HIGH |
| 獨立逐位元讀取 | 64 | `0x800000`／`-8388608` | HIGH |

後續資料約每 11–12 ms 就緒，沒有觀察到 DATA 逾時。原讀取方式與獨立實作結果一致，降低增益也未解除飽和；目前證據將疑點集中在 HX711 類比輸入、橋路接法、激勵供電或模組本體，尚不能僅憑數位握手判定類比電源正常。未發現需要修改原始值解碼的程式錯誤，已重新編譯並恢復原有 `02-irt_oled` 完整程式，保留真實原始值與 `SAT` 提示。

接線核對以板上絲印為準；舊照片六位端子由上至下是 `E−、A−、A+、B+、B−、E+`。當時黑線接E−、白線接A−、綠線接A+、紅線接E+，已由2026-10-20實測接法取代，**不要照舊照片恢復線色位置**。

以下是當時規劃的類比端電壓檢查項目；後續已分階段完成供電連通性、供電與橋路電壓量測，結果見第四版最新小節。再次量測仍須先確認接線安全，以直流電壓檔操作，避免表筆短接相鄰端子；改線先斷電，通電時不得使用電阻／導通檔。

| 紅表筆／黑表筆 | 要記錄的資料 |
|---|---|
| VIN／GND | HX711 模組實際輸入電源；目前規劃為約 3.3 V |
| E+／E− | 實際橋路激勵電壓，不能只以 VIN 值代替 |
| A+／E− | 正輸入對類比地電壓 |
| A−／E− | 負輸入對類比地電壓 |

原廠規範的共模範圍為 `AGND + 1.2 V` 至 `AVDD − 1.3 V`，差動滿量程為 `±0.5 × AVDD / gain`；Adafruit板的E+來自類比電源輸出。需同時核對激勵、兩輸入對地與差動，不只看差動是否為零。最新實測接線下15秒未見SAT，不代表原廠配線、精準差動電壓或校正已完成。供電與端子定義見 [Adafruit HX711 Pinouts](https://learn.adafruit.com/adafruit-hx711-24-bit-adc/pinouts)。

## Tracker Sensor 雙偵測點

2026-10-17，使用者決定以兩顆相同的 Tracker Sensor 取代 OS25B10，兩顆均已購入。照片為 [接腳標示面](圖片/Snipaste_2026-10-17_22-18-33.png)及[元件面](圖片/Snipaste_2026-10-17_22-18-47.png)，可辨識 `GND`、`VCC`、`OUT`。依發射與接收頭並列的外觀，初步判斷為單路紅外反射式循跡模組，不是槽型對射光閘；尚未確認廠牌、料號、內部比較器或輸出型式。

同類原理可參考 [Joy-IT SEN-KY033LT](https://joy-it.net/en/products/SEN-KY033LT)：利用表面對紅外光的反射差異輸出狀態。但這只是原理參考，不能據此認定本案模組就是 KY-033，亦不能直接套用其供電、偵測距離或極性。

### 接線與待補充的電氣確認

| 模組位置／端子 | NodeMCU-32S 對應 | 接線條件 |
|---|---|---|
| 上、下兩顆 GND | GND | 與 ESP32 共地 |
| 上、下兩顆 VCC | 3V3（使用者回報已接） | 慢速功能已確認，額定範圍待補查；不經舊版 180 Ω LED 電阻 |
| 上方 OUT | GPIO34 | 先確認數位輸出及 HIGH／LOW 電壓符合 ESP32 規格 |
| 下方 OUT | GPIO35 | 同上；兩路 OUT 不可併接 |

- 換裝先斷開 USB 與其他電源，將舊 OS25B10 及其專用電阻網路與 GPIO34／35 斷開，不必刪除舊照片或歷史程式。
- 先查實物規格；OUT 尚未接 ESP32 前，以模組額定電源供電並量測無目標／有反射目標時的 OUT–GND 電壓。3.3 V 系統的設計目標為 LOW 接近 0 V、HIGH 接近 3.3 V，不是只確認「不超過 5 V」。
- ESP32 的輸入高電位上限為所屬電源域的 `VDD + 0.3 V`，不可將 5 V OUT 直接接入。若實物必須用 5 V，應先確認輸出電路，再設計電平轉換；僅增加到 3V3 的上拉並不能消除模組內部既有的 5 V 上拉。依據：[Espressif ESP32 資料表](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)。
- GPIO34／35 使用 `INPUT`；這兩腳沒有內建上拉／下拉，不能靠 `INPUT_PULLUP` 解決懸空。推挽輸出或已有合適板載上拉時，不任意追加電阻；確認開集極且無上拉時，才依電路加到 3V3 的外接上拉。
- 不預設「反射即 LOW」或「遮擋即 HIGH」。先分別記錄兩顆的空閒／觸發電位、反應延遲及是否抖動，再決定正式計時邊緣。

### 安裝與測試限制

兩顆感測頭應朝向同一掉落物上的固定反射標記，兩處採用一致的側向間距與角度，並量測實際等效觸發位置的間距。不能把兩顆互相對射，或沿用「物體遮斷光束」的槽型光閘安裝方式。先慢速移動反射標記驗證穩定切換，再評估正式落下速度下的脈衝寬度、漏偵測與環境光影響；未完成重複性測試前，不宣稱能等效取代原光閘的計時精度。

新 `03-irt_tracker` 由 `02-irt_oled` 另存，已將 GPIO34／35 的命名、接線註解及序列輸出改為 Tracker Sensor，仍只回報實際 HIGH／LOW；原 `01-irt`／`02-irt_oled` 保留不動。第三版已完成燒錄與畫面確認，使用者也確認改接 3V3 後兩路各自可慢速反射切換；實際電壓、反射極性及高速性能仍待量測。`02-irt_oled` 與 `03-irt_tracker` 都約每 250 ms 更新一次偵測點快照，可能漏掉快速通過；正式計時需另行實作事件擷取，不可用 OLED／序列埠更新時間作為通過時間。

### 舊版 OS25B10 接線（歷史參考）

OS25B10 是四腳槽型光電開關／光遮斷器，不是具有 `VCC/GND/OUT` 標示的數位模組。實物參考圖：[OS25B10-紅外線對射光電開關.png](圖片/OS25B10-紅外線對射光電開關.png)。

換裝決定前，整組雙光閘已拉出 4 條線，均接於麵包板；下列不是 Tracker Sensor 的接法：

| 線色 | 目前記錄的接線／用途 |
|---|---|
| 紅 | 接 180 Ω 電阻，色環為棕灰棕 |
| 黑 | GND |
| 紫 | 接 6.8 kΩ 電阻，色環為藍灰紅；對應上方 OS25B10 |
| 橘 | 接 6.8 kΩ 電阻，色環為藍灰紅；對應下方 OS25B10 |

### 掉落測試計時概念

1. 同一掉落物標記觸發上方偵測點時記錄 `t_upper`，開始計時。
2. 該標記觸發下方偵測點時記錄 `t_lower`，停止計時。
3. 兩點間時間為 `Δt = t_lower - t_upper`。
4. 若上下等效觸發位置距離為 `d`，兩點間平均速度可估算為 `v = d / Δt`。

前四版只檢查個別感測器或兩路電位；第五版才加入上述中斷計時，仍須確認距離、反射進入極性及高速性能。舊 OS25B10 的電阻另一端接法與遮光極性未完成確認，不應視為 Tracker Sensor 的驗證依據。

## Relay、電磁鐵與釋放按鈕

這一部分目前是預留設計，尚未納入 `01-irt` 通訊測試程式。

### Relay 與 12 V 電磁鐵

| Relay 端子 | NodeMCU-32S／電源 | 說明 |
|---|---|---|
| IN | GPIO26 | 釋放控制；HIGH／LOW 觸發需依實物確認 |
| GND | GND | 非隔離模組需依電路確認共地 |
| VCC | 模組額定電源 | 依 Relay 實物標示，不由 GPIO 供電 |

```text
獨立 12 V+ ── Relay COM
Relay NO ── 電磁鐵正極
電磁鐵負極 ── 獨立 12 V−
```

電磁鐵兩端應並聯飛輪二極體：陰極（有色環端）接正極，陽極接負極。電磁鐵應固定於上方支架，不裝在移動掉落平台上。

### 釋放按鈕

```text
GPIO27 ── 釋放按鈕 ── GND
```

按鈕規劃使用 `INPUT_PULLUP`，按下時讀值為 LOW，並應加入去彈跳。

## 量測設計與限制

- 雙偵測點得到的是兩點間平均速度，不能直接視為撞擊瞬間速度。
- 雙偵測點應盡量靠近撞擊面；第二偵測點到撞擊面的距離需固定或做修正。反射式觸發位置還須驗證表面、角度與環境光造成的偏移。
- HX711 取樣率有限，主要適合不同材料組別的相對受力比較，不應直接宣稱為毫秒級瞬時峰值力。
- ADXL375 應固定牢靠，避免感測器本體晃動造成假峰值。
- Load Cell 線與 I²C 線應遠離 USB、Relay 及電磁鐵高電流線路。
- ADXL375、HX711 與光閘電源附近可配置 0.1 µF 去耦電容；HX711 電源可再加 10 µF。
- 正式試驗前應先做低高度、低重量、單一變因測試，確認訊號極性、機構安全與資料格式。

## 01-irt 通訊測試程式

程式位置：`01-irt/01-irt.ino`。

### 測試範圍

| 測試對象 | 程式行為 |
|---|---|
| ADXL375 | 以 GPIO21／GPIO22 的 I²C 位址 `0x53` 初始化，週期性輸出 X／Y／Z 加速度 |
| HX711 | 以 GPIO32／GPIO33 讀取 A 通道增益 128 的原始值，不換算重量 |
| 上方 OS25B10 | 讀取 GPIO34 的 HIGH／LOW |
| 下方 OS25B10 | 讀取 GPIO35 的 HIGH／LOW |

程式不包含正式落下計時、速度計算、校正、Relay 控制或資料記錄；所有非空程式碼行均附正體中文註解。

`01-irt` 保留原版感測器通訊測試；舊 OS25B10 搭配 OLED 使用下述 `02-irt_oled`，新版 Tracker Sensor 搭配 OLED 使用已燒錄的 `03-irt_tracker`。新版已通過兩路慢速切換目視測試，電氣規格與高速性能仍待確認。

### Arduino CLI 編譯與燒錄

本專案使用：

```text
C:\Program Files\Arduino CLI\arduino-cli.exe
```

目前使用的 Arduino CLI 設定目錄與 ESP32 核心：

```text
設定目錄：C:\Users\tseng\AppData\Local\Arduino15
ESP32 核心：esp32:esp32 3.1.1
FQBN：esp32:esp32:nodemcu-32s
```

PowerShell 指令如下，`COMx` 請替換成 `board list` 顯示的實際埠號：

```powershell
$cli = 'C:\Program Files\Arduino CLI\arduino-cli.exe'
$cfg = 'C:\Users\tseng\AppData\Local\Arduino15'

& $cli --config-dir $cfg board list
& $cli --config-dir $cfg lib install 'Adafruit ADXL375' 'Adafruit HX711'
& $cli --config-dir $cfg compile --fqbn esp32:esp32:nodemcu-32s .\01-irt
& $cli --config-dir $cfg upload --port COMx --fqbn 'esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40' .\01-irt
& $cli --config-dir $cfg monitor --port COMx --config baudrate=115200
```

### 最近一次編譯與燒錄結果

- 2026-09-18 編譯成功：Flash 使用 324,564 bytes（24%），RAM 使用 20,544 bytes（6%）。
- 使用 COM5、`UploadSpeed=115200`、`FlashFreq=40` 上傳成功。
- 所有寫入區段均回報 `Hash of data verified`。
- 重置後序列輸出確認 `[ADXL375] 通訊成功`。
- HX711 已讀到例如 `A128_RAW=3048`、`3775`、`3791`、`4051` 的原始值。
- 當次測試中，上方與下方 OS25B10 均讀到 `LOW`。

這些結果證明程式可編譯、韌體已寫入且基本通訊路徑可執行；不代表 OS25B10 的遮光極性、正式計時精度或落體試驗結果已完成驗證。

## 02-irt_oled OLED 簡易測試程式

程式位置：`02-irt_oled/02-irt_oled.ino`。原有 `01-irt/01-irt.ino` 保留不動。新檔與資料夾同名，以符合 Arduino sketch 規則；每個非空程式碼行均有正體中文註解。

畫面每 250 毫秒更新一次，序列埠以 115200 baud 每秒回報一次。畫面使用英數字型，顯示內容如下（僅為格式示意）：

```text
02-irt OLED TEST
ADXL375 m/s^2
X:     0.00
Y:     0.00
Z:     9.81
HX RAW:3048
UP:LOW DN:LOW
HX:OK OLED:3C
```

| 欄位／狀態 | 意義 |
|---|---|
| X／Y／Z | ADXL375 三軸加速度（m/s²）；讀取失敗時顯示 `ERROR` 與 `--` |
| HX RAW | HX711 A 通道增益 128 最近一筆原始值，未去皮或換算 kg／N |
| HX:WAIT | 開機後尚未收到 HX711 資料 |
| HX:STALE | 超過 1 秒沒有新 HX711 資料；畫面上的原始值為最後一筆快取 |
| HX:SAT | HX711 原始值到達 24-bit 正負極限，須檢查輸入或荷重元接線 |
| HX:OK | 最近 1 秒收到未達正負極限的原始值，不代表校正或類比接線已驗證 |
| UP／DN | 上方 GPIO34／下方 GPIO35 的實際 HIGH／LOW；遮光極性仍待量測 |
| OLED | 七位元 I²C 位址；序列埠另輸出畫面傳送次數 `frames` |

以 OLED 專用程式交替比較兩種驅動後，使用者最終更正為「SH1106 才正常顯示，無缺邊」，因此正式設定為 `OLED_USE_SH1106 = 1`，使用 U8g2 的 `SH1106 128×64` 驅動。若日後更換為 SSD1306 模組，再將此選項改為 `0`。初始化時明確開啟面板並設定對比度 255，與成功診斷的設定一致。`0x3C`／`0x3D` 的 ACK 本身不代表畫面正常；本次 SH1106 設定另有使用者目視確認。OLED 未回應時每 3 秒重試，感測器與序列埠仍持續運作。

HX711 在主迴圈中僅於 DATA 就緒時讀取，短暫保護讀取時脈以避免 ESP32 中斷拉長脈衝；ADXL375 以單次交易讀取三軸並檢查交易結果。此程式為接線與顯示測試，畫面更新會占用 I²C 時間，不適用於正式撞擊峰值或高速光閘計時。

### 編譯與自動燒錄指令

使用 `C:\Program Files\Arduino CLI\arduino-cli.exe`。下列指令使用本機目前的預設設定與已安裝函式庫，`COM3` 是本次辨識到的 ESP32；再次執行前可用 `board list` 確認埠號。

```powershell
$cli = 'C:\Program Files\Arduino CLI\arduino-cli.exe'
$board = 'esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40'
& $cli board list
& $cli lib install 'Adafruit ADXL375' 'Adafruit HX711' 'U8g2'
& $cli compile --jobs 2 --fqbn $board .\02-irt_oled
if ($LASTEXITCODE -eq 0) {
    & $cli upload --port COM3 --fqbn $board .\02-irt_oled
}
& $cli monitor --port COM3 --config baudrate=115200
```

### 本次驗證（2026-10-16）

- 已確認 COM3 的 CH340 裝置連接同一塊 ESP32-D0WD-V3（MAC `e0:8c:fe:a8:42:bc`）。
- 本次使用 Arduino CLI 1.5.1、ESP32 core 3.3.11、U8g2 2.36.19、Adafruit ADXL375 1.1.2 與 Adafruit HX711 1.0.2。
- 最終 SH1106 完整版編譯通過：Flash 312,468 bytes（23%），RAM 24,924 bytes（7%）。
- 已以 COM3、`UploadSpeed=115200`、`FlashFreq=40` 燒錄成功，各寫入區段皆回報 `Hash of data verified`。
- 已恢復感測器完整版，重置後讀取約 10 秒序列輸出，確認新版標題、SH1106 驅動、I²C `0x3C`／`0x53` 回應與 `frames=1` 增至 `34`。
- 使用者已確認完整版的 `02-irt OLED TEST`、X／Y／Z、HX RAW、UP／DN 與底部狀態列皆正常顯示，八行資料完整且無缺邊，OLED 顯示驗證通過。
- ADXL375 持續輸出三軸數值，例如 `X=1.44 Y=1.44 Z=-1.44 m/s^2`；上、下 OS25B10 皆為 LOW，尚未測試遮擋變化。
- HX711 每次讀到 `A128_RAW=-8388608 [SAT]`，表示負滿量程；本次尚未確認其原因，不能視為有效荷重讀值。

## 03-irt_tracker Tracker Sensor 簡易測試程式

程式位置：`03-irt_tracker/03-irt_tracker.ino`。此檔由 `02-irt_oled/02-irt_oled.ino` 另存，資料夾與 sketch 同名以符合 Arduino 規則；原版 `01`、`02` 均未修改。本版為**簡易測試程式**，初次依要求只編譯；同日後續使用者已改為要求燒錄，現已完成 COM3 寫入校驗、啟動輸出、OLED 八行及兩路 Tracker 慢速反射目視確認。HX711 正常荷重讀值與 Tracker 高速性能尚未通過驗證。

### 保留功能與 Tracker Sensor 對應

| 項目 | 第三版行為 |
|---|---|
| OLED | 沿用 SH1106 128×64、GPIO21／22、候選位址 `0x3C`／`0x3D`、八行顯示與斷線重試；本版八行畫面已由使用者確認 |
| ADXL375 | 保留 `0x53` 初始化、100 Hz 設定、三軸換算與讀取失敗提示 |
| HX711 | 保留 GPIO32／33、A128 原始值、非忙碌才讀取、時脈保護與 WAIT／STALE／SAT／OK 狀態 |
| 上方 Tracker Sensor | `TRACKER_UPPER_PIN = 34`，以 `INPUT` 讀取 OUT 原始 HIGH／LOW |
| 下方 Tracker Sensor | `TRACKER_LOWER_PIN = 35`，以 `INPUT` 讀取 OUT 原始 HIGH／LOW |
| 更新週期 | 保留 250 ms 快照／OLED 更新、115200 baud 每秒序列輸出；不是高速計時程式 |

Tracker Sensor 不需要另加通訊函式庫；接線電位與外接上拉需求仍依[雙偵測點接線條件](#tracker-sensor-雙偵測點)確認。本版不反轉讀值、不預設 HIGH 或 LOW 等於「偵測到物體」，也不新增自動校正、落下計時或 Relay 控制。每個非空行均有正體中文註解。

### 03-irt_tracker 完整接線對應表

此表依第三版程式常數與本案實物端子標示整理，所有電源地共接 ESP32 GND；**使用者已回報將 Tracker VCC 改接 3V3，但模組額定供電範圍、實際電壓及 OUT 電位仍未量測確認**。條件式接法不是已完成電氣驗證的宣告。換線前先拔除 USB 與其他電源，對照模組絲印接線，不以照片方向或常見線色代替端子標示。

| 元件 | 元件端子 | NodeMCU-32S 對應 | 供電／訊號與注意事項 |
|---|---|---|---|
| 上方 Tracker Sensor | VCC | 3V3（使用者回報已接） | 額定範圍待補查；不經舊 OS25B10 的 LED 電阻 |
| 上方 Tracker Sensor | GND | GND | 共地 |
| 上方 Tracker Sensor | OUT | GPIO34 | 程式為 `INPUT`；只接 3.3 V 相容數位訊號 |
| 下方 Tracker Sensor | VCC | 3V3（使用者回報已接） | 供電條件同上方模組 |
| 下方 Tracker Sensor | GND | GND | 共地 |
| 下方 Tracker Sensor | OUT | GPIO35 | 程式為 `INPUT`；不得與上方 OUT 併接 |
| OLED | VCC | 3V3 | 沿用本案已正常顯示的供電接法，不改接 5 V |
| OLED | GND | GND | 與 ADXL375、HX711、Tracker Sensor 共地 |
| OLED | SDA | GPIO21 | 與 ADXL375 SDA 共用 I²C |
| OLED | SCK（即 SCL） | GPIO22 | 與 ADXL375 SCL 共用 I²C；不是 HX711 SCK |
| Adafruit ADXL375 | VIN | 3V3 | 電源輸入，依板面標示接 VIN，不接 3Vo |
| Adafruit ADXL375 | GND | GND | 共地 |
| Adafruit ADXL375 | SDA | GPIO21 | I²C 資料；既有線色為橘 |
| Adafruit ADXL375 | SCL | GPIO22 | I²C 時脈；既有線色為棕 |
| Adafruit ADXL375 | CS | 3V3 | 維持 HIGH 選 I²C；此板亦有預設上拉 |
| Adafruit ADXL375 | SDO | GND | 配合程式使用 `0x53`；既有線色為綠 |
| Adafruit ADXL375 | 3Vo | 不接 | 板上穩壓器輸出，不當電源輸入 |
| Adafruit ADXL375 | INT／I2 | 不必接 | 第三版採輪詢；既有 GPIO16／17 選配線不被本程式使用 |
| Adafruit HX711 | VIN | 3V3 | 模組供電輸入，使用者已確認此接點 |
| Adafruit HX711 | GND | GND | 使用者已確認與 ESP32 共地 |
| Adafruit HX711 | DATA | GPIO32 | HX711 輸出至 ESP32，既有為紫線 |
| Adafruit HX711 | SCK | GPIO33 | ESP32 輸出至 HX711，既有為另一條紫線 |
| Adafruit HX711 | RATE | 不接 GPIO | 使用板上開關，H＝80 SPS、L＝10 SPS |
| Adafruit HX711 | VIO | 3V3 | 數位電源 DVDD 輸入，與 VIN 同接 3V3；更正舊版不接的錯誤 |

ADXL375 的 VIN、CS 與板載 I²C 上拉依據：[Adafruit ADXL375 Pinouts](https://learn.adafruit.com/adafruit-adxl375/pinouts)。HX711 的 VIO 依[官方原理圖](https://github.com/adafruit/Adafruit-HX711-24-bit-ADC-PCB/blob/4444b42fc47d665fcd6694097690cfbf6c609b43/Adafruit%20HX711%20ADC%20Breakout.sch)及晶片 DVDD 定義更正；其他端子參考 [Adafruit HX711 Pinouts](https://learn.adafruit.com/adafruit-hx711-24-bit-adc/pinouts)。

HX711另一側接荷重元，不接ESP32 GPIO。以下線色採2026-10-20實測更正，取代第三版章節早期暫定表；解除SAT的驗證是在第四版完成，第三版整合回歸尚未進行：

| HX711 端子 | 本案荷重元接線 | 確認程度 |
|---|---|---|
| E+ | **黑線** | 本案第四版實測解除SAT；不是ESP32 GND |
| E− | **白線** | 本案實測激勵負端 |
| A+ | **綠線** | 本案實測A通道正輸入 |
| A− | **紅線** | 本案實測A通道負輸入 |
| B+／B− | 不接 | 程式只讀 A 通道增益 128 |

**尚未校正**，此為本案實測接法而非通用線色；不因更新本表就宣稱第三版已做最新接線的整合測試。

### 供電與上拉電阻注意事項

1. ESP32 由 USB 供電，既有 OLED、ADXL375、HX711 沿用板上 3V3；感測器不由 GPIO 腳供電。Tracker 僅在確認支援 3.3 V 後接 3V3，供電能力也須涵蓋 ESP32 與各模組負載，不以 USB 的 5 V 直接取代感測器邏輯電源。
2. GPIO34／35 沒有內建上拉／下拉，程式的 `INPUT` 不會自行固定未連接腳位。若 Tracker OUT 確認為開集極、且板上沒有合適上拉，可從每路 OUT **各接一顆 10 kΩ 到 3V3**作低速測試起點，再依波形調整；已有板載上拉或推挽輸出時，不預設必須另加。這是條件式建議，不表示已確認本案輸出電路。
3. Tracker 供電若必須為 5 V，須先核對 OUT 電路並使用適當電平轉換，不能把 5 V OUT 接 ESP32，也不能靠加一顆 3V3 上拉來抵消內部 5 V 上拉。接 GPIO 前先量測 OUT–GND，LOW 目標接近 0 V、HIGH 目標接近 3.3 V；GPIO 電氣限制見 [Espressif ESP32 資料表](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)。
4. I²C 的 SDA、SCL 應各有拉至相容電源的上拉，不是在 SDA 與 SCL 之間接電阻。Adafruit ADXL375 板上 SDA／SCL 各有 10 kΩ 上拉，本案 VIN 使用 3V3；OLED 也可能已有上拉，因此先沿用既有正常匯流排，不任意再並聯 4.7 kΩ。核對所有板載上拉供電及空閒電位，避免拉至 5 V；多組並聯會降低等效阻值。
5. HX711 DATA／SCK 不是 I²C，正常接法不需另套用 I²C 上拉；舊 OS25B10 的 180 Ω LED 限流與 6.8 kΩ 訊號網路不直接沿用到新 Tracker 模組。未接 Tracker 時讀到 HIGH／LOW 可能是懸空，不代表有偵測到反射。

OLED 格式如下，僅為示意，**不是本版實機截圖或讀值**：

```text
03-irt TRACK TEST
ADXL375 m/s^2
X:     0.00
Y:     0.00
Z:     9.81
HX RAW:3048
UP:LOW DN:LOW
HX:OK OLED:3C
```

`UP`／`DN` 改代表兩顆 Tracker Sensor 的 OUT；HX 狀態意義與第二版相同。序列輸出識別為 `03-irt_tracker`，資料列使用 `TRACKER UP=... DOWN=...`，開機時另提醒本版待實機驗證及 OUT 電壓條件。

### 僅編譯指令（不燒錄）

```powershell
$cli = 'C:\Program Files\Arduino CLI\arduino-cli.exe'
$board = 'esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40'
& $cli compile --jobs 2 --fqbn $board .\03-irt_tracker
if ($LASTEXITCODE -ne 0) { throw '03-irt_tracker 編譯失敗' }
```

本節保留第三版初次僅編譯階段的紀錄，刻意不包含 upload 或 monitor 指令。當時編譯通過不代表 Tracker Sensor 供電、接線、極性或第三版 OLED 畫面已驗證，HX711 的 `-8388608 [SAT]` 當時也尚未排除；後續驗證以較新的紀錄為準，最新第四版 SAT 解除結果見「2026-10-20 實測荷重元接線與 SAT 解除」小節。

### 首次編譯紀錄（2026-10-17，當時未燒錄）

- 使用 Arduino CLI 1.5.1、ESP32 core 3.3.11，FQBN 為 `esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40`。
- 沿用 Adafruit ADXL375 1.1.2、Adafruit HX711 1.0.2、U8g2 2.36.19 及既有相依函式庫。
- 編譯成功：Flash 312,612／1,310,720 bytes（23%），RAM 24,924／327,680 bytes（7%）。產物保存在系統暫存目錄，未提交二進位檔。
- 232 行中的 216 個非空行均含中文註解；Tracker 腳位、`INPUT` 模式、版本識別及 OLED 八行座標檢查通過。除 Tracker 命名與識別文字外，八個既有輔助函式的程式邏輯與第二版一致。
- 原 `01-irt`、`02-irt_oled` 未修改；本次沒有執行 upload、monitor、序列埠讀取或裝置重置。第三版的供電、接線、讀值及畫面仍為**尚未實際燒錄驗證**。

### 編譯後燒錄指令（同日後續要求）

使用者後續已明確要求自動編譯後燒錄，取代前一輪「先不要燒錄」的限制。下列 COM3 為本案目前連接埠，執行前應先確認板子身分與接線安全；供電及 OUT 電位未知的 Tracker 不應接入 ESP32。

```powershell
$cli = 'C:\Program Files\Arduino CLI\arduino-cli.exe'
$board = 'esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40'
& $cli compile --jobs 2 --fqbn $board .\03-irt_tracker
if ($LASTEXITCODE -ne 0) { throw '編譯失敗，不執行燒錄' }
& $cli upload --port COM3 --fqbn $board .\03-irt_tracker
if ($LASTEXITCODE -ne 0) { throw '燒錄失敗，請保留診斷輸出' }
```

### 後續燒錄與實機確認（2026-10-17）

- 已確認 COM3 為原 ESP32-D0WD-V3，MAC `e0:8c:fe:a8:42:bc`；重新編譯成功，Flash 312,612 bytes、RAM 24,924 bytes。
- 使用同一編譯產物以 115200 baud 燒錄，bootloader、分割表、boot_app0 與程式各區段均通過 `Hash of data verified`，最後重置啟動。
- 以 115200 baud 擷取重置後約 9 秒資料，確認啟動識別為 `03-irt_tracker`、OLED `0x3C`／SH1106、ADXL375 `0x53`，`frames` 由 1 增至 30，三軸數值持續更新。
- 使用者回覆第三版「八行正常且無缺邊」，確認標題、X／Y／Z、HX RAW、UP／DN 與底部狀態列皆正常。
- 當次序列擷取兩路 Tracker 均為 LOW；後續使用者已改接 VCC 至 3V3 並確認兩路可各自慢速反射切換。尚未取得實際供電／OUT 電壓量測，不宣稱電氣規格或高速性能已驗證。
- HX711 開機先為 WAIT，之後多次為 `8388607 [SAT]`，中間一筆 `-2049 [OK]`。雖與較早負滿量程不同，單筆未飽和值不代表接線、橋路或校正已正常，仍待類比端排查。

本次實際序列輸出節錄（不是上述畫面示意值）：

```text
=== 03-irt_tracker Tracker Sensor OLED 簡易測試程式 ===
[OLED] I2C_ACK 0x3C，驅動 SH1106 128x64；畫面待目視確認
[ADXL375] 通訊成功，位址 0x53
[資料] t=3248 ms | ADXL375: X=1.92 Y=1.92 Z=-1.92 m/s^2 | HX711 A128_RAW=8388607 [SAT] | TRACKER UP=LOW DOWN=LOW | OLED=0x3C frames=10
[資料] t=5248 ms | ADXL375: X=0.96 Y=2.40 Z=-1.92 m/s^2 | HX711 A128_RAW=-2049 [OK] | TRACKER UP=LOW DOWN=LOW | OLED=0x3C frames=18
```

## 04-HX711-test 獨立簡易測試程式

程式位置：[04-HX711-test/04-HX711-test.ino](04-HX711-test/04-HX711-test.ino)。依 Arduino 規則放在同名資料夾，不與其他 `.ino` 混編；原 `01`、`02`、`03` 保留不變。本版只測 HX711 Ａ通道增益 128 與 OLED，不啟動 ADXL375、Tracker、繼電器或無線通訊。

### 第四版接線與版本差異

| 元件端子 | 接到 | 注意事項 |
|---|---|---|
| HX711 VIN | ESP32 3V3 | 與VIO同一電源；供電檢查階段量得約3.20V |
| HX711 VIO | 同一個 ESP32 3V3 | 數位供電輸入；供電檢查階段量得約3.20V，不必焊SJ1 |
| HX711 GND | ESP32 GND | 共地，完整連接路徑重測0Ω |
| HX711 SCK | **GPIO32** | 第四版時脈輸出；第三版為 GPIO33 |
| HX711 DATA | **GPIO33** | 第四版資料輸入；第三版為 GPIO32 |
| HX711 RATE | 不接 GPIO | 沿用板上 L＝10 SPS，H＝80 SPS；本版不以程式改 RATE |
| OLED VCC／GND | 3V3／GND | 沿用既有安全接法，不接 5 V |
| OLED SDA | GPIO21 | 沿用 SH1106 128×64、候選位址 `0x3C`／`0x3D` |
| OLED SCK（SCL） | GPIO22 | 沿用 100 kHz I²C |
| HX711 E+ | **荷重元黑線** | 本案實測激勵正端，不是ESP32 GND |
| HX711 E− | **荷重元白線** | 本案實測激勵負端 |
| HX711 A+ | **荷重元綠線** | 本案實測訊號正端 |
| HX711 A− | **荷重元紅線** | 本案實測訊號負端 |
| HX711 B+／B− | 不接 | 單一荷重元只使用A通道 |

換線前拔除USB與所有外接電源；表中的黑／白／綠／紅是荷重元線色，四條只接HX711綠色端子台。**不要將荷重元黑線另接ESP32 GND，也不要照舊照片恢復紅E+／黑E−／白A−。**若切回第三版，須斷電核對不同的 DATA／SCK 設定；本次最初先隔離HX711燒錄第四版，後續才按上述實測線色接回驗證。

### 讀值與畫面

每個非空白程式行均附正體中文註解。DATA 未就緒即返回，不在中斷保護區內等待轉換；讀取固定24個資料脈衝及1個A128選擇脈衝，高低各明確延遲2µs，並檢查讀後DATA回HIGH。開機只執行一次80µs高電位再拉低重置，故障時不反覆重置。最新SAT解除出現在硬體配對調整後，沒有為此修改本程式或重燒，也不以編譯成功替代硬體驗證。

OLED 每 250 ms 更新、Serial 115200 baud 每秒輸出一次。畫面格式如下（缺值與狀態依實際連線改變）：

```text
04 HX711 TEST
A128 SCK32 DATA33
RAW: --
HEX: ------
HX:TIMEOUT D:1
AGE: --
N:0 E:0
DT:-- OLED:3C
```

- `RAW` 是未校正的有正負號原始計數；`HEX` 是原始 24 位元，不是公斤或牛頓。本版不自動去皮、不濾波，保留正負 SAT。
- `WAIT`：開機等待首筆，未超過 1 秒；`TIMEOUT`：超過 1 秒仍無完整資料框。HX711 完全隔離且 DATA 由弱上拉保持 HIGH 時，TIMEOUT 是預期的缺席結果。
- `LINK_ERR`：最近一次讀後 DATA 未回 HIGH，錯框不覆寫原始值或時間；`STALE`：曾有資料，但已逾 1 秒未更新。
- `SAT`：原始值為 `-8388608` 或 `8388607`；`OK` 僅表示近期有未滿量程且框檢查通過的資料，不是供電、橋路或校正合格證明。
- `D` 為讀取當下 DATA 電位；`AGE` 為最近一筆通過框檢查的資料年齡；`N` 為接收框數（包含 SAT），`E` 為讀後 DATA 錯框數，並非全部錯誤的總數。
- `DT` 為最近兩筆接收時間差，不是保證的取樣週期。OLED 傳輸會阻塞輪詢，本版不能作為高速衝擊峰值或完整每筆資料擷取程式。

DATA 的弱上拉只是讓未接時不浮動，不能替代 VIO 供電；SCK 不額外上拉。OLED 沿用既有 I²C 上拉，不任意並聯電阻。時序及數位電源依 [HX711 晶片資料表](https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf)。

### 編譯與燒錄方式

僅在安全接線確認後執行；第四版最初採HX711完全隔離、保留OLED的方式燒錄，再接回檢查。最新SAT排查沒有再次編譯或燒錄。編譯產物放在系統暫存資料夾，不提交Git：

```powershell
$cli = 'C:\Program Files\Arduino CLI\arduino-cli.exe'
$fqbn = 'esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40'
$hx04BuildPath = Join-Path ([System.IO.Path]::GetTempPath()) ('irt-hx711-04-' + [guid]::NewGuid().ToString('N'))
& $cli compile --jobs 2 --fqbn $fqbn --build-path $hx04BuildPath '.\04-HX711-test'
if ($LASTEXITCODE -ne 0) { throw '第四版編譯失敗，不執行燒錄' }
& $cli upload --port COM3 --fqbn $fqbn --input-dir $hx04BuildPath '.\04-HX711-test'
if ($LASTEXITCODE -ne 0) { throw '第四版燒錄失敗，保留診斷輸出' }
```

### 2026-10-20 編譯、燒錄與空接驗證結果

- 使用 Arduino CLI 1.5.1、ESP32 core 3.3.11、U8g2 2.36.19，以上述 FQBN、`--jobs 2` 編譯成功；Flash 303,924 bytes（23%）、RAM 24,868 bytes（7%），未安裝新函式庫。本版不依賴 Adafruit HX711 函式庫。
- 九組從實際來源函式擷取、轉譯到 Node.js 的邏輯／脈衝模擬通過，涵蓋補數邊界、25 個脈衝、未就緒立即返回、WAIT／TIMEOUT、就緒狀態改變、錯框不覆寫與恢復後 STALE。這不是 MCU 實際波形量測，也不是荷重元驗證。
- COM3 為 CH340，確認 ESP32-D0WD-V3、MAC `e0:8c:fe:a8:42:bc`；第四版各段寫入與雜湊校驗成功，上傳結束碼 0。
- 程式 `.bin` SHA256：`98DDAC59872A5569E46FF6789E4D9DEADE6068C54261430264A272E2694E9A4C`；產物在系統暫存目錄，不提交。
- 首次在使用者回報隔離後讀取約 8 秒，卻出現 `RAW=-1 HEX=FFFFFF`、`ERR=6` 及不規則接收間隔，不能當成 HX711 正常；未以程式濾除 `-1` 或改判定門檻。
- 使用者再次斷電，將 GPIO32／33 跳線從 ESP32 端拔下，回報「已空接」，並確認 OLED 第四版八行正常且無缺邊。再次讀取約 8 秒，八筆快照均為下列預期結果，確認空接輸入基準正常：

```text
[HX711] t=50000 SCK=32 DATA=33 A128 RAW=-- HEX=------ AGE=-- [TIMEOUT] DOUT=1 RX=0 ERR=0 DT=0ms OLED=0x3C
[HX711] t=57000 SCK=32 DATA=33 A128 RAW=-- HEX=------ AGE=-- [TIMEOUT] DOUT=1 RX=0 ERR=0 DT=0ms OLED=0x3C
```

前後未修改程式或重新燒錄，只有使用者現場斷電拔線與重新接 USB。原先的誤觸發來自哪個接點或連接仍未定位；空接基準通過只表示此狀態下未誤收資料，不代表 HX711 已修復。

以上是初次燒錄與空接驗證的歷史結果；後續已接回完成下方實測。任何再次換線仍須先拔除USB與其他電源，核對 VIN／VIO→同一個3V3、GND共地、SCK＝32、DATA＝33，以及最新荷重元色序。

### 2026-10-20 實測荷重元接線與 SAT 解除

在供電完整路徑確認0Ω、ESP32 3V3＝3.29V、HX711 VIN／VIO約3.20V後接回訊號線。最初約15秒皆為 `TIMEOUT、DOUT=1、RX=0、ERR=0`；訊號路徑重測並確認 SCK→GPIO32、DATA→GPIO33 各0Ω後，已持續收框，但舊荷重元接法仍為 `RAW=-8388608 HEX=800000 [SAT]`。該次約15秒RX由230增至362、ERR＝0，未見STALE／TIMEOUT，不代表類比輸入正常。

當時量得 E+−E−＝2.77V、A+−E−＝1.20V、A−−E−＝2.00V，直接差動由使用者確認為 **A+−A−＝−0.81V**。以E+近似AVDD估算，A128差動滿量程約±10.8mV，−810mV已遠超量程，足以造成負SAT；不能以去皮或濾波消除。

斷電將荷重元四條線全部與HX711分離後，使用者回報以下六組電阻：

| 荷重元線色配對 | 電阻 |
|---|---:|
| 紅↔黑 | 312.6Ω |
| 綠↔白 | 263.6Ω |
| 紅↔綠 | 351Ω |
| 紅↔白 | 263.3Ω |
| 黑↔綠 | 313Ω |
| 黑↔白 | 400.4Ω |

依全部六組阻值與分壓關係推論黑／白、紅／綠應分成兩對，不是只以最大阻值猜線色；據此進行可逆試接：**黑→E+、白→E−、綠→A+、紅→A−**。使用者確認在斷電下接好，才進行新電壓量測及UART觀察。這是本案實測接法，仍非取得原廠線色資料表。

| 新接法量測項目 | 使用者現場回報 |
|---|---|
| E+−E− | 約3V |
| A+−E− | 約1.36V |
| A−−E− | 約1.35V |
| A+−A− | 使用者後續明確確認電表顯示 **0V**；檔位／解析度仍未確認，不代表精確零差動 |

上述為分時、有限解析度的電表讀值，不能用粗略小數位相減來宣稱精準差動已量得。使用者回覆仍通電後，COM3／115200 baud擷取約15秒，15筆每秒快照均為OK：

| 觀察項目 | 結果 |
|---|---|
| RAW範圍（15筆快照） | `-161472～-161287` |
| RX | `1748→1879`，持續收到資料框 |
| ERR | `0`，本段未觀察到讀後DATA框錯誤 |
| AGE | `9～13ms` |
| SAT／STALE／TIMEOUT | 15筆快照均未出現 |
| OLED | `0x3C`；第四版八行畫面已有使用者目視確認 |

實際輸出節錄：

```text
[HX711] t=186003 SCK=32 DATA=33 A128 RAW=-161287 HEX=FD89F9 AGE=13ms [OK] DOUT=1 RX=1748 ERR=0 DT=194ms OLED=0x3C
[HX711] t=199003 SCK=32 DATA=33 A128 RAW=-161472 HEX=FD8940 AGE=12ms [OK] DOUT=1 RX=1870 ERR=0 DT=134ms OLED=0x3C
[HX711] t=200003 SCK=32 DATA=33 A128 RAW=-161340 HEX=FD89C4 AGE=12ms [OK] DOUT=1 RX=1879 ERR=0 DT=215ms OLED=0x3C
```

**結論：本次約15秒觀察中SAT已解除，15筆快照全OK，資料持續更新且未見STALE／TIMEOUT。**前後維持同一第四版韌體，沒有重燒、去皮、濾波或更換模組；結果強烈支持原荷重元橋路配對錯接是負飽和的重要原因，但不是對全部歷史異常唯一根因的證明，也不能排除未被每秒快照捕捉的瞬態。

**尚未校正。**負RAW不是負飽和，也不是已校正的負重量；不能將約−161000的原始偏移換稱公斤或牛頓。已知小荷重反應、受力正負方向、去皮與比例係數、準確度／重複性、長時間穩定性，以及第三版整合與動態衝擊驗證均尚未完成。本次文件更新僅記錄既有實測，不另行修改程式、編譯、燒錄或重新讀取序列埠。

## 05-test 整合量測程式

程式：[05-test/05-test.ino](05-test/05-test.ino)。使用同名資料夾符合 Arduino 規則；每個非空白程式行都有正體中文註解，原 `01`～`04` 不修改。這是整合量測流程第一版，不是已校正或已驗證的正式衝擊儀器。

### 第五版完整接線

所有換線先拔除USB及其他電源；**NMK99修訂版將HX711 SCK從GPIO32移至GPIO25、DATA保留GPIO33**，讓GPIO32專用WS2812。使用者已明確確認改線完成並接USB；若尚未改線，不得燒錄本修訂版。落體須固定、電磁鐵外部電源隔離；STOP只能停止記錄，不能停止已掉落物體。

![NMK99v1 第五版 layout2 完整接線圖：HX711 SCK25、DATA33](圖片/完整線路圖_NMK99_05-layout2_20261021.png)

[下載原尺寸 PNG 線路圖（5600 × 3960）](圖片/完整線路圖_NMK99_05-layout2_20261021.png)。本圖於2026-10-21依交接第四十二節、下方接線表與`05-test v1-layout2`程式繪製，涵蓋全部現行感測器、共用供電、荷重元及板載WS2812連線。

圖中為**功能端子示意，非實板排針位置或順序**；同名`3V3`／`GND`符號各自相連。Tracker輸出型態與上拉仍須實測，因此不畫成已加外接電阻；GPIO26／27、Relay及電磁鐵標示為本版未使用。此圖不取代下方注意事項或尚未完成的校正與動態驗證。

| 元件／端子 | 連接位置 | 注意事項 |
|---|---|---|
| HX711 VIN、VIO | 同一個 ESP32 3V3 | 兩腳均須供電；VIO是輸入，不接5V，也不必焊SJ1 |
| HX711 GND | ESP32 GND | 與所有低壓模組共地 |
| HX711 SCK | **GPIO25** | NMK99新版05專用；與第四版／初版05的GPIO32不同 |
| HX711 DATA | **GPIO33** | 本版弱上拉只固定空接電位，不能代替VIO |
| HX711 RATE | 板上開關，目前L | L＝10SPS、H＝80SPS；程式不能讀知開關位置，改開關先斷電 |
| 荷重元黑／白／綠／紅 | **E+／E−／A+／A−** | 本案實測接法；黑線不是ESP32 GND，B+／B−不接 |
| OLED VCC、GND | 3V3、GND | SH1106 128×64，實測位址0x3C，亦搜尋0x3D |
| OLED SDA、SCK（SCL） | GPIO21、GPIO22 | 一百千赫，共用ADXL375匯流排 |
| ADXL375 VIN、GND | 3V3、GND | 不將3Vo輸出當成電源輸入 |
| ADXL375 SDA、SCL | GPIO21、GPIO22 | 需確認實際模組仍已接妥 |
| ADXL375 CS、SDO | 3V3、GND | I²C模式、位址0x53；INT／I2不需接，本版讀FIFO |
| 上方 Tracker VCC、GND、OUT | 3V3、GND、GPIO34 | OUT必須3.3V相容；進入反射標記時的極性須實測 |
| 下方 Tracker VCC、GND、OUT | 3V3、GND、GPIO35 | 不與上方OUT併接，兩路極性可分開設定 |
| NMK99板載WS2812 DIN | **GPIO32** | 開機用核心rgbLedWrite送全黑，之後保持LOW；不與HX711共享 |
| GPIO26、GPIO27／Relay／電磁鐵 | **本版不使用** | 不配置輸出、不自動釋放；也不保證未接管腳位可安全控制現有繼電器 |

GPIO34／35無內建上下拉；是否各加10kΩ至3V3，須依Tracker是否開集極、板載上拉及實測波形決定，不能用3V3外加上拉抵消5V輸出。I²C優先沿用Adafruit模組既有3.3V上拉，不隨意再並聯低阻值電阻；本版沒有更動既有電源或硬體上拉。

NMK99板型依使用者提供型號與照片辨識；[原設計者教材說明](https://www.nmking.io/index.php/2024/01/09/1369/)確認NMK99整合ESP32／相機，[教學作者的實作頁](https://chenlung88.github.io/ESP32-NMK99/)列單顆全彩LED為32。直接操作同一GPIO會同時送到WS2812 DIN與原HX711 SCK，故先取得改線同意及完成確認，才將兩者分離；尚未實測舊時脈如何影響燈色，不將其認定為SAT或OLED問題的唯一根因。GPIO25僅在本案未接相機／TFT／SD及現有接線前提下選用，日後加裝板載周邊須重新核對衝突。

WS2812的關閉是送全黑資料，不是切斷板上電源；只在setup執行一次並等300µs鎖存，再將GPIO32保持LOW，量測期間不送RGB資料。**使用者已目視確認WS2812熄滅**。`LED_BUILTIN=2`不是本案WS2812接腳，不能以此替代實板核對。

### 量測流程及預設值

- 開機進入 `IDLE`，持續診斷ADXL375、HX711、兩路Tracker電位及OLED。**尚未校正**，不自動去皮；距離依使用者回報22.5cm預設為 **225mm**，觸發極性仍為−1（未設定）。上下OUT分別接GPIO34／35；間距是使用者回報值，反射觸發位置偏移仍待驗證。
- ADXL375設定100Hz、32層串流FIFO，每輪最多讀32筆完整三軸，另記I²C錯誤及FIFO滿載／可能漏樣警示。每計數使用名義0.049g，不代表已完成加速度個別校正。
- HX711延續A128、25脈衝、讀後DATA回高檢查，保留WAIT／TIMEOUT／LINK_ERR／STALE／SAT／OK。只保護每個SCK高脈衝，低電位期間允許Tracker中斷，仍未實測脈寬或時間誤差。
- `ARM`只開始記錄：等上閘最多10秒、等下閘最多2秒、下閘後再記2秒。下閘先到、間隔小於200µs、逾時、STOP或緩衝滿均保留資料並進入FAULT；成功則DONE。不因觸發抖動覆寫第一個有效時間。
- ARM期間OLED停在提示頁、一般序列快照暫停；完成後恢復顯示。緩衝容量為加速度2048筆、HX711 1536筆，滿載中止、不覆寫。完成後先DUMP，再明確CLEAR才能重做；斷電會遺失RAM資料、使用者設定與校正，重開機距離回225mm、極性回未設定。
- 時間差由GPIO中斷微秒時間戳相減，支援單次32位計時溢位；速度僅在已輸入實測距離且試驗DONE時算 `距離毫米×1000／時間差微秒`，是兩點間平均速度，不是撞擊速度。

### USB序列埠命令（115200 baud、全大寫、以換行結尾）

| 命令 | 用途與限制 |
|---|---|
| `HELP`／`STATUS` | 查命令／目前原始值、電位、異常、設定；活動期間除STOP外不執行其他操作 |
| `DIST 實測毫米數` | 預設225mm，輸入0～10000mm可暫時覆寫；0表示未知、速度保持NA，不要把示意距離當實測值 |
| `POL 上方 下方` | 各填0或1，表示同一標記進入時的實測LOW或HIGH；未設定不允許ARM |
| `TARE` | 先移除外加荷重並保持靜止，再送命令；收集32筆新有效框，成功後舊比例失效、仍為UNCAL |
| `CAL 已知重量克數` | 先完成TARE，再放上真實已知、安全小重量並靜止，才送命令；收集32筆建立有正負號的counts/g |
| `UNCAL` | 明確清除去皮及比例，保留原始資料語意；不改變HX711輸出的計數 |
| `ARM` | 記錄前須極性已設定、兩路未觸發、ADXL近期有資料、HX為OK；**不釋放落體** |
| `STOP` | 取消校正收集或中止記錄，資料保留；不是物理急停 |
| `DUMP` | DONE／FAULT後輸出設定、品質旗標、摘要及原始CSV；電腦仍須另行保存輸出 |
| `CLEAR` | 明確刪除本次RAM試驗資料，回IDLE；距離、極性及校正保留至斷電 |

只接受完整有限數值，不接受NaN、無窮、額外字尾或過長截斷命令。活動期間不更動參數；DONE／FAULT保留時也不能改距離與校正，避免輸出資料套用不同設定。

校正收集最長8秒；SAT、握手錯誤、STALE、跨度大於2000counts或比例訊號差小於100counts會中止，保留先前係數。這些是初版防呆門檻，不是校正品質認證。成功比例標為 `CAL_USER`，只代表使用者靜態校正；仍需多點已知荷重、方向、重複性與準確度驗證，不可宣稱已通過動態力校正。本次尚未執行TARE或CAL。

### CSV、峰值與量測限制

`DUMP`先輸出以`#`開頭的設定與品質資訊，再輸出欄位：

```text
kind,t_read_us,x_raw,y_raw,z_raw,x_g,y_g,z_g,mag_g,hx_raw,hx_status,mass_g_static
```

ACC與HX分組輸出，各自保留讀出時間及原始值，另一種感測器欄位為NA，沒有假同步、補點或將舊荷重快取複製到每筆加速度。`t_read_us`是相對ARM的**主機讀出時間**；FIFO內樣本可能較早形成，不能據此精算撞擊持續時間或感測器之間的微秒相位差。尚未校正或該框SAT時，`mass_g_static`為NA；即使CAL_USER也只是靜態等效克數，不是瞬時衝擊峰值力。

摘要的加速度合成峰值包含重力，取自**整個ARM記錄視窗**而非已辨識撞擊段；軸絕對值接近190g僅作NEAR_RANGE警示，不是硬體飽和精準判據。HX極值排除SAT框但原始CSV仍保留SAT。FIFO_FULL、握手／I²C錯誤、曾STALE及無資料均不得當作完整正常量測；缺加速度時峰值為NA，不是0g。

100Hz加速度與HX711的10／80SPS都可能漏掉快速衝擊峰值；Tracker的表面、光線、安裝與中斷延遲也會影響速度。第五版先提供可追溯的整合流程，未完成示波器時序、已知速度、長時間、高速及動態衝擊驗證，不據此直接宣稱材料的真實峰值力或吸能量。

依據：[ADXL375資料表](https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL375.pdf)（FIFO、完整六位元組讀取、至少5µs讀間隔）、[Analog Devices I²C資料率說明](https://ez.analog.com/mems/f/q-a/91023/adxl375-sampling-frequency-in-i2c-mode)、[HX711資料表](https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf)。

### 初版SCK32編譯及驗證紀錄（2026-10-20，歷史）

**下列初版與後續同檔重燒紀錄均為SCK32時期，不是最新NMK99 layout2／SCK25的產物或驗證。**最新修訂紀錄見後方NMK99小節及交接第四十二節；舊內容保留供追溯。

主機測試：`node tests/05-test.test.cjs`，**15組**來源轉譯邏輯檢查通過，涵蓋22.5cm→225mm與速度單位、補數邊界、上下順序／重複緣、逾時、微秒溢位、資料狀態、32筆校正及異常保護；不模擬實體中斷延遲、I²C交易或感測器電氣。

Arduino CLI使用既有1.5.1、ESP32 core3.3.11、U8g2 2.36.19與Adafruit ADXL375 1.1.2，未安裝或升級套件。使用者回報間距22.5cm及USB已接回後，將距離預設改為225mm；FQBN沿用 `esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40`，`--jobs 2` **重新編譯成功、結束碼0**。Flash＝331616bytes（25%）、RAM＝68644bytes（20%）、剩餘259036bytes；15組主機檢查再次全數通過，前四版程式未變更。

本次225mm版二進位檔SHA256：`FC6B47B7C8E455AD96B6D942FAF6B7894D16FA8BDC795AF86E2A6DE35B4FE5D4`。產物位於`C:\Users\tseng\AppData\Local\Temp\irt-05-ab7ec2dddcc54b0dbab73dd2cdca1fa7`，不納入Git；較早距離0版的編譯紀錄留在交接第三十九節，不是本次燒錄產物。

**第五版已燒錄COM3：**USB接回後辨識CH340 VID1A86／PID7523，esptool 5.3.1確認原ESP32-D0WD-V3、MAC `e0:8c:fe:a8:42:bc`。使用上述225mm編譯產物上傳，bootloader、分割表、boot_app0及程式均通過寫入Hash校驗，結束碼0並重置；未對COM1試寫。

COM3／115200baud約12秒擷取13筆STATUS（含一次明確查詢），全部`IDLE、UNCAL、HX=OK、ERR=0、POL=-1/-1、DIST_mm=225.000`。RAW＝`-161681～-161513`，RX＝`152→258`，AGE＝`1～45ms`；本段快照未見SAT／STALE／TIMEOUT，不代表全部中間框或長時間運轉均已驗證。只送STATUS及HELP，未執行POL、TARE、CAL或ARM。

**前次ADXL375初始化失敗：**每次重試均為`ADXL375=ERROR address=0x53`，STATUS為`ADXL=MISSING`。`AERR=0、FIFO_FULL=0`是未進入正常讀取時的計數，不能當成加速度計正常或無漏樣的證據。使用者後續確認ADXL已接回：VIN3V3、GND、SDA21、SCL22、CS3V3、SDOGND；但尚無新的實際通訊結果，不能據此宣告初始化已恢復。第五版OLED使用者明確回報「沒畫面」，不是已通過八行目視驗證。

兩路Tracker當時均為0，只是目前電位，不是觸發極性驗證。已請使用者固定落體，用反射標記慢速靠近各感測頭，回報U／D各變0或1；未取得資料前保持未設定，不能ARM。**尚未校正，也未完成第五版全感測器整合及落下驗證。**

### 黑畫面後的同版本重新燒錄（2026-10-20，歷史SCK32版）

使用者回報ADXL已接回且接法如上、OLED無畫面，要求重新燒錄。本輪未改程式、接腳、225mm距離或未設定極性；15組檢查及Arduino CLI重新編譯成功，Flash／RAM與上方相同，bin的SHA256也相同（`FC6B47B7C8E455AD96B6D942FAF6B7894D16FA8BDC795AF86E2A6DE35B4FE5D4`）。沒有證據證明重寫相同程式即可解除黑畫面。

初次Arduino CLI、Windows序列埠及PnP列舉只見COM1，暫時未見CH340；稍後COM3與VID1A86／PID7523重新出現。此間未更動驅動或改用COM1寫入，也未定位USB暫時缺席的原因。

**本輪已完成重新上傳及校驗：**再次確認原ESP32-D0WD-V3、MAC `e0:8c:fe:a8:42:bc`，以相同225mm產物執行Arduino CLI upload，結束碼0並重置。esptool對未變更的bootloader、partitions及程式區直接驗證現有Flash相符，不重寫相同區塊；boot_app0實際寫入並通過Hash校驗。不能將此描述為改了OLED驅動或已證實修好畫面。

重啟後COM3約10秒收到11筆STATUS（含一次查詢）：HX全OK、RAW＝`-160523～-160439`、RX＝`241→331`、ERR＝0、AGE＝`1～72ms`，快照未見SAT／STALE／TIMEOUT；ADXL仍ERROR／MISSING。狀態保持IDLE／UNCAL、225mm、POL＝−1／−1，未執行ARM或校正。已請使用者再次確認OLED是否出現「05 TEST UNCAL」八行；**重啟後實際畫面尚未回覆，ADXL初始化問題仍未解決**。

### NMK99 layout2：關燈、OLED與最新實測（2026-10-20）

使用者提供NMK99v1板型及圖片，確認未接相機／TFT／SD，同意改接並回覆「SCK已改GPIO25、DATA33，已接USB」後才燒錄。舊GPIO32與板載RGB共享，不能直接在HX時脈腳送關燈資料；本次先分離實體接線，再改程式，原01～04來源未修改。GPIO25配置限於目前未使用上述板載周邊的接法。

OLED仍使用已確認的SH1106／128×64／0x3C，不任意切回SSD1306。改用4x6英數字型、`drawStr`固定八行、基線`9,16,23,30,37,44,51,58`、安全區x＝4～123／y＝4～59；每行最多30個ASCII字元，再依`getStrWidth`限於120像素，過長只裁本行，不自動換行，ARM提示也使用同一繪字函式。每次清空畫面緩衝，顯示時明確指定橫向、字向、繪色；沒有改變感測器原始資料或以版面修正掩蓋錯誤。API依據：[U8g2官方參考](https://github.com/olikraus/u8g2/wiki/u8g2reference)。

新增啟動OLED ACK／版面提示、ADXL預期0x53及替代0x1D ACK／晶片識別／寄存器讀回診斷；STATUS與CSV從實際常數輸出接腳。增加RGB／HX分離的編譯期檢查，WS2812只在開機要求全黑，未增加量測期間的RGB傳輸，也不開啟相機、TFT、SD、Relay或無線功能。

| 編譯／燒錄項目 | 實際結果 |
|---|---|
| 主機檢查 | 18組全部通過；新增RGB分腳／只開機送全黑、OLED方向／字型及有界單行繪字模擬 |
| 最終編譯 | 結束碼0，Flash349629bytes（26%）、RAM69340bytes（21%）、剩餘258340bytes |
| 編譯設定 | 沿用Arduino CLI1.5.1、core3.3.11、既有函式庫、jobs2、nodemcu-32s／UploadSpeed115200／FlashFreq40；未安裝或升級套件 |
| 目標與校驗 | COM3原ESP32-D0WD-V3、MAC`e0:8c:fe:a8:42:bc`，所有寫入通過Hash校驗，結束碼0並重置 |
| 本次bin SHA256 | `2C3D9AC1A1FD5D64B5D5FD9AC690D439D8F4402112BE9CA8F89DD7A99EFED79C` |
| 暫存產物 | `C:\Users\tseng\AppData\Local\Temp\irt-05-nmk99-layout2`，不提交二進位檔 |

初次建立新編譯目錄後，另完成最後的基線及STATUS修訂，重新編譯並核對生成CPP包含基線9與SCK25／LAYOUT2後才上傳；未上傳中間版本。最新COM3／115200baud約12秒取得13筆STATUS（含一次查詢）：

| 最新觀察 | 結果 |
|---|---|
| 狀態／設定 | 全IDLE、UNCAL、225mm、POL＝−1／−1；SCK25／DATA33、RGB32、LAYOUT2 |
| HX711 | 13筆全OK、RAW＝`-159032～-158862`、RX＝`10→116`、ERR＝0、AGE＝`10～208ms` |
| ADXL375 | 13筆近期狀態全OK，AERR＝0；不以此宣稱已做加速度校正 |
| FIFO_FULL | 首筆即為1、整段維持1；未擷取確切發生時刻，不能當成無漏樣證據 |
| OLED通訊 | `0x3C` |
| 使用者目視 | **WS2812已熄滅；OLED八行正常、無缺邊** |

實際首末節錄：

```text
# STATUS phase=IDLE reason=NONE CAL=UNCAL HX=OK RAW=-159030 RX=10 ERR=0 AGE_ms=10 ADXL=OK AERR=0 FIFO_FULL=1 UP=0 DN=0 POL=-1/-1 DIST_mm=225.000 OLED=0x3C SCK=25 DATA=33 RGB=32:OFF_REQ LAYOUT=2
# STATUS phase=IDLE reason=NONE CAL=UNCAL HX=OK RAW=-158896 RX=116 ERR=0 AGE_ms=199 ADXL=OK AERR=0 FIFO_FULL=1 UP=0 DN=0 POL=-1/-1 DIST_mm=225.000 OLED=0x3C SCK=25 DATA=33 RGB=32:OFF_REQ LAYOUT=2
```

本段快照未見SAT／STALE／TIMEOUT；加速度近期交易已恢復，但前後同時涉及使用者接線／重接USB、重啟與程式修訂，**不能指認其中哪一項是舊ADXL初始化失敗或OLED問題的唯一根因**。只送STATUS，未執行POL、TARE、CAL、ARM或正式落下。FIFO警示、兩路反射進入極性、長時間／高速及動態衝擊仍待驗證；**尚未校正**，不把短時間OK或目視通過等同完整量測合格。

## 後續工作

2026-10-17後續接線回報：兩顆Tracker在3V3下已通過上下獨立慢速反射切換，當時第三版HX711仍有SAT／STALE。後續2026-10-20已在第四版、更正荷重元配對後觀察到下述SAT解除；不以此宣稱Tracker、第三版整合或高速性能已完成新增驗證。

HX711最新優先事項（更新至2026-10-20）：保留 **黑E+／白E−／綠A+／紅A−** 荷重元接法及**NMK99新版05 SCK25／DATA33、RGB32**。新版13筆快照HX與ADXL均OK，WS已熄滅、OLED八行已確認；**尚未校正**。接續固定安裝、查明FIFO滿載警示、確認反射極性，再作已知小荷重反應、方向、去皮／比例校正、重複性與長時間測試。舊01～04及初版05腳位不同，不直接燒回舊版；不能以短時間結果宣稱真實動態峰值或全部歷史根因已定位。

1. 補查兩顆 Tracker Sensor 的型號、額定供電範圍、OUT 電路與反射觸發極性，量測 3V3 供電下的 OUT 高低電壓；現有 GPIO34／35 接線已通過慢速切換目視測試。
2. 固定兩個反射偵測點及掉落物標記，量測輸出波形、延遲與重複性，評估是否需要訊號整形及能否用於預定速度。
3. ADXL375已回報CS接3V3、SDO接GND，新版近期交易已OK；接續核對固定方式、靜態方向、校正、FIFO警示及高速資料完整性。
4. 保留荷重元實測接法，補查原廠型號／額定容量，完成HX711已知小荷重反應及靜態校正；目前尚未校正。
5. 確認 Relay 額定電壓與觸發邏輯，完成電磁鐵空載釋放測試。
6. 第三版燒錄、OLED 畫面與兩路慢速反射測試已通過確認；接著補足電氣檢查，再另行實作事件擷取、觸發時間、時間差與資料記錄，保留舊版驗證紀錄。
7. 先完成低高度、低重量測試，再進入緩衝材料比較實驗。

## 參考資料

- [Adafruit HX711 24-bit ADC](https://learn.adafruit.com/adafruit-hx711-24-bit-adc)
- [Adafruit HX711 Pinouts](https://learn.adafruit.com/adafruit-hx711-24-bit-adc/pinouts)
- [Adafruit ADXL375 Pinouts](https://learn.adafruit.com/adafruit-adxl375/pinouts)
- [Seeed Studio OS25B10 Photo Interrupter](https://wiki.seeedstudio.com/ja/Photo_interrupter_OS25B10/)
- [Joy-IT 紅外反射循跡模組原理參考（非本案型號認定）](https://joy-it.net/en/products/SEN-KY033LT)
- [Espressif ESP32 資料表：GPIO34／35 與電氣特性](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- [Espressif Arduino-ESP32 NodeMCU-32S variant](https://github.com/espressif/arduino-esp32/blob/master/variants/nodemcu-32s/pins_arduino.h)
- [Espressif ESP32-WROOM-32 datasheet](https://documentation.espressif.com/esp32-wroom-32_datasheet_en.html)
- [U8g2 OLED 控制器與建構子設定](https://github.com/olikraus/u8g2/wiki/u8g2setupcpp)
- [U8g2 API（含 I²C 位址與畫面緩衝）](https://github.com/olikraus/u8g2/wiki/u8g2reference)

