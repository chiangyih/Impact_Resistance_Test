# HX711逐讀摘要協定與離線檢查

候選公開版本：`v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1`。
本文說明程式契約與主機測試，不預先宣稱正式編譯、燒錄、實板功能或量測精度通過；實際結果另見[部署session](2026-10-28-hx711-per-read-diagnostic-session.md)。

## 有界記錄與命令

新boot在setup完成感測初始化後、第一次`pollHx()`前呼叫`hxdiagBegin(micros())`，從該點開始600秒目標觀測窗；不是從USB開埠、第一筆attempt或第一筆主機STATUS開始。初始化可能早於主機監控就緒。沒有自動TARE/CAL、舊係數恢復或診斷重啟命令。

固定ring可存256筆，每筆56 bytes，ring共14336 bytes。平常保留最近127次已完成25脈衝軟體嘗試；DATA未ready而直接返回的poll不產生attempt。以下任一事件首次觸發：原DATA後檢查拒絕、原本接受的正負滿量程，或相對上一個接受值跳變至少1000 counts。1000只作檢索，不是精度／校正驗收門檻，也不濾除、修正或拒收該值。

首事件固定，保存可得前史、觸發筆及最多128筆後史。後窗20秒或總窗600秒到期亦凍結，不因新事件延長。快速失敗可使128筆只涵蓋很短時間；事件靠近總窗尾部會截短後史。期限由主迴圈服務／讀取hook檢查，可能晚到，不能當硬即時上限。凍結只停止ring寫入，原HX輪詢、RX／ERR及量測流程繼續。

沒有事件時，總窗到期只保留最後至多127筆，不代表保存完整10分鐘逐讀資料。`pre_overwritten`明列正常滾動捨棄，`omitted_after_freeze`為凍結後未收錄的完成嘗試；兩者不是ADC漏樣量。`overflow`另表示非預期容量異常。

| 命令／輸出 | 契約 |
|---|---|
| `HX_DIAG STATUS` | 唯讀4行摘要；即使HX不健康也可查。校正中拒絕，既有記錄／光學模式護欄保留。 |
| `HX_DIAG DUMP` | 僅FROZEN可匯出；LIVE／POST時拒絕，不強制凍結。匯出後保留資料，可再讀。 |
| `# HX_DIAG_READY` | 開機摘要，含schema、隨機session及窗口大小。 |
| `# HX_DIAG_FROZEN` | 安全背景狀態下一次通知；不是ADC或MCU對主機命令的識別回覆。 |

建議第一次觀測只被動收取，等凍結通知後再匯出。若未見通知，以一次`HX_DIAG STATUS`查明；不能因主機等待時間到了便聲稱ring已凍結。DUMP是同步大量輸出，會阻塞後續輪詢；它在凍結之後執行，因此不改已保留的窗口，但匯出之後的資料間隔不能視作自然排程。

## 時間、識別與欄位

每次啟動產生隨機8位十六進位session；配合版本、來源雜湊與主機啟動證據識別epoch。session不是永久MAC、保證唯一ID或校正識別。燒錄／重啟後RAM校正與累計重新開始，不能跨epoch沿用或把新零計數當問題已解。

`start_us`／`end_us`是25脈衝區塊外側兩次CPU `micros()`，包含原讀取區塊及後檢查，不量每個SCK high或ADC轉換時刻。診斷沒有新增位元內計時、Serial、配置記憶體或CRC，但前後取時、固定摘要寫入與截止服務仍改變排程，不能稱零干擾。

時間是boot微秒低32位；窗口遠短於其約71.6分鐘週期，區間使用`(end-start) >>> 0`回繞差。`freeze_us`由讀取括弧端點或服務CPU時間提供，不是實體GPIO凍結瞬間。DUMP各行的主機UTC是匯出收行時間，不能貼到當初每個attempt；應保留裝置時間，必要時另以啟動／握手的主機時間括弧建立有限對照。

CSV共21欄：`kind`、下列19個canonical word，及`crc32`。每列`kind=HXRD`。

| 欄位 | 意義 |
|---|---|
| `seq` | 已完成且到達診斷hook的attempt序號，包含拒絕；不是RX或ADC轉換數。未完成便掛死／重啟的嘗試不在內。 |
| `start_us,end_us,attempt_gap_us` | CPU讀取括弧及相鄰attempt開始間隔；首筆gap須看context有效性。 |
| `last_accept_ms_before` | 本attempt更新前、最近成功讀取的boot毫秒快取。 |
| `bits24,raw` | 取得點24-bit字及同字補數解碼；拒絕字也保存，但不寫入有效`hx_raw`。 |
| `rx_before,rx_after,err_before,err_after` | 原HX接收／錯誤累計前後；接受則RX加1，拒絕則ERR加1，無號回繞。 |
| `flags` | bit0 LINK拒絕、bit1 SAT接受滿量程、bit2 JUMP接受大跳，可組合。 |
| `cal_count_after,phase_after,cal_mode_after` | 原處理完成後狀態；phase為0 IDLE、1 ARMED、2 WAIT_DN、3 CAPTURE、4 DONE、5 FAULT；cal_mode為0無收集、1 TARE、2 CAL。 |
| `accepted,dout_after25,pulses_issued` | 原後檢查與接受結果；25是軟體迴圈發出數，不證明晶片收到25個有效邊緣。 |
| `context_flags` | bit0 原先見過成功框；bit1 attempt_gap有效。 |

freeze_reason：1後史滿128筆、2後窗時間到、3總窗時間到、4容量異常。`trigger_flags=0`時trigger時間／序號零欄無效；不能當時刻0事件。`post_complete=1`只代表128筆後史齊全，不代表整個物理事件、觀測期限或測試通過。

CRC32採ISO-HDLC，初／末反相、多項式反向0xEDB88320；19個欄位各轉uint32並以低位byte先行計算，signed RAW按補數uint32。CRC在凍結匯出時計算，僅檢查形成後的匯出完整性；不是ADC自帶CRC，也不能排除取得錯誤或匯出前RAM遭改寫。`bits24`與`raw`同源，一致不能提供獨立ADC真值。

## 離線驗證

只讀公開串列檔，沒有COM、網路或秘密檔操作：

```powershell
node tools/hx-read-diagnostic.cjs docs/calibration/<capture>.serial.log
node tests/05-hx-read-diag.test.cjs
node tests/hx-read-diagnostic.test.cjs
```

分析器要求完整BEGIN／END及欄名、同版本／session、固定容量／時間契約、行數、逐筆CRC、bits補數解碼、accepted／DATA後檢查、脈衝數、RX／ERR增量與跨列連續、序號／間隔、trigger定位及接受值大跳分類一致。任一矛盾退出非0並保存原文；不修資料或補尾行。多個不交錯完整DUMP分開回傳。

摘要的雙鄰算術右移k=1..23僅列數值候選，不能證實錯位、還原正確RAW或定位軟硬體根因。原生C++測試直接編譯公開`hx_read_diag.h`，驗證ring／計數／回繞／CRC；與Node回歸一樣，均不是實板GPIO時序或精度驗證。

交付時239項Node檢查（舊回歸215、ring12、匯出協定12）與10項原生C++檢查通過。正式Arduino編譯、目標寫入校驗、啟動識別、獨立Ampere QC及實際逐讀資料須另行留證；無事件只表示本窗口未再現，不能自動解除校正暫停或執行CAL366。
