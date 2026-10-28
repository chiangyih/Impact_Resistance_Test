# 落體衝擊量測平台

以 **ESP32／NMK99v1** 為核心，比較 STF、三浦摺疊、EVA 及複合緩衝材料在落下衝擊中的反應。ADXL375 記錄加速度，HX711＋荷重元保存底座受力相關原始計數，上下兩顆 Tracker 記錄通過時間；力值、動態峰值與材料效果尚未驗收。

**已部署實板版**：[`hxfail3-pcnt1`](docs/calibration/2026-10-28-hxpcnt-diagnostic-session.md)（完整版本見部署紀錄）　｜　**目前校正**：UNCAL，366g校正未完成。

> **資料更新至2026-10-28 15:09（台北）**：已捕獲被接受的近零異常字，以及LINK拒絕後的DATA／PCNT數位觀察；根因仍未明，量測資料品質未通過。最新5分鐘又見ERR3→5，不能以短窗HX OK、CRC或無大跳恢復校正。重點分析與改善步驟見[目前進度](#status)；完整證據見[PCNT session](docs/calibration/2026-10-28-hxpcnt-diagnostic-session.md)、[獨立QC](docs/quality/reports/2026-10-28-pcnt1-continuous02-qc.md)及[最新被動窗](docs/calibration/20261028-hx711-pcnt-frame-design-passive-150900.lines.jsonl)。

> **開發候選與已部署版分開**：每讀框PCNT sidecar已完成主機驗證與紙審，但[正式Arduino連結失敗](docs/calibration/20261028-pcnt2-compile-failed.log)：`.dram0.bss`超出DRAM區40,400 bytes。尚未燒錄、未做新版實板QC。以[公開主機驗證](docs/calibration/20261028-pcnt2-public-host-validation.json)、正式建置與各版QC分別追溯，主機可驗證不代表裝置已部署。

**工作來源已恢復pcnt1**：[主程式](05-test/05-test.ino)及PCNT adapter／diag已依[已部署公開hash](docs/calibration/20261028-pcnt1-public-host-validation.json)恢復。 恢復版已依原設定[重新編譯成功](docs/calibration/20261028-pcnt1-restored-compile.log)，未再次燒錄。未完成pcnt2候選的38個source／tests／tools已另存本機 `C:/Users/tseng/Documents/Codex/pcnt2-wip-20261028`，只供工作接續；此儲存庫外路徑不隨遠端clone提供，也不是遠端部署來源或已部署結果。

**光學功能已保存結果**：2026-10-26獨立SPEED模式在使用者回報220mm間距下，首筆DONE的`dt_us=1153289`換算0.1907587777m/s，OLED0.1908m/s取位一致；使用者確認同一標記先上後下。這是單次計時／顯示功能結果，距離不確定度、準確度與重複性仍待驗。[原始SPEED](docs/experiments/manual-speed/20261026-optical1-001.speed.txt) · [光學紀錄](docs/experiments/manual-speed/2026-10-26-manual-speed-session.md) · [當版QC](docs/quality/reports/2026-10-26-optical1-qc.md)。

<details>
<summary>10/21～10/22校正與串列失聯歷史入口</summary>

當時兩輪輸入`CAL 400`，實物後更正為20g×10＝200g，分別按各自零點離線重算，並未把修正比例寫回裝置。當時的CAL_USER、電壓／電表回報、OLED照片及串列無回應均為歷史條件，不能當作10/28現況或沿用參數。完整數值見[200g更正報告](docs/calibration/2026-10-21-hx711-correction-200g.md)、[歷史封存索引](docs/calibration/README.md)及[交接紀錄](handoff.md)。10/27重接後已恢復自動串列擷取，之後又經多次診斷重啟；各輪RAM校正與計數分開。

</details>

## 從這裡開始

| 你想做什麼 | 閱讀入口 |
|---|---|
| 了解完成到哪裡 | [1. 目前進度](#status) |
| 查閱校正數據與改善順序 | [校正資料分析](#calibration-review) · [後續改善與驗證](#calibration-next) |
| 查閱10/20的電阻、電壓與讀值 | [2026-10-20 排查量測紀錄](#measurements-20261020) |
| 核對電路或重新接線 | [2. 完整接線圖與接線表](#wiring) |
| 第一次接 USB、查看讀值 | [3. 開機檢查](#first-run) |
| 設定距離、Tracker 極性或荷重係數 | [4. 設定與靜態校正](#setup) |
| 記錄一次試驗、保存 CSV | [5. 記錄與匯出](#recording) |
| 處理異常或理解資料 | [6. 狀態判讀](#troubleshooting) · [7. 資料限制](#data-limits) |
| 查看程式、編譯或追查舊紀錄 | [8. 版本與檔案](#files) · [9. 開發環境](#development) · [10. 後續與歷史](#history) |
| 手動通過兩頭測平均速度 | [獨立光學計時指南](docs/experiments/manual-speed/manual-optical-guide.md) · [本輪紀錄](docs/experiments/manual-speed/2026-10-26-manual-speed-session.md) |
| 查閱燒錄後QC流程與結果 | [QC檢核清單](docs/quality/post-flash-qc.md) · [PCNT部署QC](docs/quality/reports/2026-10-28-pcnt1-qc.md) · [PCNT真事件QC](docs/quality/reports/2026-10-28-pcnt1-continuous02-qc.md) · [四位協作規則](AGENTS.md) |

<a id="status"></a>

## 1. 目前進度

以下依2026-10-27～28已保存串列與獨立QC整理，資料截至10/28 15:09。**部署／資料保存通過，與量測資料品質、靜態校正及動態驗收分開判定。** 最新引用窗口已正常關埠；現場操作進度與COM持有者以[handoff](handoff.md)為準。

| 項目 | 已確認 | 未完成／限制 |
|---|---|---|
| 實板診斷 | pcnt1已編譯、正確MAC及四區寫入校驗；DF497E3D自然boot、雙槽與PCNT事件已保存 | 每讀框sidecar尚未部署；主機驗證不能替代實板QC |
| HX711／校正 | 10/27兩組CAL241程序成功；目前診斷boot為UNCAL | 近零／極端字與ERR持續重現；366g新比例、回零及獨立準確度未完成 |
| PCNT／失敗後DATA | 真LINK後的離散LOW及MCU端SCK0／DATA4計數有完整CRC／時窗證據 | 不是HX端波形、脈寬或供電量測；根因未定位 |
| ADXL375 | 最新窗口近期OK／AERR0，FIFO累計3 | 啟動及DUMP警示分列，來源／漏樣量、加速度校正及動態完整性待查 |
| 光學／顯示 | 10/26首筆SPEED與OLED換算相符，舊版OLED／RGB有目視紀錄 | 新診斷版ACK不代替目視；光學精度與動態試驗未驗 |

<a id="calibration-review"></a>

### 校正資料分析：參數按輪次配對

完整計算、資料來源與量測界線見[校正量測分析報告](docs/calibration/2026-10-28-calibration-assessment.md)。

下列offset單位為counts，scale為counts/g。`mass_g=(RAW-offset)/scale`只適用於該輪配對參數及已確認條件；重啟會失去RAM校正，新的TARE也使舊比例失效。早期校正輪沒有完整自然boot識別，不補造boot ID；`CAL_USER`本身不能證明精確係數或供電連續性。

| 校正輪／階段 | 保存參數與結果 | 可以支持的結論 |
|---|---|---|
| [10/27首次CAL241](docs/calibration/20261027-hx711-241g-calibration-summary.json) | offset−157326.312；scale20.413381743；同一砝碼留置11筆均值240.0895g | 程序已接受；卸載時COM失聯，沒有正式串列回零結果；同砝碼留置不是獨立精度 |
| [10/27 USB重接後CAL241](docs/calibration/2026-10-27-hx711-usb-reconnect-serial-session.md) | offset−158294.438；scale20.577282158；首次卸載−0.772g、241g重放240.903g、再卸載−4.151g、續空載−7.026g | 以上僅依本輪列印參數條件換算；短窗零點未保持，不能因重放接近241g判合格 |
| [10/27 23:51新TARE及跨夜前檢](docs/calibration/2026-10-27-hx711-authorized-auto-calibration.md) | offset−160216.281；舊scale失效，UNCAL；CAL366守衛阻止，後續前檢出現−156與ERR增加 | 沒有CAL366成功或有效新比例；此offset不能配前一輪scale使用 |
| [10/28各診斷boot](docs/calibration/2026-10-28-hxpcnt-diagnostic-session.md) | 依各自session保存RAW／ERR／CRC，當前仍UNCAL | RAW不是g或N；不套用上述任何歷史比例 |

同一套241g校正後也做過兩次366g連續加卸載，各取預定前空載B0／載重L／後空載B1三個60秒窗、每窗60筆；它們是額外重量響應觀察，**不是CAL366**。

| 366g觀察 | L−B0（counts） | L−B1（counts） | B1−B0（counts） |
|---|---:|---:|---:|
| [第一輪](docs/calibration/2026-10-27-hx711-366g-continuous-load-unload.md) | 7622.9667 | 7395.3833 | +227.5833 |
| [第二輪](docs/calibration/2026-10-27-hx711-366g-repeat-load-unload.md) | 7616.3000 | 7541.9167 | +74.3833 |

前後基線不同，載重同期真零點未知；兩個增量均保留，不選較接近366g的一側判精度。366g來自使用者磅秤回報，磅秤解析度、誤差與追溯未知；兩輪手動位置／時間也非同步獨立量測。因此目前只能分列「空載零點變化」與「離群讀值／握手錯誤」兩類問題。[空載排查](docs/calibration/2026-10-27-hx711-negative-drift-investigation.md)有部分連續窗呈負向趨勢，但不可跨缺口或不同boot擬合成全程單調漂移。

### 異常與已取得的診斷證據

| 已保存現象 | 意義與界線 |
|---|---|
| [A1C20320：accepted−320，稍後LINK拒絕−160；9有效LOW／3missed](docs/calibration/2026-10-28-hxdual-until-link-session.md) | 失敗後DATA捕獲可信；約171ms未取點區間電位未知，不能稱連續LOW；hold273.810ms已改後續時序 |
| [DF497E3D：accepted−641／ERR0；LINK拒絕−529408，PCNT SCK0／DATA4](docs/quality/reports/2026-10-28-pcnt1-continuous02-qc.md) | PCNT兩窗valid、無overflow且SCK包絡DATA；DATA開始較anchor遲343～346µs。只證MCU端窗口內數位計數，不是異常讀框全程、HX端類比／供電或根因；HXF8 LOW／4missed仍不證連續LOW |
| [14:46:31.285快照RAW−1282／HX OK／ERR3](docs/calibration/20261028-hx711-pcnt-frame-design-passive-145034.lines.jsonl) | 前後−164000／−164028右移7位數值相容，僅候選；ring已凍結，無該次逐框／edge，不能還原RAW或定丟位原因 |
| [15:04～15:09被動窗再見ERR3→5](docs/calibration/20261028-hx711-pcnt-frame-design-passive-150900.lines.jsonl) | 299 STATUS；首次觀察3→4在15:06:02.629～03.635、4→5在15:06:36.764～37.767。FIFO3，無新逐框資料，不能定位錯誤瞬間或原因 |

`HX=OK`、後檢HIGH、ERR暫不增及CRC通過都不能保證RAW正確。CRC驗的是形成後資料的保存／匯出，主機收行UTC不是ADC時刻；約1Hz STATUS可能看不到已接受異常字。相鄰值算術位移相容不授權修值，缺值／missed維持NA。同步DUMP造成的STALE／MISSING與FIFO增加另列，不能用DUMP解釋尚未DUMP的啟動警示或所有HX異常。

<a id="calibration-next"></a>

### 後續改善與驗證順序

1. **先補異常讀框本身的證據。** 每讀框PCNT sidecar雖完成[主機驗證](docs/calibration/20261028-pcnt2-public-host-validation.json)與紙審，正式連結因DRAM超額40,400 bytes失敗，尚未部署。先處理記憶體配置／預算，重新完成完整連結，再做正確目標燒錄與新版QC；實板heap餘量未知，不能將改用動態記憶體當作已解決。目標是把原始bits、接受狀態、同seq的SCK／DATA計數與CPU括弧一起保存；目前pcnt1只量LINK後等待窗。新功能增加讀框前後開銷，須驗資源、counter／overflow／NA、完整CRC與實板回歸；50個數位edge也不證脈寬、順序或HX端實收。
2. **分開定位零點變化與數位異常。** 固定安裝、載入位置、等待時間與環境條件，一次改一個因素，保留RAW、失敗框與累計警示。現有MCU證據不足以分離取位、接觸／類比、供電或機械／溫度因素；如需判別HX端脈衝或供電，仍需對應的獨立量測。不得以平滑、再TARE或反向位移消除異常。
3. **原因及必要修復通過對應QC後，才重建校正。** 先寫明用途允許誤差、砝碼／磅秤解析度及追溯，再在同一boot近時空載→一次TARE→366g CAL→卸載→241g額外點→卸載→366g重放→卸載；固定各窗口與位置，保存係數、全部異常及回零變化。241g／366g自身誤差仍須納入，不臨時湊一個通過門檻。
4. **靜態通過後另驗動態。** 回零／多點／重複性通過只支持靜態質量用途；衝擊峰值還需感測頻寬、機構傳遞、時間同步、飽和與FIFO完整性證據，之後才比較材料效果。

<a id="measurements-20261020"></a>

### 2026-10-20 排查量測值紀錄

以下依 [handoff.md](handoff.md) 第三十八至四十二節整理。**電阻／電壓來自使用者現場回報；序列數值來自當時 COM3 實際擷取**，本次未重新量測。展開各階段可查看數值。

<details>
<summary>① 供電、完整連接路徑與訊號線檢查</summary>

電阻為拔除 USB 與全部外部電源後量得；直流電壓是在確認接線並通電後量得，兩者不是同時量測。

| 檢查階段 | 量測點 | 2026-10-20 回報值 |
|---|---|---|
| 完整供電路徑 | HX711 VIN ↔ ESP32 3V3 | 0 Ω |
| 完整供電路徑 | HX711 VIO ↔ ESP32 3V3 | 0 Ω |
| 完整接地路徑 | HX711 GND ↔ ESP32 GND | 0 Ω |
| SCK／DATA 尚未接時 | ESP32 3V3 對 GND | 3.29 V |
| SCK／DATA 尚未接時 | HX711 VIN 對 GND | 3.20 V |
| SCK／DATA 尚未接時 | HX711 VIO 對 GND | 3.20 V |
| 第四版完整訊號路徑 | HX711 SCK ↔ GPIO32、DATA ↔ GPIO33 | 兩路曾回報 18.2 MΩ／16.3 MΩ；後續重測均為 0 Ω |

訊號線的 0 Ω 已確認是接好後、板端到板端的完整路徑，不只是拆下跳線量本體。先前 MΩ 值的具體接觸／測點原因，以及供電兩端約 0.09 V 差異的原因未定位；不能據此宣告長期接觸與供電瞬態均合格。

**此處 GPIO32／33 是當時第四版的歷史測點**；現行第五版 SCK 已改 GPIO25、DATA 保持 GPIO33。

</details>

<details>
<summary>② 荷重元四線分離後的六組電阻</summary>

量測條件為完全斷電、四條荷重元線全數與 HX711 分離，使用電阻檔；數值依現場回報記錄，未另取得這一輪電表照片。

| 線色配對 | 電阻 |
|---|---:|
| 紅 ↔ 黑 | 312.6 Ω |
| 綠 ↔ 白 | 263.6 Ω |
| 紅 ↔ 綠 | 351 Ω |
| 紅 ↔ 白 | 263.3 Ω |
| 黑 ↔ 綠 | 313 Ω |
| 黑 ↔ 白 | 400.4 Ω |

六組阻值與後續電壓／UART 對照支持以黑／白作激勵對、綠／紅作訊號對的診斷配置；不是只取最大阻值指定線色，也不是已取得原廠通用線色定義。

</details>

<details>
<summary>③ 荷重元改線前後的直流電壓</summary>

**舊配對（歷史）**：紅 E+、黑 E-、綠 A+、白 A-。**更正後配對**：黑 E+、白 E-、綠 A+、紅 A-。兩次分階段量測，改線均先斷電；表中「甲／乙」表示紅表筆接甲、黑表筆接乙。

| 量測點（紅表筆／黑表筆） | 舊配對 | 更正後配對 |
|---|---:|---:|
| E+／E- | 2.77 V | 約 3 V |
| A+／E- | 1.20 V | 約 1.36 V |
| A-／E- | 2.00 V | 約 1.35 V |
| A+／A-，直接差動量測 | **-0.81 V** | **電表顯示 0 V** |

直接差動的單位已由使用者後續確認為 V，但電表檔位與解析度未確認。**0 V 不能寫成精確零差動或 0 mV**；也不能用分時、粗略小數位的 1.36 與 1.35 V 相減，宣稱精準量得 10 mV。

舊配對的 -0.81 V 遠超 A128 的約毫伏級差動量程，支持負 SAT 與橋路配對有關；這不證明 HX711／荷重元損壞，也不代表已完成荷重校正。

</details>

<details>
<summary>④ 各階段序列讀值與第五版 FIFO 警示</summary>

以下均為當時 115200 baud 擷取的快照；RX 是接收框累計，ERR 是程式檢查到的 HX 讀後握手錯誤，AGE 是最近一框年齡。各段起點與韌體不同，不能把 RX 接續合併。

| 階段／當時版本 | 觀察窗與狀態 | HX RAW | RX／ERR／AGE |
|---|---|---|---|
| HX711 完全空接基準／第四版 | 約8秒、8筆均 TIMEOUT；DOUT=1 | 無資料 | RX=0、ERR=0；AGE 無資料 |
| 接回訊號後初次觀察／第四版 | 約15秒仍 TIMEOUT；DOUT=1 | 無資料 | RX=0、ERR=0 |
| 訊號路徑確認、荷重元仍為舊配對／第四版 | 約15秒、15筆均 SAT | **-8388608**，HEX=800000 | 230→362／0／2～17 ms |
| 荷重元更正配對／第四版 | 約15秒、15筆均 OK | **-161472～-161287** | 1748→1879／0／9～13 ms |
| 距離225mm初版／第五版 SCK32 | 約12秒、13筆 HX 均 OK；ADXL MISSING | -161681～-161513 | 152→258／0／1～45 ms |
| 相同程式重新上傳／第五版 SCK32 | 約10秒、11筆 HX 均 OK；ADXL 仍 MISSING | -160523～-160439 | 241→331／0／1～72 ms |
| NMK99 layout2／第五版 SCK25 | 約12秒、13筆 HX 與 ADXL 近期狀態均 OK | **-159032～-158862** | 10→116／0／10～208 ms |

第四版更正配對後的15筆時間戳為 `186003～200003 ms`，未見 SAT／STALE／TIMEOUT。前後使用同一第四版韌體與模組，未因改配對而重燒、去皮或濾波；結果支持原配對是負 SAT 的重要原因，仍不能證明所有歷史異常只有同一原因。

第五版 NMK99 layout2 的補充紀錄：

| 項目 | 2026-10-20 最新一段結果 |
|---|---|
| ADXL 狀態 | 13筆近期狀態均 OK，AERR=0；未獨立記錄本輪三軸數值或完成校正 |
| FIFO_FULL | **首筆即1、整段維持1**；是累計警示觀察，不是目前深度或精確遺失筆數 |
| 設定與電位 | IDLE、UNCAL、225mm、POL=-1/-1、UP=0、DN=0 |
| OLED／WS2812 | 位址0x3C；使用者確認「八行正常，無缺邊」與「WS2812已熄滅」 |

**所有 RAW 均為未校正原始計數，不是公斤或牛頓**。短時間快照不能取代長時間、全部中間資料框、已知荷重及正式動態驗證；Tracker 極性與第五版切換也未在這段確認。

</details>

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

這是本案經排查後的實測配對，不能套用到其他荷重元。銘牌可讀到 `CAP: 180 kg`，原廠型號與完整額定規格仍待確認。本次實際200g靜態加載得到正比例係數（當時錯輸入CAL 400，已離線修正），但尚未確認準確度；RAW不能直接當作公斤或牛頓。

### 安裝與電氣注意

- 全部現行低壓模組使用 3V3 並共地；5V OUT 不可直入 ESP32。
- GPIO34／35 **沒有內建上拉／下拉**。Tracker 是否另加上拉須依輸出型態與板載電路確認；圖中未假設已加外接電阻，也不沿用舊 OS25B10 的電阻網路。
- I²C 沿用 Adafruit 模組既有上拉，先核對其拉高電源，不任意再並聯電阻。
- 兩顆 Tracker 朝向同一掉落物上的固定反射標記；不是互相對射。2026-10-26使用者將新安裝後有效觸發間距更正為 **220 mm**，並確認安裝時斷電、時間差由OLED讀取，回報欄位單位為ms；實際時間差數字尚未提供。量測方法、不確定度、角度與環境光影響仍待驗證，詳見[安裝補充](handoff.md#tracker-spacing-update-20261026)。
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
   | 啟動識別／`LAYOUT` | 與[已部署版紀錄](docs/calibration/2026-10-28-hxpcnt-diagnostic-session.md)的完整version／session核對；`LAYOUT=2`，開發候選不等於已燒錄版 |
   | `SCK`／`DATA`／`RGB` | `25`／`33`／`32:OFF_REQ`；RGB 熄滅另以目視確認 |
   | `phase`／`CAL`／`POL` | 初始為 `IDLE`／`UNCAL`／`-1/-1` |
   | `HX`／`ADXL` | 檢查近期更新；`HX=OK`仍可能出現已接受異常字，不能當資料或校正通過 |
   | `AERR`／`ERR`／`FIFO_FULL` | 按本boot保存首末及增加時間界線，不跨boot相減；既有警示與資料品質問題待解 |

5. 目視確認 OLED 八行資料及 WS2812 熄滅；再固定裝置，緩慢讓反射標記靠近／移開各 Tracker，確認上方只改 UP、下方只改 DN，記下**標記進入時**各自為 0 或 1。

目前只能依既有證據做接線、通訊與靜態檢查。進入正式落下前，先完成[待辦與驗證](#history)。

<a id="setup"></a>

## 4. 設定與靜態校正

本節說明指令能力；目前因HX資料品質問題保持UNCAL，先按[改善與驗證順序](#calibration-next)處理，再建立新校正。使用者已有自動校正授權，不代表前檢失敗時可反覆TARE／CAL追零。參數只能在 `IDLE` 且未收集校正資料時更改；保留的DONE／FAULT或診斷資料先完整匯出保存。

<details>
<summary>10/21～10/22歷史校正來源與200g更正</summary>

當時兩輪各輸入TARE／CAL 400，實際砝碼後更正為200g；新版20.46921875 counts/g與重放203.76g為該歷史輪的離線修正，不能套入10/27～28或現在的RAW。詳見[200g更正紀錄](handoff.md#hx711-mass-correction-200g-20261022)。

本輪歷史輸出已補存為 [200筆 STATUS／RAW CSV](docs/calibration/2026-10-21-hx711-status.csv)，附[原始文字輸出](docs/calibration/2026-10-21-hx711-source.log)及[欄位／缺值／重複回報說明](docs/calibration/README.md)。保留4筆同RX重複快照，未填造逐筆時間或TARE／CAL內部32框；這不是重新讀取裝置或新的校正結果。

新版fifo-diag1的TARE／CAL 400、回零／重放及後續排查另存[新版原始紀錄](docs/calibration/2026-10-21-hx711-fifo-diag1-source.log)：134筆歷史STATUS及7筆獨立標示的OLED照片觀察，並附電壓回報、COM3無回應檢查與暫停註記；沒有把照片或無回應補成序列資料，也未混入舊版CSV。詳見[新版封存索引](docs/calibration/README.md#fifo-diag1-session)。

**更正後分析入口**：[200g重算報告](docs/calibration/2026-10-21-hx711-correction-200g.md) · [舊版200筆修正CSV](docs/calibration/2026-10-21-hx711-status-corrected-200g.csv) · [新版134筆修正CSV](docs/calibration/2026-10-21-hx711-fifo-diag1-status-corrected-200g.csv) · [七張照片修正CSV](docs/calibration/2026-10-21-hx711-oled-corrected-200g.csv)。原始CSV與log保存當時錯誤重量及換算值，後續分析請採修正版。

</details>

### 距離與 Tracker 極性

| 命令格式 | 要填的內容 |
|---|---|
| `DIST 毫米數` | 0～10000。預設 220（2026-10-26使用者更正）；應依上下等效觸發位置的實測距離調整。`DIST 0` 可記時間，但速度輸出 `NA` |
| `POL 上方 下方` | 各填實測的 `0` 或 `1`，表示同一反射標記**進入**該感測頭時的電位；兩路可不同，不能猜測 |

上表的「毫米數／上方／下方」是參數說明，送出時須換成實際數值。`POL=-1/-1` 表示未設定，不能 `ARM`。

### 荷重靜態校正

以下流程適用於異常原因／必要修復經對應QC、參考重量與允許誤差已確認後；目前尚未進入這個新校正階段。`HX=OK`一項不足以滿足前提。

1. 確認 HX 狀態為 `OK`、安裝固定，保留正式使用的托盤／治具，移除外加荷重並保持靜止，送出 `TARE`；等待成功回報。
2. 放上**真實已知且安全的小重量**，保持靜止，再送出 `CAL 克數`，把「克數」換成該物體的實際重量；等待成功回報。
3. 成功後標示 `CAL_USER`，僅代表建立使用者靜態係數；還要驗證方向、多點荷重、重複性與準確度。

`TARE` 成功會使舊比例失效；`UNCAL` 清除去皮與比例。未校正仍可記錄 RAW，`mass_g_static` 會保留 `NA`。**`CAL_USER` 不代表動態力校正合格。**

<details>
<summary>校正收集與拒絕條件</summary>

每次收集 32 筆新有效框，最長 8 秒。SAT、握手錯誤、STALE、讀值跨度大於 2000 counts 或校正訊號差小於 100 counts 時會拒絕；失敗不套用新係數。這些是初版防呆門檻，不是校正品質認證。可送 `STOP` 取消收集。

</details>

<a id="recording"></a>

## 5. 單次記錄與匯出

程式已提供下列流程；目前HX異常排查不進行正式落下，實體觸發與動態量測仍待驗證。獨立手動速度測試使用[SPEED指南](docs/experiments/manual-speed/manual-optical-guide.md)。**`ARM` 不釋放落體，`STOP` 也不是物理急停。**

| 步驟 | 操作 | 程式行為 |
|---|---|---|
| 1. 準備 | 確認距離、極性；兩路標記均移開 | `IDLE`，ADXL 近期有資料、HX 為 `OK` 才能開始 |
| 2. 開始 | `ARM` | 開始記錄，等上方觸發最多 10 秒 |
| 3. 通過 | 依序上方 → 下方 | 上方後等下方最多 2 秒；下方後繼續記錄 2 秒 |
| 4. 結束 | 等待 `DONE` 或 `FAULT` | 保留本次資料；故障也不覆寫 |
| 5. 匯出 | `DUMP`，在電腦端保存輸出 | 輸出設定、品質旗標、摘要與原始 CSV；**不自動存成電腦檔案** |
| 6. 下一次 | 確認已保存，再送 `CLEAR` | 清除本次 RAM 記錄，回到 `IDLE` |

記錄期間 OLED 停在提示頁、一般序列快照暫停，只有 `STOP` 可執行，其他命令被忽略。下方先觸發、兩點間隔不足 200 µs、逾時或緩衝滿會進入 `FAULT`。

**斷電／重啟會遺失 RAM 記錄、極性與校正**，新版距離回到 220 mm。`CLEAR` 只清試驗資料，距離、極性與校正保留至斷電；DONE／FAULT 保留期間不能修改這些設定。

<details>
<summary>全部序列命令速查</summary>

| 命令 | 用途 |
|---|---|
| `HELP`／`STATUS` | 查命令／目前讀值、設定與異常 |
| `ADXL_DIAG` | 已燒錄的fifo-diag1新增：列出已保存的 FIFO 來源、時間與 OLED 耗時；只印快取，不清警示 |
| `HX_DIAG STATUS`／`HX_DIAG DUMP` | 逐讀／雙槽／PCNT診斷；以該版完整外層END與CRC確認匯出，DUMP同步輸出會阻塞輪詢 |
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
| `HX=OK` | 程式近期接受過讀值；本案已出現OK／ERR不增而RAW近零異常，不能當資料正確或量測通過。負RAW本身也不代表負飽和 |
| `WAIT`／`TIMEOUT` | 尚無首筆完整資料／等待超時；先斷電核對 VIN、VIO、共地及 SCK25／DATA33 |
| `STALE` | 曾有資料但超過 1 秒未更新；顯示值可能是舊快取 |
| `SAT` | 到達正負滿量程；保留原始值，檢查橋路、激勵與輸入，不用去皮或濾波掩蓋 |
| `LINK_ERR`／`ERR` 增加 | 原HX讀後DATA檢查未通過；保留原字、計數與時間界線，根因未定，不由累計數推漏樣量 |
| `ADXL=MISSING` | 近期無有效三軸資料；檢查 3V3、SDA21、SCL22、CS 高、SDO 地及初始化診斷 |
| `FIFO_FULL`／`AERR` 增加 | 前者是開機以來警示觀察次數，後者是原加速度讀取／FIFO 檢查錯誤；固定 `1` 不表示目前持續滿載，也不等於只漏一筆 |
| `POL=-1/-1` 或不能 `ARM` | 先量測進入極性、設定 POL；再確認 IDLE、兩路未觸發、HX OK、ADXL 近期有資料 |
| OLED 有 ACK 但畫面不正常 | ACK 不等於顯示成功；核對 SH1106 設定、3V3 供電及八行畫面 |
| COM3可開但無STATUS，OLED的RAW仍更新 | 先保留RAM、不按EN／BOOT或拔USB；核對OLED phase、UART／USB路徑，不能把零接收字元當成0g或ERR=0；見最新排查紀錄 |
| 記錄時 OLED／STATUS 不更新 | 記錄期間刻意暫停；完成後恢復。需要中止時送 `STOP` |

完整排查數據與各階段接法保留在[歷史 README](docs/archive/README-before-redesign-20261021.md)及[交接紀錄](handoff.md)，請用日期區分，不把舊電壓或舊照片當成現況。

<details>
<summary>2026-10-21：FIFO_FULL＝1 的程式排查、診斷版部署與首次待機結果</summary>

原程式遇到 `entries >= 32` **或** `INT_SOURCE` 的 D0 Overrun 位元時，就把同一個 `FIFO_FULL` 加一；兩種情況原本無法區分。歷史 13 筆 STATUS 均為 1，僅表示那段快照未再看到增加，不能反推首次警示原因或精確漏樣量。

已安裝的 ADXL375 驅動會先啟用量測，再由本案設定 ODR 與 FIFO。依[原廠資料表第14、23頁](https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL375.pdf)，Overrun 在 bypass 也可能產生，讀 INT_SOURCE 本身不清除該資料旗標；因此啟動前後帶入舊旗標是**待驗證假說**，尚未證實。100 kHz 傳送一幀 1024 bytes 的 OLED，資料部分約92 ms；100 Hz 下 FIFO 容量約320 ms，單靠一幀傳輸不足以判定必滿，仍須實測整體阻塞時間。

`v1-layout2-fifo-diag1` 保留原警示、100 Hz、100 kHz、既有 FIFO 重設與所有接腳；新增以下證據，不用清零或降低取樣率製造「正常」：

| 診斷輸出 | 如何使用 |
|---|---|
| `DEPTH_OBS`／`OVR_OBS` | 分開保存深度滿載與 D0 的觀察次數；可同時成立，不能相加當作漏樣數 |
| `FIFO_PRE`／`FIFO_POST` | 深度、原始來源位元與各自主機時間；POST 僅在非記錄時更新，並非同時快照 |
| `FIFO_EVENT` | 保留首次與最近警示的主機觀察時間、深度、來源及階段；不是硬體事件精準時刻，無警示時為 NONE |
| `FIFO_DEPTH_MAX`／`FIFO_POLL_GAP_MAX` | 最大已觀察深度／poll 開始到開始的最大間隔；無有效觀察或尚無兩次 poll 時為 NA |
| `ACC_RX`／`DIAG_ERR` | 成功完整三軸交易總數／選用診斷交易錯誤；後者不混入原 AERR |
| `FIFO_INIT`／初始化快照 | 保存驅動與重設前後狀態；有效旗標與 NA 必須連同數值閱讀 |
| `OLED_TIMING` | 最近／最長 `sendBuffer()` 主機呼叫耗時，包含一般畫面與 ARM 前提示；不是示波器脈寬 |

`FIRST_PHASE`／`LAST_PHASE` 的數字對應：0＝IDLE、1＝ARMED、2＝WAIT_DN、3＝CAPTURE、4＝DONE、5＝FAULT。

以下是2026-10-21 fifo-diag1的歷史待機窗：使用者當時確認落體固定、電磁鐵外部電源隔離及USB供電，再要求燒錄，保存啟動及唯讀診斷。15筆STATUS的FIFO_FULL均為1；兩次診斷均為`FULL_OBS=1 DEPTH_OBS=0 OVR_OBS=1 DIAG_ERR=0`，首次／最近事件同為`US=505194 N=3 SRC=0x81 PHASE=0`，最大深度14。初始化前`N=32 SRC=0x83`、重設後`N=0 SRC=0x81`；非同時快照不能定根因／漏樣量。OLED呼叫最大118341µs、poll開始間隔最大140533µs只屬當時短窗；新PCNT啟動及DUMP警示另見[本輪QC](docs/quality/reports/2026-10-28-pcnt1-continuous02-qc.md)。

命令只印快取，不讀I²C、不重設FIFO；記錄期間仍只允許STOP。新增非記錄期快照與文字輸出也會占用時間。先前只編譯未燒錄的歷史另見handoff第四十五、四十六及四十八節；本次部署見[第四十九節](handoff.md#fifo-diag1-deployment-20261021)與[原始序列紀錄](docs/deployment/2026-10-21-fifo-diag1-serial.log)。使用者其後已確認新版OLED八行正常、RGB熄滅。**已完成燒錄及基本待機驗證，但根因、精度及正式落下仍未驗證，沒有清除警示或宣告修復。**

</details>

<a id="data-limits"></a>

## 7. CSV 與量測限制

`DUMP` 先輸出 `#` 開頭的設定與品質資訊，再輸出以下欄位。分析前保留設定與警示，不要只留下數值列。

```text
kind,t_read_us,x_raw,y_raw,z_raw,x_g,y_g,z_g,mag_g,hx_raw,hx_status,mass_g_static
```

| 欄位／結果 | 正確讀法 |
|---|---|
| `kind` | `ACC` 與 `HX` 分組輸出；不同感測器沒有假同步或補點 |
| `t_read_us` | 相對ARM的**ESP32讀出時間**，不是電腦收行UTC；FIFO樣本可能更早形成，不能當精準採樣時間 |
| `x_raw`～`mag_g` | 加速度原始計數及名義 g 值；合成峰值**包含重力**，視窗為整次 ARM，尚未自動辨識撞擊段 |
| `hx_raw`／`hx_status` | HX 原始計數及狀態；SAT 保留於 CSV，但不當成可信受力極值 |
| `mass_g_static` | 未校正或 SAT 時為 `NA`；即使完成 CAL_USER，仍只是靜態等效克數 |
| `speed_mps` 摘要 | 僅 DONE 且距離已知時輸出兩點間平均速度；不是撞擊瞬間速度 |

**使用資料前，先確認這些限制：**

- ADXL375 目前為 **100 Hz**，HX711 為 **10／80 SPS**，都可能漏掉快速衝擊峰值；不宣稱毫秒級真實瞬時峰值力或精確吸能量。
- 加速度緩衝 2048 筆、HX 緩衝 1536 筆，滿載即中止，不覆寫。FIFO 滿、STALE、握手及 I²C 錯誤都應保留為品質旗標。
- 缺值為 `NA`，不補成 0；加速度近 190 g 的 `NEAR_RANGE` 僅是警示，不是精準硬體飽和判據。
- 診斷READ／HXF／PCNT的CRC支持資料形成後保存與匯出完整，不證ADC真值；雙槽重疊按同一session／seq只計一次。STATUS快照、逐讀attempt與全部ADC轉換分開。
- Tracker 的反射表面、側向距離、角度、環境光及延遲會影響時間；正式比較材料前，先完成低高度、低重量與重複性驗證。

<a id="files"></a>

## 8. 程式版本與檔案

### 哪個版本對應目前接線？

| 版本 | 用途 | HX711 SCK／DATA | 使用狀態 |
|---|---|---|---|
| **[已部署pcnt1](docs/calibration/2026-10-28-hxpcnt-diagnostic-session.md)** | **雙槽逐讀、失敗後DATA與MCU PCNT數位計數，沿用整合功能** | **25／33** | 工作樹主程式已按部署hash恢復pcnt1；有限QC通過、資料品質未通過，UNCAL／366g未完成 |
| [每讀框sidecar候選](docs/calibration/20261028-pcnt2-public-host-validation.json) | 原讀框加PCNT附加記錄 | 25／33 | 候選外置備份；主機驗證／紙審通過但正式連結DRAM超額失敗，未部署／新版實板QC未做 |
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
| [tests/05-fifo-diag.test.cjs](tests/05-fifo-diag.test.cjs) | 新增 FIFO 來源、時間及診斷防護的主機檢查 |
| [docs/archive/](docs/archive/) | 本次改版前 README 的完整歷史快照 |
| [docs/calibration/](docs/calibration/) | 各日期／校正輪的RAW、參數、載卸、逐讀／PCNT原文與主機紀錄；最新入口見[校正分析](#calibration-review) |
| [docs/quality/reports/](docs/quality/reports/) | 各部署版及同版新session的獨立QC；未測／未解不由舊版代替 |
| [docs/deployment/](docs/deployment/) | fifo-diag1燒錄、目標身分、啟動與待機診斷原始紀錄 |

`auto-git-watch.ps1` 為本機限定同步工具，已被 `.gitignore` 排除，不屬於硬體操作流程。

<a id="development"></a>

## 9. 開發環境與編譯

最近已部署pcnt1使用Arduino CLI1.5.1、ESP32 core3.3.12／IDF5.5.5及下列既有FQBN。正式編譯程式1013665／1310720 bytes、全域123928／327680 bytes；後者是編譯器摘要，不是runtime heap量測。[公開主機驗證與來源hash](docs/calibration/20261028-pcnt1-public-host-validation.json) · [正式compile](docs/calibration/20261028-pcnt1-compile.log) · [正確目標upload](docs/calibration/20261028-pcnt1-upload.log) · [本版QC](docs/quality/reports/2026-10-28-pcnt1-qc.md)。

每讀框sidecar候選的[主機驗證](docs/calibration/20261028-pcnt2-public-host-validation.json)與[正式連結失敗](docs/calibration/20261028-pcnt2-compile-failed.log)分開：主機邏輯／SDK語法通過未覆蓋實際DRAM配置，尚未部署或取得新版實板QC。下方命令是開發方法，不將主機測試當成實板功能通過。

<details>
<summary>2026-10-21核心更新、舊建置與產物hash（歷史）</summary>

當時先封存HX711歷史快照，再依使用者要求將本機ESP32 core由3.3.11更新到3.3.12，以同一份FIFO診斷來源編譯／部署。當時版本核對來源為[Espressif官方索引](https://espressif.github.io/arduino-esp32/package_esp32_index.json)與[發布頁](https://github.com/espressif/arduino-esp32/releases/tag/3.3.12)；以下數字不當成pcnt1或新候選的建置結果。

| 項目 | 2026-10-20 交接記錄 | 本輪本機2026-10-21更新後實際讀回／結果 |
|---|---|---|
| Arduino CLI／ESP32 core | 1.5.1／3.3.11 | CLI **1.5.1**（01f3d4f2b）；core **3.3.12**，core list及實際目錄均確認 |
| U8g2／Adafruit ADXL375 | 2.36.19／1.1.2 | 2.36.19／1.1.2，應用函式庫未升級；核心所需工具由套件管理器安裝 |
| FQBN | `esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40` | 相同 FQBN；`build.options.json`確認實際核心為3.3.12 |
| 編譯結果 | 舊已燒錄版：Flash 349629 bytes、RAM 69340 bytes | 本次重新編譯／已燒錄版：Flash **351921 bytes（26%）**、RAM **69428 bytes（21%）**，編譯及上傳均結束碼0 |
| 主機檢查 | 當時18組通過 | 原18組＋新增19組＝**37組通過**；非硬體時序驗證 |

同步文件中另有nightly-20260918／189327e、core3.1.1→3.3.12的歷史建置，保留於handoff第四十五、四十六節；不能據此推定目前本機狀態。**本輪**更新前即時讀回為CLI1.5.1／core3.3.11，更新後為1.5.1／3.3.12；新產物與驗證詳見[第四十八節](handoff.md#hx711-archive-local-build-20261021)。各輪環境、產物及雜湊分別記錄，不互相代用。

本次重新編譯並實際燒錄的bin SHA256（先前只編譯的產物另保留於第四十八節）：

```text
B62CDBCB2FF2DB21DB86AA96C1751AC1AEAA5533B40B216CFA2890FE71C750E1
```

建置產物留在系統暫存目錄，未納入Git。本次上傳的bootloader、partitions、boot_app0及主程式均通過Hash校驗，新啟動識別及待機命令已確認；見[第四十九節部署紀錄](handoff.md#fifo-diag1-deployment-20261021)。**基本啟動通過不等於FIFO根因、感測精度或動態驗證通過，重燒後需重新建立並驗證校正。**

</details>

FQBN 是沿用的 ESP32 編譯設定，**不表示已完整對應 NMK99 所有板載周邊**；現行 GPIO25 配置以未接相機／TFT／SD 為前提。

在專案根目錄執行 PowerShell，將產物留在系統暫存目錄：

```powershell
$irtCli = 'C:\Program Files\Arduino CLI\arduino-cli.exe'
$irtBoard = 'esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40'
$irtBuild = Join-Path $env:TEMP 'irt-05-build'

& $irtCli compile --jobs 2 --fqbn $irtBoard --build-path $irtBuild .\05-test
if ($LASTEXITCODE -ne 0) { throw '編譯失敗，停止後續作業' }
```

主機邏輯檢查使用 `node tests/05-test.test.cjs` 及 `node tests/05-fifo-diag.test.cjs`。通過只表示所涵蓋的邏輯模擬通過，不等同中斷延遲、波形、感測精度或完整 ESP32 排程驗證。

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

正確目標完整寫入校驗成功後，立即依[QC清單](docs/quality/post-flash-qc.md)由Ampere做該版獨立檢核並留報告；保留自然啟動、版本／session、STATUS、OLED與燈號證據，未測保留待補。Flash Hash、主機測試與舊版報告不能代替新版實板及量測驗證。

</details>

<a id="history"></a>

## 10. 後續工作與歷史紀錄

校正與資料品質的優先路徑見[後續改善與驗證順序](#calibration-next)。接續還需：

1. 將ADXL啟動／匯出期FIFO警示與HX異常分開驗證，確認固定、方向、加速度校正及動態資料完整性。
2. 補足Tracker的等效觸發距離、進入極性、延遲、光學準確度與高速重複性；10/26單次SPEED結果不代替這些項目。
3. 在HX品質與靜態驗證完成後，再驗計時、緩衝、同步及動態量測，之後才進入材料比較；`ARM`不會自動釋放落體。
4. 如日後加入電磁鐵自動釋放，另核Relay電源、觸發邏輯、飛輪二極體與機械條件；目前未啟用。

10/21～22的VIN／激勵電壓、電表照片、約5mm間隙及當時UART失聯均保留於[歷史交接](handoff.md#hx711-voltage-serial-20261021)與[200g更正報告](docs/calibration/2026-10-21-hx711-correction-200g.md)，不冒充目前同步供電或機械量測。

**追查紀錄：**[完整交接 handoff.md](handoff.md) · [改版前 README 歷史快照](docs/archive/README-before-redesign-20261021.md)

歷史快照保留原有各版接線、排查數據、編譯／燒錄指令及參考資料；舊GPIO、線色、NodeMCU-32S與OS25B10描述按日期閱讀。接線依本頁layout2與實線核對，已部署程式依該次來源hash／部署紀錄識別，工作中的候選來源不當成已燒錄版本。

<details>
<summary>主要原廠參考資料</summary>

- [ADXL375 資料表](https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL375.pdf)
- [Adafruit ADXL375 端子](https://learn.adafruit.com/adafruit-adxl375/pinouts)
- [HX711 資料表](https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf)
- [Adafruit HX711 端子與 VIO 焊橋](https://learn.adafruit.com/adafruit-hx711-24-bit-adc/pinouts)
- [Espressif ESP32 資料表](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- [U8g2 API 與 OLED 設定](https://github.com/olikraus/u8g2/wiki/u8g2reference)

</details>
