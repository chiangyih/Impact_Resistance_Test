# 2026-10-27 USB重接後HX711串列紀錄與新校正

日期／時區：2026-10-27，Asia/Taipei（UTC+08:00）。硬體及擷取操作者：主代理；文件與新資料核算：Planck。

**USB重接後已取得新的自動串列資料；本輪各一次TARE及CAL 241均有裝置成功回應。** 新offset列印值為−158294.438 counts，新尺度為20.577282158 counts/g。首次卸載60秒窗口的59筆STATUS，用本輪列印參數換算均值約−0.771921g；這是短窗觀察，不代表回零精度或獨立準確度合格。後續完成一次重放與再卸載；再卸載後兩個空載窗平均分別約−4.151g及−7.026g，零點仍向負變化，原因與精度待確認。

本文件只使用下列USB重接後的新serial、host及逐行JSONL，沒有使用或分析先前OLED數值，也沒有將未擷取間隔接成連續ADC序列。載荷／USB實體動作依本輪主代理與使用者確認的背景記錄，非文件整理者親見。

## 新證據與主機流程

- A：重接後空托盤被動窗口：[serial原文](20261027-hx711-241g-auto-capture-182156.serial.log)、[逐行JSONL](20261027-hx711-241g-auto-capture-182156.lines.jsonl)、[host](20261027-hx711-241g-auto-capture-182156.host.json)、[控制紀錄](20261027-hx711-241g-auto-capture-182156.control.log)、[stderr](20261027-hx711-241g-auto-capture-182156.stderr.log)。
- B：一次新TARE：[serial原文](20261027-hx711-241g-new-tare-182308.serial.log)、[逐行JSONL](20261027-hx711-241g-new-tare-182308.lines.jsonl)、[host](20261027-hx711-241g-new-tare-182308.host.json)、[控制紀錄](20261027-hx711-241g-new-tare-182308.control.log)、[stderr](20261027-hx711-241g-new-tare-182308.stderr.log)。
- C：一次新CAL 241及留置砝碼：[serial原文](20261027-hx711-241g-new-cal-182456.serial.log)、[逐行JSONL](20261027-hx711-241g-new-cal-182456.lines.jsonl)、[host](20261027-hx711-241g-new-cal-182456.host.json)、[控制紀錄](20261027-hx711-241g-new-cal-182456.control.log)、[stderr](20261027-hx711-241g-new-cal-182456.stderr.log)。
- D：首次卸載被動窗口：[serial原文](20261027-hx711-241g-auto-capture-182633.serial.log)、[逐行JSONL](20261027-hx711-241g-auto-capture-182633.lines.jsonl)、[host](20261027-hx711-241g-auto-capture-182633.host.json)。

四組host均記COM3、serial-monitor v0.15.0（commit 13b8e80），OPEN前選定115200／8N1、DTR／RTS=off，監控器OPEN ACK及本機TCP資料通道連線成功。這些控制ACK是監控器回報；本次裝置資料是否存在，另以serial原文及TARE／CAL裝置回應確認。

| 階段 | 主機擷取窗口（+08:00） | 窗長 | 裝置TX／接收保存 |
|---|---|---:|---|
| A | 18:20:56.659～18:21:56.672 | 60.013秒 | 無裝置TX；19741 bytes，120完整行 |
| B | 18:22:53.231～18:23:08.744 | 15.513秒 | 一次TARE；4326 bytes，28完整行 |
| C | 18:24:34.024～18:24:56.045 | 22.021秒 | 一次CAL 241；6346 bytes，40完整行 |
| D | 18:25:33.665～18:26:33.668 | 60.003秒 | 無裝置TX；19588 bytes，118完整行 |

上述時間來自host控制事件，由UTC換成+08:00；JSONL的時間是主機收到完整文字行的時刻，不是HX711轉換時刻。檔名時間不代替實際窗口。各組host均為overflow=false、尾端不完整bytes=0、failure=null；CLOSE／QUIT ACK成功、程序exit=0並釋放埠。主代理沒有在這些窗口中額外切換DTR／RTS、清佇列或主動重置；USB重接是本輪已回報的實體操作，不能由此補造未捕獲的啟動時刻。

## A：重接後空托盤新串列基線

新host記錄使用者已重接USB、空托盤、USB直連電腦且TX0／RX0無額外線。A含60筆STATUS及60行WiFi；60筆RX均不同，1105→1664，增加559。全部STATUS為normal IDLE／NONE／UNCAL、HX=OK、ADXL=OK，ERR／AERR／FIFO_FULL均0；AGE_ms有59筆15、1筆197。這支持本窗快取持續更新，不推精確採樣率、零漏樣或長期健康。

| A的RAW快照統計 | 結果 |
|---|---:|
| 觀察數／不同RX | 60／60 |
| 平均（counts） | −158344.316667 |
| 最小／最大（counts） | −158449／−158247 |
| 跨度（counts） | 202 |
| 首筆／末筆（counts） | −158320／−158296 |
| 快照樣本標準差（n−1，counts） | 43.622996 |

A的首末STATUS主機收行時間為18:20:57.116及18:21:56.318。以上以全部60筆STATUS等權計算；它們是約每秒快取快照，不是全部ADC框或已確認獨立同分布樣本。UNCAL階段不換算公克、不宣告穩定／精度合格。60行WiFi均為CONNECTED、radio=1、IP=192.168.52.71，只支持該窗口的回報，不代替外網或射頻驗證。

本組新資料沒有自然開機banner或實板VERSION字串；沒有為補識別額外重置，也不從接腳／LAYOUT推定版本。版本識別與USB晶片唯一識別未由這組資料補足。

## B：一次新去皮

B的裝置TX時間為18:22:56.733。裝置原文及主機收行時間：

```text
18:22:56.822  # TARE_START remove external load and hold still; 32 fresh frames; STOP cancels
18:23:00.007  # TARE_OK offset=-158294.438; UNCAL; use known mass with CAL grams
```

B共有13筆不同RX的STATUS：TARE_START前4筆、TARE_OK後9筆。成功後9筆RX2268→2343，均IDLE／UNCAL、HX／ADXL=OK，ERR／AERR／FIFO_FULL均0；RAW均值−158319、範圍−158391～−158200、跨度191 counts。這些後續快照不等於TARE內部32個新框；內部框沒有逐筆輸出，不能事後還原。

TARE成功只建立本輪新零點，當時仍UNCAL、尚無有效新尺度。offset按裝置列印的三位小數保存，不補造未列印內部位數。TARE後WiFi回報SUSPENDED／radio=0／IP=NA；未以此推定其他時段的連線。B沒有ABORT／FAULT或第二次TARE回應。

## C：同組新零點的一次CAL 241

依主代理本輪轉達，使用者已明確確認241g放穩，主代理在18:24:44.035只送一次CAL 241。裝置原文及主機收行時間：

```text
18:24:44.160  # CAL_START hold the stated known mass still; 32 fresh frames; STOP cancels
18:24:47.387  # CAL_USER counts_per_g=20.577282158; static user calibration, not independently validated
```

本輪參數配對為B的offset=-158294.438 counts及C的counts_per_g=20.577282158。C共有19筆STATUS，前10筆UNCAL，後9筆CAL_USER且RX3280→3355均不同。後9筆RAW均值−153335.444444、範圍−153407～−153291、跨度116 counts。按本輪列印參數在主機重算`mass_g = (RAW - offset) / counts_per_g`，均值240.993612g、範圍237.516207～243.153491g；這是同一標定砝碼仍留置的短窗，不是獨立準確度或重放驗收。

C已見STATUS均HX／ADXL=OK、ERR／AERR／FIFO_FULL為0。CAL成功後WiFi為SUSPENDED／radio=0／IP=NA。CAL內部32個新框沒有逐筆輸出，STATUS本身也沒有讀回精確offset／尺度；本文件依本輪成功回應配對列印值，沒有將CAL_USER旗標當精確參數查詢。

**C metadata的載荷欄位補充：** C的host沿用`user_observations.tray_empty=true`，它只描述USB重接時的初始空載，不是CAL當時載荷；CAL階段依使用者明確確認241g已放穩。Ampere已指出此欄位延續問題，本文件予以註明，原始host不改寫，也不把CAL時解讀成空載。

## D：首次卸載串列觀察

D按主代理本輪卸載階段保存。59筆STATUS均為不同RX，3723→4275；首末收行時間18:25:34.531～18:26:32.731。全部為IDLE／CAL_USER、HX／ADXL=OK，ERR／AERR／FIFO_FULL均0。WiFi在此窗口回報CONNECTED、radio=1及IP=192.168.52.71，不將CAL時暫停狀態延伸到本窗。

| D的卸載快照統計 | 結果 |
|---|---:|
| 觀察數／不同RX | 59／59 |
| RAW平均（counts） | −158310.322034 |
| RAW最小／最大（counts） | −158417／−158200 |
| RAW跨度（counts） | 217 |
| 依本輪列印參數條件換算均值（g） | −0.771921 |
| 同口徑換算範圍（g） | −5.956180～4.589430 |

這是本輪固定零點／尺度下的短窗卸載描述。列印參數有限位數、快照與完整ADC序列不同，且尚無獨立標準比較，**不能把均值接近0當作回零精度合格**。不為消除偏移再次去皮，不自行設定允許誤差或推定動態撞擊能力。

## 保存核對、結論與接續

Planck逐行核對A／B／C／D的JSONL raw_line與serial原文，行數分別120／28／40／118、逐行內容一致；四份serial SHA256亦與各自host所存值一致。這是保存完整性核對，不是量測準確度驗收。

| 原始serial | SHA256 |
|---|---|
| A | `41570C9E6B828DBA413D804ECBE4FA32BEF4524A24FDF8B918E2B76A5B3BC799` |
| B | `BCBCB908DFF1C6BE63B7C4C30B17E255A5DFFCAAA98B4DF61FCD0BC6F84BEC26` |
| C | `C8DBDFD51CC908B757ECEC931F4C70515AA0DBA8346E6B15AD63C8758D0983EF` |
| D | `B5A23A05C7B0E444D834BBAFF8903E34FBB4BB78E5B54EB1C10241D7D86139C2` |

目前可支持新串列接收／STATUS更新，以及本輪各一次TARE／CAL命令成功回應；新零點與尺度已建立，不能據此宣告量測系統通過。尚待多循環固定條件重複性、獨立準確度與驗收界線確認；啟動版本與完整功能QC亦未由本文件補足。沒有把不同開埠窗口或未擷取間隔視為連續觀察，也不以零錯誤快照宣稱全部框零錯誤。

後續使用者重放241g並再次卸載，主代理完成自動記錄；結果見下節。各已保存窗口均關閉／釋放COM3，沒有常駐監控。文件整理者未開埠、讀秘密或建置產物、修改原始證據／handoff／其他文件，亦未執行Git；後續由主代理安排、Ampere依新證據獨立核對。


## 後續重放與第二次卸載

使用者確認同一組241g已重新放穩；[重放原始串列](20261027-hx711-241g-reload-capture-182908.serial.log)、[逐行時間](20261027-hx711-241g-reload-capture-182908.lines.jsonl)、[主機資料](20261027-hx711-241g-reload-capture-182908.host.json)。18:28:09.054～18:29:08.262台北主機收行時間有60筆不同RX，RAW平均−153337.316667 counts，依本輪列印offset/scale換算平均240.902627g，範圍235.766704～245.437564g。這是同一標定砝碼一次重放，非獨立準確度或多循環重複性驗證。

使用者隨後確認再卸載，主代理未再TARE，保存[第二卸載原始串列](20261027-hx711-241g-auto-capture-183040.serial.log)、[逐行時間](20261027-hx711-241g-auto-capture-183040.lines.jsonl)、[主機資料](20261027-hx711-241g-auto-capture-183040.host.json)。18:29:40.376～18:30:39.592為60筆不同RX，RAW平均−158379.85 counts，依本輪參數換算平均−4.150791g、範圍−9.406587～−0.270298g。比第一次卸載平均−0.771921g低3.378870g。

維持空載又收集[續查原始串列](20261027-hx711-241g-auto-capture-183158.serial.log)、[逐行時間](20261027-hx711-241g-auto-capture-183158.lines.jsonl)、[主機資料](20261027-hx711-241g-auto-capture-183158.host.json)。18:30:58.656～18:31:57.862為60筆不同RX，RAW平均−158439.016667 counts，換算平均−7.026130g、範圍−11.642062～−1.485230g，三個20秒段的換算均值−6.211316／−7.025320／−7.841755g。續查平均比第二卸載窗低2.875339g；兩窗之間約19秒沒有資料，不能拼成連續逐框紀錄或推長期漂移率。

三個新窗口均IDLE／CAL_USER／HX=OK、ERR／AERR／FIFO_FULL=0；Ampere核對原始bytes、JSONL、SHA256、時間與關埠。這些狀態不能消除觀察到的空載變化；回零穩定及獨立靜態準確度尚未驗證，原因未定位。無需用目視數值；新輪所有計算僅用新串列資料。COM3已關閉，沒有常駐監控；韌體未修改、未燒錄、未commit或push。
