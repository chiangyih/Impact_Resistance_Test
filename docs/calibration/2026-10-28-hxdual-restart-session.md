# 2026-10-28 HX711雙槽診斷授權重啟session

日期／時區：2026-10-28，Asia/Taipei（UTC+08:00）。主代理為唯一COM3操作者；Planck只核對公開證據及整理此文件。

**兩次授權重啟及各自的雙槽WINDOW到期bundle均已保存，COM3已釋放。** 兩輪`C7B7BD64`與`99C7AFD6`各有自然boot，不能合併成連續無故障紀錄。每輪兩槽各127列完全重疊、DATA12點未取樣NA，均未hold；第二輪5487封存計數包含1筆越窗未分類，規則未觸發範圍僅支持seq1～5486。兩輪未捕獲真LINK／DATA，原始資料品質與根因未解，不以無觸發結案或校正。USB供電保持、仍UNCAL，366g校正未完成；未新flash、commit／push。

## 授權、舊資料與epoch界線

本次是同一已部署hxfail2韌體的授權重啟，未另編譯／燒錄。部署來源、寫入校驗、舊session `B17323B9`的期限封存及量測限制見[前一session](2026-10-28-hx711-post-failure-data-diagnostic-session.md)及[hxfail2獨立QC](../quality/reports/2026-10-28-hxfail2-qc.md)。舊bundle已匯出保存，本次不覆寫或以新boot補造舊epoch資料。

USB維持供電是本次操作條件，不代表執行中RAM或計數跨重啟保留。新舊session、RX／ERR／FIFO與診斷序號分開，不跨epoch計算增量、不套用舊offset／scale，也不把新計數歸零當作舊−80／−317或其他資料異常已修復。既有366g校正尚未完成，本次未TARE／CAL或作載卸試驗。

## 重啟前已保存狀態

- [serial原文](20261028-hx711-before-authorized-restart-114509.serial.log)
- [完整行JSONL](20261028-hx711-before-authorized-restart-114509.lines.jsonl)
- [host設定、命令與收尾](20261028-hx711-before-authorized-restart-114509.host.json)
- [monitor控制原文](20261028-hx711-before-authorized-restart-114509.control.log)

serial-monitor v0.15.0、OPEN前115200／8N1、DTR／RTS off。11:44:58.872 OPEN ACK後，只送11:45:02.383 STATUS、11:45:03.486 HX_DIAG STATUS兩條唯讀命令，capture於11:45:09.590結束。12筆STATUS收行11:44:59.419～11:45:09.548，RX18890→18980，IDLE／UNCAL／HX及ADXL OK，ERR0／AERR0／FIFO2。

兩槽讀回hxfail2／session `B17323B9`、FROZEN／freeze_reason3、records127、export_sealed1；承接已保存的[舊完整bundle](20261028-hx711-hxdual-observe-112033.serial.log)，沒有重新取得新事件。本窗CLOSE／QUIT成功、process exit0、overflow=false、failure=null，該連線已釋放。

## 單次RTS操作與自然boot

- [重啟serial原文](20261028-hxdual-authorized-restart-1145.serial.log)
- [逐行接收時間JSONL](20261028-hxdual-authorized-restart-1145.lines.jsonl)
- [SerialPort設定、RTS事件與關埠host](20261028-hxdual-authorized-restart-1145.host.json)

主代理以.NET SerialPort開COM3，115200／8N1、handshake none，開埠前DTR=false／RTS=false；未拔插USB。取得至少兩筆IDLE／UNCAL及RX前進的守衛條件後才重啟；實際檔內保存3筆舊epoch狀態，RX19782→19791→19801，11:46:40.4472032記錄GUARD_PASS。

| 已發生操作／原文 | 主機記錄時間（台北）與界線 |
|---|---|
| SerialPort OPEN | 11:46:36.9032828 |
| 單次RTS assert／DTR維持false | 11:46:40.4483617 |
| RTS release／DTR維持false | 11:46:40.6629161；目標200ms，主機單調時刻差214.5919ms，為呼叫間隔而非獨立量到的RTS電壓脈衝 |
| ROM自然輸出 | 11:46:40.7165476收到`rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)` |
| 自然應用版本 | 11:46:41.1593293收到`v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1-hxfail2`、UNCAL |
| HX_DIAG_READY | 11:46:41.2222504讀回schema2、session `C7B7BD64`、slots2、window_us600000000、rings_bytes28672 |
| 初始HXF | 同一接收時間為WAITING／captured0／next0／valid0／missed0，未取樣 |
| 本次重啟擷取結束／SerialPort CLOSE | 11:46:55.6776769／11:46:55.6817139，host port_closed=true、failure=null |

ROM的POWERON_RESET如實保存為裝置輸出，**不能單憑該字串推定USB曾拔除、供電曾中斷或電壓下陷**；本次主機明確操作是一次RTS assert／release並意圖保持USB供電。沒有獨立供電／RTS波形量測，兩類證據不可互代。

此檔含重啟前3筆STATUS與新boot後14筆STATUS，不能合併成單一RX序列。新boot首筆HX WAIT／RAW NA／RX0，不將NA改成0；其後13筆有RAW，HX OK、RX7→121，ERR0／AERR0。自然boot僅補足本次啟動證據，不能回填前兩次燒錄後錯過的boot畫面。

## boot期間FIFO與外設觀察

新boot的FIFO_INIT保存PRE_N32／PRE_SRC0x83→POST_N0／POST_SRC0x81，明示snapshots_not_simultaneous。初始化前硬體來源／深度不能單獨當作新MCU epoch產生的實際overrun。

11:46:44.6771884的STATUS RX28仍FIFO1，11:46:45.6562370 RX36已FIFO2，之後至此檔末筆維持2。**此增加發生在新epoch尚未DUMP之前，不可歸因DUMP同步匯出。** 沒有對應逐事件ADXL快取／GPIO或電氣量測，增加的具體時間、根因及漏樣量仍未知，也不指定與HX異常同因。

boot先於11:46:41.1280946印OLED MISSING，後於11:46:44.5348301印OLED ACK=0x3C；ACK不代替現場OLED內容／RGB目視。WiFi由WAIT／CONNECTING進入CONNECTED，僅是該短窗串列狀態，不代表外網或量測時序驗證。未使用先前OLED數值。

## 完整性與保存識別

Planck依原始UTF-8位元組核對兩份serial與JSONL的完整行、行號及byte offset，差異均0。重啟前5746 bytes／33完整行，重啟7108 bytes／56完整行，兩者無未完成尾片；各serial SHA均與host保存值一致。重啟JSONL未提供預先解析fields，內容依原文判讀，不冒稱已核對不存在的欄位。

| 公開檔 | SHA256 |
|---|---|
| before-authorized-restart serial.log | `7A67CFD566C0EC9FC2CA8B53335557D333CDA1E6A5115A51C6928EB9C947267D` |
| before-authorized-restart lines.jsonl | `A0FCFC63F73B82657F8C3432667AD855047721C8521ED78D0E4C43280A7CF4FC` |
| before-authorized-restart host.json | `5CC31CCA628736937598D3EF7A63A1EE9C0AC55A90DF2B8141DA7EEF09CFAB28` |
| before-authorized-restart control.log | `20C89E60163C5035CC067814700B842114C0904728EDC0C73935360346B806A5` |
| authorized-restart serial.log | `C430147EB1A282EAAA61E2F7E95483B17E787EC4569EC4E83D3E01604EE58E98` |
| authorized-restart lines.jsonl | `C403702E87077DC90A5790EC626674BF52955BDE9860221329C8A7E9AE81B750` |
| authorized-restart host.json | `1A3B69808DB2C6E2A974747AB966FC6C364C3E35D7ED08C3BD51E150E4427A10` |

主機完整行接收時間不是原始讀框／ADC轉換時刻；SerialPort的RX_CHUNK時間也不是逐bit時間。Ampere已另建[同版重啟觀察QC](../quality/reports/2026-10-28-hxfail2-restart1-qc.md)，它不是新燒錄QC。此第一輪boot之後取得的bundle另列如下，不回填成boot當時已封存。

重啟前monitor窗及第一輪RTS擷取各已關埠；第一輪後續觀察與第二輪重啟見下節。保持供電／UNCAL，舊量測問題及366g校正待解。Planck僅整理本文件，未操作COM、讀秘密、修改handoff／韌體／QC或commit／push。

## 第一輪C7B7BD64：雙槽到期bundle已保存

- [bundle serial原文](20261028-hx711-hxdual-restart-observe-115650.serial.log)
- [完整行JSONL](20261028-hx711-hxdual-restart-observe-115650.lines.jsonl)
- [host操作與收尾](20261028-hx711-hxdual-restart-observe-115650.host.json)
- [完整analysis-v2](20261028-hx711-hxdual-restart-observe-115650.analysis-v2.json)

本窗OPEN ACK為11:47:27.034，capture結束11:56:49.868；只送11:47:30.537 HX_DIAG STATUS及11:56:41.844 HX_DIAG DUMP。11:56:41.202已先收到同session兩槽all_frozen1／reason3通知，DUMP沒有強制封存。外層BEGIN／END_HX_CAPTURE收行11:56:41.898～11:56:44.484，完整跨度2.586秒。

| 項目 | 已取得的有限結果 |
|---|---|
| epoch／期限 | hxfail2／`C7B7BD64`；兩槽共同start_us88386、目標600000000µs、freeze_us600123406，實際600.035020秒，為需服務的目標期限而非硬即時時鐘。 |
| ANY／LINK | 均WINDOW／reason3、trigger_flags0；各127列seq5350～5476，accepted127、rejected0、overflow0。沒有事件127前＋128後的實板路徑結果。 |
| 封存計數／重疊 | 各5476＝pre_overwritten5349＋保留127＋omitted0。254個HXRD輸出列只有127個不同attempt；Planck比較原始整列與解析器逐word重疊結果一致。 |
| 實際保留範圍 | 首start586248420～末end600051610 boot µs；其餘已覆寫資料沒有完整原字可供離線驗真。 |
| HXF | CLOSED_NO_LINK／captured0、12個未取樣NA、valid0／missed0；metadata／resume NA，103-word CRC `0x7E8831FE`通過。不是12個LOW或逾點。 |
| 實際介入 | `low_hold_intervention_applied=false`、`changed_next_attempt_timing=false`，本輪沒有250ms hold；真LINK後DATA路徑仍未測。 |

按已部署觸發契約與摘要，只能說本輪5476次完成hook未觸發LINK／SAT／相鄰accepted差≥1000 counts；不是全部ADC conversion、完整窗口的原字獨立驗真或量測品質已修復。Planck從原serial重跑v2.0.0工具exit0，全部JSON與保存analysis-v2一致，兩槽CRC與完整bundle契約通過；CRC不等於ADC真值。前次−80／−317與其他資料異常、真LINK及先ANY後LINK路徑未解，不以本輪無觸發結案或直接校正。

本窗559個STATUS，收行11:47:27.799～11:56:49.639、RX422→5528，均IDLE／UNCAL／ERR0／AERR0。與本文件第一輪boot的DUMP前FIFO1→2分開，這次同步匯出後另於11:56:44.622出現HX STALE／AGE2739ms、ADXL MISSING、FIFO2；11:56:45.625近期OK並FIFO升3，後續維持3。同步DUMP阻塞是觀察介入，不能用它解釋先前boot期間的FIFO增加，亦未獨立定位每次來源或漏樣量。

serial216434 bytes、1417完整行、無尾片；原文／行號／UTF-8 offset與JSONL差異0，serial SHA與host一致。此monitor窗CLOSE／QUIT成功、process exit0、failure=null，該連線已釋放。

| 第一輪bundle公開檔 | SHA256 |
|---|---|
| serial.log | `9B8EEBA1F2E7E6BFB00C151A63ED40A0FD940DEA20A7DDC289826F3E0355E42E` |
| lines.jsonl | `2D3BAB6D1A6C9C4A598F87F8EE1A02902E2D93C549FD1AFD7D0E63C18ED548BC` |
| host.json | `EE2826C5401FB9228C1090D94462A9E3EA767F763885115A1936A7D75BEC91B9` |
| analysis-v2.json | `79798E2C9E75D5ADC6E09FE7D74F61F598DBA40D8CC3E85B606D006D46488592` |

## 只再一次有界重啟：第二輪99C7AFD6自然boot

主代理已向使用者告知，為增加自然捕獲機會，本輪只再進行一次有界重啟，不作無限重試。USB未拔插，同一hxfail2程式，沒有重新燒錄、TARE／CAL或載卸動作。

- [第二次重啟serial](20261028-hxdual-authorized-restart-2.serial.log)
- [逐行接收JSONL](20261028-hxdual-authorized-restart-2.lines.jsonl)
- [RTS、守衛與收尾host](20261028-hxdual-authorized-restart-2.host.json)

11:57:58.2562381以115200／8N1、handshake none、DTR／RTS false開COM3，守衛實際保存3筆舊`C7B7BD64`狀態，IDLE／UNCAL且RX6161→6178。11:58:01.8108433 GUARD_PASS後，只有一次RTS assert（11:58:01.8120329）／release（11:58:02.0198772），DTR維持false；目標200ms、主機單調呼叫間隔207.8379ms，不是獨立RTS管腳波形。

11:58:02.0495046自然ROM再印`rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)`；同樣不能由此推定USB曾斷電。11:58:02.9364325自然應用banner讀回hxfail2及UNCAL；11:58:03.1260546 HX_DIAG_READY讀回schema2、slots2、window_us600000000、新session **`99C7AFD6`**，HXF WAITING／captured0。

此檔18個STATUS含重啟前3筆及新boot後15筆，不當單一RX序列；新boot由RX1到123、IDLE／UNCAL／HX及ADXL OK、ERR0／AERR0／FIFO1，僅描述這段短窗。7378 bytes／57完整行、無尾片，原文／行號／byte offset與JSONL差異0，serial SHA與host一致。

| 第二次重啟公開檔 | SHA256 |
|---|---|
| serial.log | `9707DDBFF5139493E88048B9A6B87BC53FAD31062EC14D9649094CE6A4CCA5BA` |
| lines.jsonl | `DC877279F3706CDDED17A4AAE9F993D299B280F9F6F88AA890BDCDB4615055CC` |
| host.json | `E05B78D8F39B10DF74D8623FDD03B30515A80A2BA021714EABDEC6E6D9BD7E96` |

第二次RTS擷取於11:58:17.0300463結束、11:58:17.0395564 CLOSE，host port_closed=true／failure=null。主代理其後另開有界連線收集`99C7AFD6`，現已取得下節bundle；不把boot時的WAITING回填成當時已封存。

## 第二輪99C7AFD6：完整bundle與截止邊界

- [第二輪bundle serial](20261028-hx711-hxdual-restart-observe-120811.serial.log)
- [完整行JSONL](20261028-hx711-hxdual-restart-observe-120811.lines.jsonl)
- [host設定、命令及收尾](20261028-hx711-hxdual-restart-observe-120811.host.json)
- [完整analysis-v2](20261028-hx711-hxdual-restart-observe-120811.analysis-v2.json)
- [Ampere第二輪重啟觀察QC](../quality/reports/2026-10-28-hxfail2-restart2-qc.md)

本窗OPEN ACK為11:58:42.866，capture結束12:08:11.706；只送11:58:46.371 HX_DIAG STATUS及12:08:03.690 HX_DIAG DUMP。12:08:02.981先收到同session兩槽all_frozen1／reason3通知，外層BEGIN／END_HX_CAPTURE收行12:08:03.743～12:08:06.329，完整跨度2.586秒。

| 封存項目 | 核對結果與限制 |
|---|---|
| 版本／epoch | 同一hxfail2、session `99C7AFD6`，與第一輪`C7B7BD64`分開，不跨boot加總RX／ERR或當連續量測。 |
| 共同期限 | start_us516300、目標600000000µs、freeze_us600539418，實際600.023118秒，比目標晚23.118ms；CPU服務時間不是GPIO或ADC時鐘。 |
| ANY／LINK | 兩槽WINDOW／reason3、trigger_flags0，沒有有效trigger時刻；各127列seq5360～5486、accepted127／rejected0／overflow0。 |
| 計數邊界 | frozen attempts5487＝5359 overwritten＋127 retained＋1 omitted；不能把5487全部列入觸發判斷涵蓋數。 |
| 重疊／完整性 | 兩槽254輸出列僅127個不同attempt，canonical逐word重疊一致、兩槽CRC與完整bundle契約通過；CRC不證ADC真值。 |
| HXF | CLOSED_NO_LINK／captured0，12個未取樣NA、valid0／missed0，metadata／resume NA，103-word CRC `0x7E8831FE`通過；未hold。 |

**第5487次完成hook不在本輪事件判斷範圍。** 公開`hxdiagObserveShared`先更新attempts，再以`record.start_us`執行Tick；若已FROZEN即omitted++並返回，之後才分類LINK／SAT／JUMP。seq5486的start600509475／end600509675µs在deadline600516300前，末end早6.625ms；封存於600539418µs並含omitted1，支持第5487次在開始括弧已越窗、未進flags分類。它的原字／end／flags未保存，不能補造或稱正常。

依已部署契約與封存metadata，本輪可支持**seq1～5486的窗內完成attempt未觸發既定規則**；JUMP首個accepted無先前比較，後續相鄰accepted比較最多5485次。這不是5487筆全部無事件，也不是5486個字經離線獨立驗真；5359筆已覆寫、原始只留最後127筆，不涵蓋未ready poll、未完成attempt或全部ADC conversion。不能將兩輪無觸發拼成可靠度合格或解除舊−80／−317／漂移。

Planck從原serial重跑v2.0.0解析器exit0，完整JSON與保存analysis-v2全物件一致，實際hold／timing介入均false。DATA採點、真LINK及先ANY後LINK等事件路徑仍未觸發，不能以到期匯出通過代替。

## 第二輪STATUS、DUMP干擾與完整性

565 STATUS／565不同RX377→5539，完整收行11:58:43.548～12:08:11.486；RAW平均−163222.9150、範圍−163332～−163085 counts，564對相鄰快照最大絕對差173 counts。約1Hz快照及主機UTC不是每個讀字或ADC時刻；無大跳不證原字正常或靜態精度合格。

全窗IDLE／UNCAL、ERR0／AERR0。DUMP前FIFO1；外層END之後12:08:06.468首次印HX STALE／AGE2739ms、ADXL MISSING／FIFO1，12:08:07.469近期OK並FIFO升2，之後至末筆持平。同步DUMP阻塞輪詢是已知介入，這些後續現象不能回因或改寫封存前資料；未另取ADXL_DIAG定位第二警示來源，不推實際漏樣量或與HX同因。它也不能解釋第一輪boot期間、DUMP前的FIFO1→2。

serial218411 bytes、1429完整行、無尾片；Planck逐列核對原文、行號、UTF-8 byte offset及STATUS已解析欄位，差異0，serial SHA與host一致。沒有將缺值／NA補0，或把兩次重啟前後的STATUS當單一RX序列。

| 第二輪bundle公開檔 | SHA256 |
|---|---|
| serial.log | `3B046C8007E3A5F4AAED6ED12D310EFFCC0E62F3AED6D25327A74A867959FE4A` |
| lines.jsonl | `624816EA9D3EBF3611F1EDA71E7453929EAA5283BFBDB22284764CDBFD47C8E5` |
| host.json | `9351EF884BB09222EFD7C7D0DE6517D2F85E4B34DD49D4EE8B52C69FE0ACE845` |
| control.log | `20C89E60163C5035CC067814700B842114C0904728EDC0C73935360346B806A5` |
| analysis-v2.json | `FB6DA6EABC64E5052B772FC15B9139908A041DBFE77464F78E1E9A08298D16A5` |

## 兩輪收尾與待解項

兩輪各由使用者授權的USB不拔／單次RTS assert-release啟動，開埠前DTR／RTS false，IDLE／UNCAL及RX前進guard成立後才操作；各有自己的自然banner及session，未新燒錄。ROM POWERON_RESET仍只作裝置原文，不用來判USB供電曾中斷。

第一輪[同版重啟QC](../quality/reports/2026-10-28-hxfail2-restart1-qc.md)及第二輪QC分別追溯；兩輪到期保存／CRC／匯出有限結果不等於量測或DATA路徑通過。真LINK／DATA未捕獲，舊已接受異常字、FIFO來源及根因仍未解，366g校正未完成、保持UNCAL，不以無觸發直接TARE／CAL或無限重啟。

第二輪觀察CLOSE／QUIT成功、process exit0、overflow=false、failure=null、主機確認埠已釋放；兩輪各RTS擷取及觀察均已收尾，**目前COM3已釋放，USB供電保持**。未TARE／CAL、新flash、commit／push。Planck此次僅更新本文件，未開COM、讀秘密或改handoff／韌體／QC。
