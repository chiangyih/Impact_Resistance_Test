# 落體衝擊量測平台

以 **ESP32／NMK99v1** 為核心，比較 STF、三浦摺疊、EVA 及複合緩衝材料在落下衝擊中的反應。ADXL375 記錄加速度，HX711＋荷重元記錄底座受力原始值，上下兩顆 Tracker 記錄通過時間。

**現行版本**：[05-test v1-layout2](05-test/05-test.ino)　｜　**文件更新**：2026-10-21　｜　**最新硬體紀錄**：2026-10-20

> **目前為整合測試階段，尚未校正**。計時、原始資料記錄及 CSV 匯出已實作；正式落下、高速與動態量測仍待驗證。`ARM` 只開始記錄，**不控制電磁鐵釋放**。

## 從這裡開始

| 你想做什麼 | 閱讀入口 |
|---|---|
| 了解完成到哪裡 | [1. 目前進度](#status) |
| 核對電路或重新接線 | [2. 完整接線圖與接線表](#wiring) |
| 第一次接 USB、查看讀值 | [3. 開機檢查](#first-run) |
| 設定距離、Tracker 極性或荷重係數 | [4. 設定與靜態校正](#setup) |
| 記錄一次試驗、保存 CSV | [5. 記錄與匯出](#recording) |
| 處理異常或理解資料 | [6. 狀態判讀](#troubleshooting) · [7. 資料限制](#data-limits) |
| 查看程式、編譯或追查舊紀錄 | [8. 版本與檔案](#files) · [9. 開發環境](#development) · [10. 後續與歷史](#history) |

<a id="status"></a>

## 1. 目前進度

以下彙整**截至 2026-10-20 的已知狀態**，包含第五版 NMK99 layout2 最新觀察與較早的現場回報；並非本次編排文件時重新量測。

| 項目 | 已確認的結果 | 尚待完成 |
|---|---|---|
| 主板與韌體 | NMK99v1；第五版編譯、COM3 燒錄及 Hash 校驗成功 | 未接相機／TFT／SD；加裝前須重查腳位衝突 |
| HX711＋荷重元 | 約 12 秒、13 筆 STATUS 均為 `OK`，未見 SAT／STALE／TIMEOUT | 小荷重反應、方向、靜態校正與長時間測試 |
| ADXL375 | 13 筆近期狀態均 `OK`、`AERR=0` | **`FIFO_FULL=1` 仍待查**；校正與動態資料完整性未驗證 |
| OLED | SH1106 128×64 設定、位址 `0x3C`；使用者確認八行正常、無缺邊 | 此為驅動與畫面確認，未讀取控制器型號 |
| 板載 WS2812 | GPIO32；使用者確認已熄滅 | 關燈是送全黑資料，板上電源仍存在 |
| 雙 Tracker | 2026-10-17 第三版使用者回報：3V3 下兩路可各自慢速反射切換 | 第五版切換尚未重驗；OUT 電壓、輸出型態、進入極性與高速性能 |

目前設定為 `IDLE`、`UNCAL`、`DIST_mm=225`、`POL=-1/-1`。尚未執行 `POL`、`TARE`、`CAL`、`ARM` 或正式釋放；短時間 `OK` 不代表完整量測已通過。

<a id="wiring"></a>

## 2. 完整接線圖與接線表

> **換線前先拔除 USB 及所有外部電源，固定落體並隔離電磁鐵外部電源**。現行 HX711 是 **SCK25／DATA33**；GPIO32 專供板載 WS2812。舊版接腳不同，不可直接燒回舊韌體。

![NMK99v1 第五版 layout2 完整接線圖：HX711 SCK25、DATA33](圖片/完整線路圖_NMK99_05-layout2_20261021.png)

[下載原尺寸 PNG（5600 × 3960）](圖片/完整線路圖_NMK99_05-layout2_20261021.png)

圖中是**功能端子示意，不是實板排針位置或順序**；同名 `3V3`／`GND` 符號各自相連。接線以模組絲印及 GPIO 編號為準。

### 供電與訊號

| 模組 | 模組端子 → NMK99v1 | 說明 |
|---|---|---|
| ADXL375 | `VIN`、`CS` → **3V3**；`GND`、`SDO` → **GND** | I²C 模式，位址 `0x53`；`3Vo` 是輸出，保持不接 |
| ADXL375 | `SDA` → **GPIO21**；`SCL` → **GPIO22** | 本版讀 FIFO，`INT`／`I2` 不需接 |
| OLED | `VCC` → **3V3**；`GND` → **GND** | SH1106 128×64 設定，實測位址 `0x3C` |
| OLED | `SDA` → **GPIO21**；`SCK` → **GPIO22** | 此處 `SCK` 即 I²C SCL，與 ADXL375 共用匯流排 |
| HX711 | **`VIN`、`VIO` 都 → 同一個 3V3**；`GND` → **GND** | `VIO` 是數位供電輸入，不懸空、不接 5V；不必焊 SJ1 |
| HX711 | **`SCK` → GPIO25；`DATA` → GPIO33** | A 通道、增益 128；GPIO32 不再接 HX711 |
| 上方 Tracker | `VCC` → **3V3**；`GND` → **GND**；`OUT` → **GPIO34** | 反射進入電位需實測 |
| 下方 Tracker | `VCC` → **3V3**；`GND` → **GND**；`OUT` → **GPIO35** | 兩路 OUT 不可併接 |
| 板載 WS2812 | DIN 使用 **GPIO32**，板內已接 | 不需外拉線；開機送全黑，之後保持 LOW |
| Relay／電磁鐵／按鈕 | **本版不使用 GPIO26／27** | 不自動釋放，也不接管外部繼電器狀態 |

**HX711 RATE 是板上開關**：目前回報為 `L = 10 SPS`，`H = 80 SPS`；不接 GPIO，程式不能讀知開關位置，改開關先斷電。`B+`／`B-` 保持不接。

### 荷重元四條線

| 本案實測線色 | HX711 端子 | 功能 |
|---|---|---|
| **黑** | **E+** | 激勵正端；**不是 ESP32 GND** |
| **白** | **E-** | 激勵負端 |
| **綠** | **A+** | A 通道正輸入 |
| **紅** | **A-** | A 通道負輸入 |

這是本案經排查後的實測配對，不能套用到其他荷重元。銘牌可讀到 `CAP: 180 kg`，原廠型號、額定規格與受力正負方向仍待確認；RAW 不能直接當作公斤或牛頓。

### 安裝與電氣注意

- 全部現行低壓模組使用 3V3 並共地；5V OUT 不可直入 ESP32。
- GPIO34／35 **沒有內建上拉／下拉**。Tracker 是否另加上拉須依輸出型態與板載電路確認；圖中未假設已加外接電阻，也不沿用舊 OS25B10 的電阻網路。
- I²C 沿用 Adafruit 模組既有上拉，先核對其拉高電源，不任意再並聯電阻。
- 兩顆 Tracker 朝向同一掉落物上的固定反射標記；不是互相對射。現有間距回報為 **22.5 cm＝225 mm**，等效觸發位置、角度與環境光影響仍待驗證。
- ADXL375 固定在掉落平台並靠近重心；荷重元採一端固定、一端受力與剛性上壓板，避免整支樑完全夾死。

<a id="first-run"></a>

## 3. 開機檢查

1. 固定落體、隔離外部電磁鐵電源，確認接線符合上圖；目前不進行落下試驗。
2. 接上 USB，以序列監控工具開啟**實際辨識到的 ESP32 埠**。設定 **115200 baud、全大寫命令、送出換行**；COM3 是歷史記錄，不保證每次相同。
3. 先依序送出下列兩個查詢命令：

   ```text
   HELP
   STATUS
   ```

4. 確認版次、接腳與設定，再看感測器近期狀態：

   | 檢查欄位 | 如何判讀 |
   |---|---|
   | 啟動識別／`LAYOUT` | `05-test NMK99 ... v1-layout2`／`LAYOUT=2` |
   | `SCK`／`DATA`／`RGB` | `25`／`33`／`32:OFF_REQ`；RGB 熄滅另以目視確認 |
   | `phase`／`CAL`／`POL` | 初始為 `IDLE`／`UNCAL`／`-1/-1` |
   | `HX`／`ADXL` | 近期應有有效資料；`OK` 不表示已校正 |
   | `AERR`／`ERR`／`FIFO_FULL` | 保留異常證據；既有 `FIFO_FULL=1` 尚待排查 |

5. 目視確認 OLED 八行資料及 WS2812 熄滅；再固定裝置，緩慢讓反射標記靠近／移開各 Tracker，確認上方只改 UP、下方只改 DN，記下**標記進入時**各自為 0 或 1。

目前只能依既有證據做接線、通訊與靜態檢查。進入正式落下前，先完成[待辦與驗證](#history)。

<a id="setup"></a>

## 4. 設定與靜態校正

以下是操作方式，**不代表本案已經執行或完成校正**。參數只能在 `IDLE` 且未收集校正資料時更改；若保留上一筆 DONE／FAULT 資料，先匯出保存，再 `CLEAR`。

### 距離與 Tracker 極性

| 命令格式 | 要填的內容 |
|---|---|
| `DIST 毫米數` | 0～10000。預設 225；應依上下等效觸發位置的實測距離調整。`DIST 0` 可記時間，但速度輸出 `NA` |
| `POL 上方 下方` | 各填實測的 `0` 或 `1`，表示同一反射標記**進入**該感測頭時的電位；兩路可不同，不能猜測 |

上表的「毫米數／上方／下方」是參數說明，送出時須換成實際數值。`POL=-1/-1` 表示未設定，不能 `ARM`。

### 荷重靜態校正

1. 確認 HX 狀態為 `OK`、安裝固定，移除外加荷重並保持靜止，送出 `TARE`；等待成功回報。
2. 放上**真實已知且安全的小重量**，保持靜止，再送出 `CAL 克數`，把「克數」換成該物體的實際重量；等待成功回報。
3. 成功後標示 `CAL_USER`，僅代表建立使用者靜態係數；還要驗證方向、多點荷重、重複性與準確度。

`TARE` 成功會使舊比例失效；`UNCAL` 清除去皮與比例。未校正仍可記錄 RAW，`mass_g_static` 會保留 `NA`。**`CAL_USER` 不代表動態力校正合格。**

<details>
<summary>校正收集與拒絕條件</summary>

每次收集 32 筆新有效框，最長 8 秒。SAT、握手錯誤、STALE、讀值跨度大於 2000 counts 或校正訊號差小於 100 counts 時會拒絕；失敗不套用新係數。這些是初版防呆門檻，不是校正品質認證。可送 `STOP` 取消收集。

</details>

<a id="recording"></a>

## 5. 單次記錄與匯出

程式已提供下列流程；實體觸發與動態量測仍待驗證。**`ARM` 不釋放落體，`STOP` 也不是物理急停。**

| 步驟 | 操作 | 程式行為 |
|---|---|---|
| 1. 準備 | 確認距離、極性；兩路標記均移開 | `IDLE`，ADXL 近期有資料、HX 為 `OK` 才能開始 |
| 2. 開始 | `ARM` | 開始記錄，等上方觸發最多 10 秒 |
| 3. 通過 | 依序上方 → 下方 | 上方後等下方最多 2 秒；下方後繼續記錄 2 秒 |
| 4. 結束 | 等待 `DONE` 或 `FAULT` | 保留本次資料；故障也不覆寫 |
| 5. 匯出 | `DUMP`，在電腦端保存輸出 | 輸出設定、品質旗標、摘要與原始 CSV；**不自動存成電腦檔案** |
| 6. 下一次 | 確認已保存，再送 `CLEAR` | 清除本次 RAM 記錄，回到 `IDLE` |

記錄期間 OLED 停在提示頁、一般序列快照暫停，只有 `STOP` 可執行，其他命令被忽略。下方先觸發、兩點間隔不足 200 µs、逾時或緩衝滿會進入 `FAULT`。

**斷電／重啟會遺失 RAM 記錄、極性與校正**，距離回到 225 mm。`CLEAR` 只清試驗資料，距離、極性與校正保留至斷電；DONE／FAULT 保留期間不能修改這些設定。

<details>
<summary>全部序列命令速查</summary>

| 命令 | 用途 |
|---|---|
| `HELP`／`STATUS` | 查命令／目前讀值、設定與異常 |
| `DIST 毫米數`／`POL 上方 下方` | 設定距離／兩路觸發進入電位 |
| `TARE`／`CAL 克數`／`UNCAL` | 去皮／使用已知荷重建立靜態係數／清除係數 |
| `ARM`／`STOP` | 開始記錄／取消校正或中止記錄 |
| `DUMP`／`CLEAR` | 匯出保留的試驗／明確清除試驗 RAM |

命令為全大寫、以換行結尾。完整數值才會接受；NaN、無窮、額外字尾及過長命令會被拒絕。

</details>

<a id="troubleshooting"></a>

## 6. 狀態判讀與常見問題

| 看到的狀態 | 意義與下一步 |
|---|---|
| `HX=OK` | 近期收到完整且未滿量程的資料；仍須校正與穩定性驗證。負 RAW 本身不代表負飽和 |
| `WAIT`／`TIMEOUT` | 尚無首筆完整資料／等待超時；先斷電核對 VIN、VIO、共地及 SCK25／DATA33 |
| `STALE` | 曾有資料但超過 1 秒未更新；顯示值可能是舊快取 |
| `SAT` | 到達正負滿量程；保留原始值，檢查橋路、激勵與輸入，不用去皮或濾波掩蓋 |
| `LINK_ERR`／`ERR` 增加 | HX 讀後 DATA 握手異常；核對數位供電、接線與時序 |
| `ADXL=MISSING` | 近期無有效三軸資料；檢查 3V3、SDA21、SCL22、CS 高、SDO 地及初始化診斷 |
| `FIFO_FULL`／`AERR` 增加 | 可能漏樣／I²C 交易錯誤；本案已有 FIFO 警示，不能當作完整正常資料 |
| `POL=-1/-1` 或不能 `ARM` | 先量測進入極性、設定 POL；再確認 IDLE、兩路未觸發、HX OK、ADXL 近期有資料 |
| OLED 有 ACK 但畫面不正常 | ACK 不等於顯示成功；核對 SH1106 設定、3V3 供電及八行畫面 |
| 記錄時 OLED／STATUS 不更新 | 記錄期間刻意暫停；完成後恢復。需要中止時送 `STOP` |

完整排查數據與各階段接法保留在[歷史 README](docs/archive/README-before-redesign-20261021.md)及[交接紀錄](handoff.md)，請用日期區分，不把舊電壓或舊照片當成現況。

<a id="data-limits"></a>

## 7. CSV 與量測限制

`DUMP` 先輸出 `#` 開頭的設定與品質資訊，再輸出以下欄位。分析前保留設定與警示，不要只留下數值列。

```text
kind,t_read_us,x_raw,y_raw,z_raw,x_g,y_g,z_g,mag_g,hx_raw,hx_status,mass_g_static
```

| 欄位／結果 | 正確讀法 |
|---|---|
| `kind` | `ACC` 與 `HX` 分組輸出；不同感測器沒有假同步或補點 |
| `t_read_us` | 相對 ARM 的**主機讀出時間**；FIFO 樣本可能更早形成，不能當作精準採樣時間 |
| `x_raw`～`mag_g` | 加速度原始計數及名義 g 值；合成峰值**包含重力**，視窗為整次 ARM，尚未自動辨識撞擊段 |
| `hx_raw`／`hx_status` | HX 原始計數及狀態；SAT 保留於 CSV，但不當成可信受力極值 |
| `mass_g_static` | 未校正或 SAT 時為 `NA`；即使完成 CAL_USER，仍只是靜態等效克數 |
| `speed_mps` 摘要 | 僅 DONE 且距離已知時輸出兩點間平均速度；不是撞擊瞬間速度 |

**使用資料前，先確認這些限制：**

- ADXL375 目前為 **100 Hz**，HX711 為 **10／80 SPS**，都可能漏掉快速衝擊峰值；不宣稱毫秒級真實瞬時峰值力或精確吸能量。
- 加速度緩衝 2048 筆、HX 緩衝 1536 筆，滿載即中止，不覆寫。FIFO 滿、STALE、握手及 I²C 錯誤都應保留為品質旗標。
- 缺值為 `NA`，不補成 0；加速度近 190 g 的 `NEAR_RANGE` 僅是警示，不是精準硬體飽和判據。
- Tracker 的反射表面、側向距離、角度、環境光及延遲會影響時間；正式比較材料前，先完成低高度、低重量與重複性驗證。

<a id="files"></a>

## 8. 程式版本與檔案

### 哪個版本對應目前接線？

| 版本 | 用途 | HX711 SCK／DATA | 使用狀態 |
|---|---|---|---|
| **[05-test](05-test/05-test.ino) v1-layout2** | **整合感測、Tracker 中斷計時、手動校正與 CSV** | **25／33** | **現行 NMK99v1 版本**，RGB32 已分開 |
| 初版 05（歷史） | 早期整合版本 | 32／33 | 已由同檔新版取代，詳見歷史紀錄 |
| [04-HX711-test](04-HX711-test/04-HX711-test.ino) | HX711＋OLED 獨立排查 | 32／33 | 保留歷史來源 |
| [03-irt_tracker](03-irt_tracker/03-irt_tracker.ino) | 感測器＋OLED＋雙 Tracker 電位 | 33／32 | 保留歷史來源 |
| [02-irt_oled](02-irt_oled/02-irt_oled.ino) | 感測器＋OLED＋OS25B10 | 33／32 | 保留歷史來源 |
| [01-irt](01-irt/01-irt.ino) | 基本通訊＋OS25B10 | 33／32 | 保留歷史來源 |

舊版不是目前配線的替代燒錄選項。尤其舊 SCK33 會接到目前 HX711 DATA33，切換版本前必須先斷電、核對來源常數及實線；NodeMCU-32S 腳位圖也不能當作 NMK99 實板排針圖。

### 文件與資料入口

| 檔案／資料夾 | 內容 |
|---|---|
| [README.md](README.md) | 現行接線與使用指南 |
| [handoff.md](handoff.md) | 按日期追加的完整驗證證據、歷史更正與未完成事項 |
| [完整線路圖 PNG](圖片/完整線路圖_NMK99_05-layout2_20261021.png) | 現行接線，含電源、荷重元與板內 RGB |
| [圖片/](圖片/) | 元件、麵包板與機構照片；舊照片需對照日期 |
| [tests/05-test.test.cjs](tests/05-test.test.cjs) | 從實際來源轉譯的主機邏輯模擬，非硬體時序測試 |
| [docs/archive/](docs/archive/) | 本次改版前 README 的完整歷史快照 |

`auto-git-watch.ps1` 為本機限定同步工具，已被 `.gitignore` 排除，不屬於硬體操作流程。

<a id="development"></a>

## 9. 開發環境與編譯

以下是最近成功編譯的工具組合，來自 **2026-10-20 紀錄**；本次文件改版未安裝套件、重新編譯或操作裝置。

| 項目 | 記錄值 |
|---|---|
| Arduino CLI／ESP32 core | 1.5.1／3.3.11 |
| U8g2／Adafruit ADXL375 | 2.36.19／1.1.2，沿用既有相依函式庫 |
| FQBN | `esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40` |
| 現行第五版結果 | Flash 349629 bytes（26%）、RAM 69340 bytes（21%）；18 組主機檢查通過 |

FQBN 是沿用的 ESP32 編譯設定，**不表示已完整對應 NMK99 所有板載周邊**；現行 GPIO25 配置以未接相機／TFT／SD 為前提。

在專案根目錄執行 PowerShell，將產物留在系統暫存目錄：

```powershell
$irtCli = 'C:\Program Files\Arduino CLI\arduino-cli.exe'
$irtBoard = 'esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40'
$irtBuild = Join-Path $env:TEMP 'irt-05-build'

& $irtCli compile --jobs 2 --fqbn $irtBoard --build-path $irtBuild .\05-test
if ($LASTEXITCODE -ne 0) { throw '編譯失敗，停止後續作業' }
```

主機邏輯檢查使用 `node tests/05-test.test.cjs`。通過只表示所涵蓋的邏輯模擬通過，不等同中斷延遲、波形、感測精度或完整 ESP32 排程驗證。

<details>
<summary>需要重新燒錄時：確認接線與裝置後，再單獨執行</summary>

先固定落體、隔離電磁鐵外部電源，核對 SCK25／DATA33 與實板身分，關閉占用埠的序列監控；使用剛才成功編譯的同一個產物目錄。下方 `COMx` 必須換成已確認的目標埠，不能把未知的 COM1 當作 ESP32。

```powershell
& $irtCli board list
$irtPort = 'COMx' # 換成已確認身分的實際 ESP32 埠
if ($irtPort -eq 'COMx') { throw '尚未指定已確認的目標埠' }

& $irtCli upload --port $irtPort --fqbn $irtBoard --input-dir $irtBuild .\05-test
if ($LASTEXITCODE -ne 0) { throw '燒錄失敗，保留診斷輸出' }
& $irtCli monitor --port $irtPort --config baudrate=115200
```

上傳成功後仍須檢查啟動識別、STATUS、OLED 與燈號；Flash Hash 校驗不能代替感測器與畫面驗證。

</details>

<a id="history"></a>

## 10. 後續工作與歷史紀錄

依現行整合版本，下一階段按以下順序驗證：

1. 排查 `FIFO_FULL`，確認 ADXL375 固定、方向、資料完整性及加速度校正。
2. 補查 Tracker 型號、額定供電、OUT 電壓與輸出電路；確認進入極性、等效間距、延遲及高速重複性。
3. 完成荷重元機械安裝、已知小荷重反應、受力方向、多點靜態校正與長時間穩定性。
4. 驗證第五版既有計時、緩衝與 CSV 流程；先低高度、低重量，再進入材料比較。
5. 若要加入電磁鐵自動釋放，另行確認 Relay 額定電源、觸發邏輯、飛輪二極體與機械安全；此功能尚未啟用。

**追查紀錄：**[完整交接 handoff.md](handoff.md) · [改版前 README 歷史快照](docs/archive/README-before-redesign-20261021.md)

歷史快照保留原有各版接線、排查數據、編譯／燒錄指令及參考資料；其中舊 GPIO、線色、NodeMCU-32S 與 OS25B10 描述須依日期閱讀。**現行操作以本頁接線與第五版來源為準。**

<details>
<summary>主要原廠參考資料</summary>

- [ADXL375 資料表](https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL375.pdf)
- [Adafruit ADXL375 端子](https://learn.adafruit.com/adafruit-adxl375/pinouts)
- [HX711 資料表](https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf)
- [Adafruit HX711 端子與 VIO 焊橋](https://learn.adafruit.com/adafruit-hx711-24-bit-adc/pinouts)
- [Espressif ESP32 資料表](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- [U8g2 API 與 OLED 設定](https://github.com/olikraus/u8g2/wiki/u8g2reference)

</details>
