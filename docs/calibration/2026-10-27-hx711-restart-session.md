# 2026-10-27 HX711重新重量校正紀錄

日期／時區：2026-10-27，Asia/Taipei（UTC+08:00）。硬體操作者：主代理；文件整理：Planck。

**本次241g的TARE與CAL程序已接受：offset列印值=-157326.312 counts，counts_per_g=20.413381743，最後成功serial讀回CAL_USER；正式回零／重放與靜態準確度尚未驗證。** CAL後同一標定砝碼留置短窗11筆換算均值240.089532g，不是獨立精度結果。取下241g後，使用者回報OLED CAL_USER及3筆RAW：−157785、−157622、−157642，均為IDLE、A:0 H:0；HX健康狀態未提供，精確取值時間／間隔未知。供電與精確RAM係數連續性仍未確認；若僅假設保存的同組係數仍適用，3筆平均換算約−17.473244g，屬條件性偏移跡象，不能列正式回零結果。最新約8.127秒唯讀窗雖有3個主機TX仍0字元、已關埠，命令交付未確認；沒有重置或重新校正。

## 範圍與證據

- [前檢終端原始工具chunks](20261027-hx711-restart-precheck.terminal.json)：`host-terminal-observation-v1`，包含開埠被動觀察、查詢嘗試、續讀及Ctrl+C關閉的工具輸出；原chunks保留不清洗。
- [重接後唯讀serial log](20261027-hx711-241g-readonly-165458.serial.log)及[主機操作metadata](20261027-hx711-241g-readonly-165458.host.json)：無PTY的.NET有界擷取，取得VERSION、SPEED STATUS及STATUS；主機起迄為16:54:58.7785661～16:55:07.3863863（+08:00），該窗完成後已關埠。
- [空托盤基線serial log](20261027-hx711-241g-empty-baseline-165536.serial.log)及[主機操作metadata](20261027-hx711-241g-empty-baseline-165536.host.json)：使用者確認空托盤後的無命令被動擷取，16:55:36.4213615～16:56:21.4560377（+08:00），elapsed_ms=45033；45筆不同RX的STATUS快照，該窗已關埠。
- [一次TARE serial log](20261027-hx711-241g-tare-165741.serial.log)及[主機操作metadata](20261027-hx711-241g-tare-165741.host.json)：16:57:41.5862755～16:57:50.3304935（+08:00），僅TX一次TARE，保存TARE_START→TARE_OK及新offset；該窗已關埠。
- [一次CAL 241 serial log](20261027-hx711-241g-cal-165924.serial.log)及[主機操作metadata](20261027-hx711-241g-cal-165924.host.json)：使用者回覆「241g已放穩」後，16:59:24.1409341～16:59:39.7210687（+08:00）僅TX一次CAL 241，保存CAL_START→CAL_USER與留置砝碼短窗；該窗已關埠。
- [首次卸載嘗試的serial檔](20261027-hx711-241g-unload1-170057.serial.log)及[失敗metadata](20261027-hx711-241g-unload1-170057.host.json)：使用者回覆「241g已取下，空托盤靜止」後，17:00:57.8790142～17:00:57.8966799（+08:00）Open立即失敗，serial檔0字元、沒有RX或TX；是失敗證據，不能當回零紀錄。
- [COM3重新出現後的0字元serial檔](20261027-hx711-241g-unload-readback-171013.serial.log)及[主機metadata](20261027-hx711-241g-unload-readback-171013.host.json)：17:10:13.8229765～17:10:16.3584373（+08:00），Open成功後被動觀察約2.5秒，未取得normal IDLE或任何字元，沒有TX，已關埠；不是20秒成功回零窗。
- [OLED目視回覆後的0字元serial檔](20261027-hx711-241g-oled-followup-171835.serial.log)及[主機metadata](20261027-hx711-241g-oled-followup-171835.host.json)：17:18:35.8220372～17:18:43.9499498（+08:00），elapsed_ms=8127；基於新目視IDLE／RAW變化，Open成功後分別TX VERSION、STATUS、SPEED STATUS，仍沒有任何接收字元，交付未確認，已關埠。
- [三筆OLED原文、條件性核算與追溯JSON](20261027-hx711-241g-oled-unload-observation.json)：保存使用者來源、HX健康／實際間隔未知、係數假設及新查詢與USB診斷的SHA256；不是正式回零驗收。
- [17:21唯讀USB診斷](20261027-hx711-241g-USB-readonly-diagnostics.json)：只讀既有目標裝置屬性及有界System事件查詢，不操作serial、掃描／重設裝置或變更驅動；未定位根因。
- [交接紀錄](../../handoff.md)：由主代理統一更新；本文件只整理本次重量校正進度。
- [歷史200g更正報告](2026-10-21-hx711-correction-200g.md)：只供追溯2026-10-21兩輪錯輸入CAL 400的歷史；不能將其200g、offset或比例預填成本輪條件。

16:41前檢由主代理核對目標為COM3／USB-SERIAL CH340、VID1A86／PID7523，獨占操作Arduino CLI監控。監控列示115200、8N1、DTR=off／RTS=off。該份保存內容的主機時間標示約為16:41:02～16:41:45（+08:00），不是精確開關埠邊界或逐ADC框時戳。

16:41前檢沒有送TARE／CAL／UNCAL／CLEAR／STOP，主代理未主動重置、燒錄或改線／供電。該段由主代理以Ctrl+C終止主機監控後已關閉及釋放COM3；工具exit=0只表示監控程序完成，不是校正或功能QC通過碼。後續使用者確認的USB重接、基線及一次TARE另列下節，不把不同窗口拼成連續擷取。文件整理者未操作COM3。

## 16:41前檢觀察及判讀限制（歷史窗口）

| 項目 | 已見觀察 | 本輪可支持的判斷／待補 |
|---|---|---|
| 一般狀態 | 自然每秒STATUS顯示phase=IDLE、reason=NONE、CAL=UNCAL | 當時一般狀態為待機、未校正；不是所有模式均已確認 |
| HX711 | HX=OK、RX6665→7068，ERR首末均0 | 本窗STATUS快取有更新、已見HX錯誤累計沒有增加；不能據此宣告靜態穩定、零漏樣或校正合格 |
| RAW及載荷條件 | 有RAW快照；空托盤與外加重量未確認 | 不定義為空載、不換算公克、不用其均值建立新offset |
| ADXL／FIFO | ADXL仍顯示OK，但AERR由0增至905，FIFO_FULL由0增至3 | 存在本窗錯誤／警示計數增加，根因未知；不能用OK標籤結案，也不將計數直接視為漏樣數或已知故障原因 |
| 設定快照 | STATUS顯示POL=-1/-1、DIST_mm=220.000、SCK25／DATA33 | 為本窗讀回；不沿用前日POL1/1或將220mm讀回視為幾何精度證明 |
| 版本及SPEED狀態 | 曾經由主機stdin嘗試VERSION／SPEED STATUS／STATUS，未取得可唯一辨識的查詢回應 | 指令送達、實板版本及SPEED狀態仍待確認；自然STATUS不能冒充這些查詢已成功，也不能據此確診命令介面失效 |

Ampere已獨立核對前檢證據，結論由主代理轉達為：**前檢部分通過、校正待執行、ADXL／FIFO異常待查**。HX=OK、RX增加403且ERR維持0支持本窗HX資料持續更新；沒有VERSION／SPEED可辨識回應、未知現場荷重及尚無新offset／比例的限制均保留。這是校正前檢判定，不能當作重新校正或完整功能QC通過。

終端輸出含ANSI游標控制碼、折行與邊界重複字元。JSON保存的是工具擷取的PTY輸出，**不是位元組精確的serial原流，也不是乾淨逐框ADC紀錄**。未將折行內容補造成新的原始STATUS或CSV，不從約每秒快照推精確採樣率。主機時間、工具等待時間與裝置採樣時刻保持分開。

ADXL／FIFO現象須保留證據並另行定位，不能先盲改取樣、清警示或放寬驗收。後續物理複核對一次暫定TARE的判斷見下節，不代表前檢異常已定位或完整量測驗收通過。

## 現場條件回覆、USB重接及新窗查詢

使用者後續確認：①空托盤已裝妥、靜止無其他物品；②本次校正總重241g；③曾斷電／USB重接，荷重元接線未改。這是使用者現場回報，不是文件整理者親見或獨立秤量；其他機械安裝改動未明確，重量的獨立追溯與不確定度未提供。

約16:53，主代理的三種主機枚舉暫時均未見COM3。詢問後使用者回覆「USB已接妥」，重新枚舉已見COM3／CH340 VID1A86、PID7523。這是不同時點的主機觀察與現場回報，不據此推定第一次消失的根因。

主代理其後改用無PTY的.NET SerialPort，一次有界唯讀擷取，115200／8N1、Handshake=None、DTR=false／RTS=false；host metadata記載capture_error=null、port_closed=true。讀回如下：

| 項目 | 16:54:58～16:55:07新窗證據與判定 |
|---|---|
| 實板VERSION | `v1-layout2-fifo-diag1-wifi1-d220-right-optical1`，完整可辨識VERSION回應已取得；不是自然boot banner |
| SPEED | id=0、state=OFF、reason=NONE、A=0／H=0，dt及speed均NA；明確支持本窗手動光學模式未啟動，沒有有效速度結果 |
| 一般狀態／校正 | normal phase=IDLE、reason=NONE、CAL=UNCAL、HX=OK、ERR=0；尚未重新校正 |
| HX更新 | 本窗STATUS的RX501→578；只在該窗比較，不與16:41窗口拼接或推精確資料率 |
| ADXL／FIFO | 本窗STATUS為ADXL=OK、AERR=0、FIFO_FULL=1；不同供電／重接窗口的較低計數不構成先前異常已修復的證明 |
| 命令交付 | metadata記錄VERSION、SPEED STATUS及STATUS的TX，serial有完整VERSION與SPEED回應；新窗已補足可辨識唯讀查詢證據，16:41前檢「未取得回應」仍保留為歷史 |
| 關埠 | 該次有界擷取結束後已關閉COM3；隨後空托盤基線及一次TARE分窗保存，結果見下節 |

前後已經過使用者確認的斷電／USB重接歷程，計數、RAM與模式均以各窗實際讀回為準。不能把RX、AERR或FIFO_FULL跨窗相減當作連續變化，也不能以本窗AERR=0或命令可回應推定前窗問題原因已定位。新serial log同樣只是STATUS快照，不是每個ADC框；host起迄／TX時刻不是ADC時戳。

## 約45秒空托盤新基線與一次TARE

基線由主代理獨占COM3被動擷取，host metadata記錄commands=[]、capture_error=null、port_closed=true。Planck依完整serial log的STATUS行重新核算如下，不使用前檢折行RAW或歷史counts/g：

| 指標 | 本窗結果與統計口徑 |
|---|---|
| 快照與更新 | 45筆STATUS、45個不同RX，RX861→1277；是約每秒快照，不是全部ADC轉換框 |
| 狀態／錯誤 | 全部normal IDLE／UNCAL／HX=OK，ERR=0、AERR=0、FIFO_FULL=1；只描述此窗 |
| RAW範圍／跨度 | −157464～−157262 counts，跨度202 counts |
| RAW均值 | −157369.0888889 counts，45個快照等權平均；不是裝置TARE的32框平均 |
| 三段觀察均值 | 按觀察順序每15筆分段：−157379.0000、−157398.2000、−157330.0667 counts；不是三段精確ADC時間窗 |

這些數值只支持已保存的短窗空托盤觀察，尚不能宣稱長期穩定或精度合格，也不套用舊比例換算成公克。Kierkegaard的物理複核由主代理轉達：目前沒有已確認必須停止的理由，可進行一次暫定TARE；這不是放寬準確度驗收，也不是確認零點穩定。

主代理其後只送一次TARE，host TX為16:57:43.8283902（+08:00），保存以下完整回應：

```text
# TARE_START remove external load and hold still; 32 fresh frames; STOP cancels
# TARE_OK offset=-157326.312; UNCAL; use known mass with CAL grams
```

TARE log未見ABORT，host metadata為capture_error=null、port_closed=true，僅有一次TARE TX；新offset依裝置列印值保存，不補造內部未列印位數。裝置宣告收集32個新框，但這32框沒有逐筆輸出，不能拿45筆前置STATUS重建或冒充。TARE後仍為UNCAL，舊比例失效；此時沒有新的counts_per_g，不能輸出有效公克結果或宣告CAL完成。

TARE後已見HX=OK、ERR=0、AERR=0、FIFO_FULL=1，WiFi=SUSPENDED、radio=0。serial尾端的`# WIFI state=SUSPENDED reque`是不完整fragment，原檔原樣保留，沒有補齊成完整WiFi回應；完整前行已可支持本窗暫停狀態，不能據此宣告WiFi、ADC動態或完整QC均通過。Ampere已獨立核對TARE程序成功，此結論不等於回零或準確度驗收。

主代理當時通知使用者放置本次241g砝碼並詢問「241g已放穩」，沒有預先批次送CAL或重複TARE。使用者後來已回覆「241g已放穩」，接續的CAL與卸載嘗試見下節。

## 一次CAL 241及標定砝碼留置短窗

使用者確認「241g已放穩」後，主代理於16:59:26.3702120（+08:00）只送一次`CAL 241`。serial保存以下完整回應，未見ABORT；host metadata為capture_error=null、port_closed=true，並註明CAL使用的32個新框沒有逐筆輸出：

```text
# CAL_START hold the stated known mass still; 32 fresh frames; STOP cancels
# CAL_USER counts_per_g=20.413381743; static user calibration, not independently validated
```

本次比例與前一步TARE的offset列印值綁定；主機換算為`mass_g = (RAW - (-157326.312)) / 20.413381743`。offset只列印三位小數，以下使用列印值計算，不能補造其內部完整精度。Ampere、Gibbs及Kierkegaard均已獨立核對；Planck亦從完整serial重新核算：

| 指標 | CAL_USER後留置同一241g砝碼的結果 |
|---|---|
| 快照／RX | 11筆STATUS、11個不同RX，3052→3145；不是CAL內部32框 |
| 換算均值 | 240.089532g |
| 換算範圍 | 237.996431～242.895178g |
| 相對使用者回報241g的均值差 | −0.910468g，約−0.377787% |
| 狀態／警示 | normal IDLE／CAL_USER／HX=OK；已見快照ERR=1、AERR=0、FIFO_FULL=1；WiFi SUSPENDED、radio=0 |

這是同一標定砝碼仍留置的短窗讀值與換算，支持TARE／CAL程序已接受及新比例可用於該窗重算，**不構成獨立靜態準確度或重複性合格**。重量獨立追溯與不確定度、卸載回零、重新放置同重量及再卸載尚未完成，不以CAL_USER成功回應取代這些驗證。

TARE末筆RX2116的ERR=0；CAL開窗、尚未送CAL時RX3003已為ERR=1，CAL末筆RX3145仍為ERR=1。增加是在未擷取間隔被發現，不能定位發生時刻或根因，也不能說是CAL造成、以HX=OK抵消或直接當成漏樣量。AERR=0／FIFO_FULL=1只描述本窗，前檢歷史問題繼續保留。

## 卸載回零嘗試失敗與目前待確認狀態

使用者後續回覆「241g已取下，空托盤靜止」，主代理依此準備以固定新offset／比例做首次回零觀察，不重新去皮。17:00:57的.NET `Open`立即失敗：`Could not find file 'COM3'.`，沒有送命令，receive_observations=[]，serial檔為0字元。host metadata內的offset／比例只是引用既有來源，並非這次從裝置新讀回的係數或校正仍保留的證明。

約17:01，主代理回報三種主機枚舉只見COM1、未見COM3。**回零沒有資料：RAW、換算公克與回零誤差均為NA／未測，不能填0。** COM3未被枚舉不能單獨證明裝置重啟、斷電或RAM係數已消失；目前RAM校正仍未知，不沿用最後CAL_USER當作當前狀態。

主代理當時詢問USB／供電與OLED是否顯示CAL_USER或UNCAL，保留現況而未主動重置或再TARE；使用者後續回覆及COM3重新出現後的一次被動讀取另列下節。首次Open失敗與最後成功CAL窗關埠的歷史保持原樣，不以後續Open成功推定首次失敗原因。

## OLED回覆、枚舉變動及約2.5秒空白被動讀取

使用者先回覆OLED「顯示CAL_USER」。主代理再核對CAL 241後是否全程供電、未拔插或重置，使用者答「不確定」。此為現場目視與歷程回報，不是新的serial係數讀回。**OLED顯示CAL_USER不能證明本次offset／比例的精確值或供電／RAM連續性**，因此不將當前RAM標成已確認。

主代理回報17:08:06的三種枚舉仍只見COM1；17:08:46同一CH340 InstanceId已為IsPresent=true、ProblemCode=0，稍後PortNames及PnP亦見COM3。這些是不同時點的主機狀態，變動原因仍未知，不能假定連線根因已修復或裝置沒有重啟。

主代理其後對正確目標做一次無PTY的.NET開埠，115200／8N1、DTR=false／RTS=false。host metadata記錄OPEN成功，但elapsed_ms=2534的被動觀察沒有收到任何字元；未取得normal IDLE，所以沒有送VERSION、STATUS或其他查詢。其capture_error為`No passive normal IDLE evidence; no query sent`，events只有OPEN、沒有TX，port_closed=true，serial檔0字元。這只是一個約2.5秒的空白被動窗口，沒有形成20秒成功回零資料，不能把0字元解讀為RAW=0或回零通過。

主代理當時請使用者只看OLED，約隔3秒提供3筆RAW及phase／HX狀態；後續已取得的目視原文及一次唯讀查詢另列下節。17:10:13窗口沒有TX、0字元及已關埠的歷史不改寫，不能與新窗口的三個TX混記。本窗沒有送重置／TARE／CAL，原本校正程序成功的歷史結論保留。

## 三筆OLED目視RAW、條件性換算及約8.127秒空白查詢窗

使用者新原文為「-157785、-157622、-157642；IDLE都是:A:0 H:0」。三筆依先前約隔3秒的請求抄錄，但實際精確時間與間隔沒有確認；這是使用者目視來源，不能指認為3個獨立新ADC框。荷重條件承接先前「241g已取下，空托盤靜止」回報，並非本次另做的獨立空載確認。

三筆RAW均值為−157683 counts，範圍−157785～−157622、跨度163 counts。A／H是一般trial的ACC／HX記錄筆數（acc_count／hx_count），A:0 H:0不能當作HX健康狀態、RX更新計數或公克；本回覆的HX健康狀態仍未提供。RAW數值有變化，只支持這三筆目視數字不同，不足以建立精確資料率、穩定性或供電連續性。

**僅在保存的offset=-157326.312與counts_per_g=20.413381743仍適用的假設下**，主機依`(RAW-offset)/counts_per_g`條件性重算如下，Planck已獨立核算：

| 目視順序 | 使用者RAW（counts） | 假設原係數仍適用的換算（g） |
|---|---:|---:|
| 1 | −157785 | −22.469966 |
| 2 | −157622 | −14.485008 |
| 3 | −157642 | −15.464758 |
| 三筆均值 | −157683 | −17.473244 |

這些是條件性偏移跡象，**不是已驗證的回零公克或誤差**；當前精確RAM係數與未重啟連續性沒有讀回確認，空載承接條件與目視時間也有限制。不能僅因數字偏離0便確診根因、反覆TARE清掉偏移或宣告校正／回零通過。

主代理約17:17再次核對同一COM3／CH340列舉正常，基於新目視IDLE及RAW變化做一次無PTY的.NET有界唯讀查詢；DTR／RTS均false。新host metadata記錄OPEN成功，三個主機TX依序為VERSION、STATUS、SPEED STATUS，但全窗0字元，capture_error為`No device characters after bounded read-only queries; no retry or reset`，port_closed=true。TX只證明主機曾寫入，裝置是否收到或執行仍未確認，沒有取得新的版本／狀態／SPEED／精確校正值。

本窗依host metadata為elapsed_ms=8127、約8.127秒；工具wall_time若含其他開銷，不能替代實際擷取窗。這與17:10:13窗口的2534ms／零TX分開。Ampere已核對新窗三個唯讀TX、0字元與關埠，以及條件性OLED均值；結論仍為程序已接受、存在條件性偏移跡象，正式回零未驗。主代理未重試、重置或改校正，沒有持有COM3或常駐監控。

17:21唯讀USB診斷保存目標IsPresent=true、ProblemCode=0、Windows LastArrivalDate=17:08:06.362744（+08:00）、既有wch.cn驅動3.9.2024.9及可識別父USB Hub；這些屬性不能證明通訊已修復或MCU供電連續。有界System事件查詢回傳0筆，不能據此宣稱沒有USB事件或已找到根因。原文、條件性換算及診斷追溯已另存上述JSON，未改動舊校正summary快照。

17:24:03主代理另做一次有界CLI對照：`monitor --timestamp --raw`、DTR／RTS=off，stdout／stderr直接寫檔避免PTY折行，未送裝置命令。證據為[host metadata](20261027-hx711-241g-cli-compare-172403.host.json)、[monitor輸出](20261027-hx711-241g-cli-compare-172403.monitor.log)及[stderr](20261027-hx711-241g-cli-compare-172403.stderr.log)，兩輸出均0字元，連線確認亦未取得，不能宣稱CLI成功開埠或區分.NET／UART／USB／韌體原因。主代理Ctrl+C結束後，17:24:19核對已無arduino-cli／serial-monitor程序、沒有持有埠；exit=0只表示主機程序結束。此次未再TARE／CAL／重置，沒有反覆無界重試；正式回零及精度仍未驗。

## 已確認條件與校正待補欄位

| 欄位 | 本輪狀態 |
|---|---|
| 托盤是否保留且已移除所有校正砝碼／外加荷重 | 前置空托盤已確認；CAL後使用者回覆241g已取下、空托盤靜止；3筆OLED RAW承接該條件，沒有新的獨立空載確認 |
| 機械受力路徑、是否碰底／卡住及安裝改動 | 荷重元接線未改；其他機械改動與受力細節尚未明確 |
| 本次校正砝碼總重 | 使用者確認241g；不沿用歷史200g或400g，獨立重量追溯與不確定度未提供 |
| 前次操作後斷電／USB重接／重啟歷程 | 前置確認曾斷電／USB重接、約16:53後USB接妥；對CAL後全程供電／未拔插或重置回覆「不確定」，不推定重啟或連續供電 |
| 實板VERSION與SPEED保存／活動狀態 | 16:55新窗已取得完整optical1 VERSION及SPEED id0 OFF／A0H0；最新目視回報IDLE，新查詢窗3個TX仍0字元；當前serial版本／模式、精確RAM係數與連續性仍未確認 |
| 新空載基線、TARE成功回應與offset | 約45秒／45不同RX基線已保存；一次TARE_START→TARE_OK，offset=-157326.312 counts；Ampere核對程序成功，TARE當時UNCAL、其後已CAL_USER |
| 241g砝碼放置／移除 | 已回覆241g已放穩並執行CAL；其後已回覆241g已取下、空托盤靜止 |
| CAL成功回應與counts_per_g | 一次CAL_START→CAL_USER，20.413381743 counts/g；程序已接受，該窗後關埠；OLED後來回報CAL_USER但不足證明當前相同精確係數仍保留 |
| 卸載回零 | 3次serial嘗試都無接收資料；OLED已有3筆RAW，但同組精確係數未確認，條件性換算不列正式回零結果，正式回零公克／誤差仍NA／未驗 |
| 最新OLED RAW／phase／HX目視 | −157785、−157622、−157642；均IDLE、A:0 H:0；RAW均值−157683／跨度163 counts；HX健康狀態與精確時間間隔尚未提供 |
| 已知重量重新放置、再卸載及穩定性 | 尚未測試；同一標定砝碼留置11筆不能替代重放或儀器重複性，驗收界線待確認 |
| 校正有效性／合格結論 | TARE／CAL程序已接受，OLED有條件性偏移跡象；正式回零／重放及獨立靜態準確度未驗，不能總判通過 |

## 接續步驟

1. 保存已取得的三筆OLED原文、條件性換算及HX健康／時間未提供的限制；保留供電連續性「不確定」及相同精確係數未確認，不自行重置、STOP／CLEAR或反覆TARE。
2. COM3可Open但兩次後續成功開埠窗口均0字元；最新雖有3個主機TX，仍未確認交付。後續由主代理依現場新證據核對目標、串列通訊、實板版本、當前校正／模式及精確係數追溯，再決定是否能以本次同組offset／比例繼續驗證；不能僅以OLED CAL_USER或A:0 H:0視為係數連續／HX正常，也不默默套用舊比例。
3. 確認仍可追溯到本次有效係數及空托盤條件後，先保存真正卸載RAW與回零誤差；再依授權安排同一241g砝碼重新放置、再卸載觀察，全程固定同一組offset／比例，不在驗證中重新去皮。
4. 區分RAW、主機換算值、快取快照與實際新框，補齊回零、重放及穩定性證據；誤差及允許界線依用途與新資料評估，不能只用CAL成功ACK或留置標定砝碼的短窗宣告驗收通過。
5. 由Ampere獨立核對新證據，Gibbs檢查可定位軟體問題，Kierkegaard核對物理與校正界線，Planck整理追溯。若後續修改韌體並成功燒錄，另依固定流程立即執行該版燒錄後QC。

本紀錄截至一次TARE／CAL程序已接受、三筆OLED RAW已有條件性偏移跡象、3次卸載serial嘗試均無接收資料及精確係數連續性未知階段；正式回零／重放、完整校正有效性、量測系統與材料效果均未驗收。至本次補充未修改韌體、未編譯／燒錄、未執行Git；所有成功開埠窗口均由主代理結束並關埠，最新約8.127秒窗口3個唯讀TX、0字元，沒有持有COM3或常駐監控。文件整理者只更新本文件，未開埠或改動原始證據。
