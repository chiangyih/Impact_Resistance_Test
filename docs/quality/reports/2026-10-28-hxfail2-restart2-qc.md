# hxfail2 同版額外有界重啟觀察（2026-10-28，第2輪）

本輪由主代理依使用者自動排查授權，在[第一輪C7B7BD64證據保存](2026-10-28-hxfail2-restart1-qc.md)後，再做一次保持USB插接的有界重啟觀察。沒有新韌體修改、編譯或燒錄，**不是新的燒錄QC**；歷史部署仍見[hxfail2 QC](2026-10-28-hxfail2-qc.md)。Ampere只讀公開證據並撰寫此獨立報告，未開COM、重啟、讀秘密或build。

**結論：第二次單次授權reset、新boot／同版識別／session `99C7AFD6`、雙槽到期封存及完整bundle匯出子項通過；第二輪仍未捕獲LINK，DATA實際取樣未觸發。** 封存attempts5487含1筆越窗未分類attempt，不能稱5487筆全部受事件規則涵蓋。FIFO初始化警示及匯出後STALE／MISSING／新增FIFO警示保留；不以有限窗口未觸發解除舊量測異常。保持UNCAL與校正暫停，不跨session合計或沿用RAM係數。

## 舊證據及本輪控制

前輪[已封存bundle](../../calibration/20261028-hx711-hxdual-restart-observe-115650.serial.log)已保存，SHA256 `9B8EEBA1F2E7E6BFB00C151A63ED40A0FD940DEA20A7DDC289826F3E0355E42E`，其CRC／完整性見第一輪報告；本次重啟不覆寫公開原文或把前輪結論轉成新輪驗證。

本輪[serial](../../calibration/20261028-hxdual-authorized-restart-2.serial.log)、[host](../../calibration/20261028-hxdual-authorized-restart-2.host.json)、[逐行映射JSONL](../../calibration/20261028-hxdual-authorized-restart-2.lines.jsonl)。主代理.NET SerialPort在OPEN前設定COM3／115200／8N1／Handshake None，DTR=false／RTS=false。

| host事件 | 台北主機時間／核對 |
|---|---|
| OPEN | 11:57:58.2562381 |
| 前置guard | 11:58:01.8108433通過；先收到三筆IDLE／UNCAL／HX及ADXL OK，RX6161→6169→6178、ERR0／AERR0／FIFO3。舊epoch依已保存bundle及操作追溯，未在這三筆STATUS另查session。 |
| RTS assert／release | 11:58:01.8120329／11:58:02.0198772，各一次，DTR均false；單調間隔207.8379ms，是主機設定界線，不是實測管腳脈寬。 |
| CAPTURE_END／CLOSE | 11:58:17.0300463／11:58:17.0395564；host port_closed=true、failure=null。 |

RTS切換只記本次已授權reset；host事件沒有裝置文字命令TX，不冒稱一般唯讀窗完全沒有控制線變化。USB保持插接是主代理操作條件，沒有獨立供電波形；裝置`POWERON_RESET`字樣不證USB曾斷電。沒有TARE／CAL／UNCAL／ARM／STOP／CLEAR／WiFi切換或新flash。

## 原文、chunk及時間映射

7378 bytes與host一致，SHA256 `9707DDBFF5139493E88048B9A6B87BC53FAD31062EC14D9649094CE6A4CCA5BA`。57個RX_CHUNK的offset／length從0連續涵蓋全部位元組；57完整行、無尾片。

Ampere獨立逐行核對raw、byte offset、行號，並以每行LF最後位元組所在chunk比對JSONL的完整主機UTC及單調時間，差異0；chunk順序亦一致。JSONL沒有假設ADC取樣時刻；同一chunk的多行可以有相同接收時間，不能當同步物理事件。

## 本次自然boot

- JSONL9於11:58:02.0495046收到`rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)`，保留原訊息，不推電氣根因或供電斷續。
- JSONL22於11:58:02.9364325收到自然banner：`v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1-hxfail2`及UNCAL／SCK25 DATA33。runtime版本仍是既有hxfail2，不是新部署，也不回填前次燒錄時漏失的banner。
- JSONL26於11:58:03.1260546收到HX_DIAG_READY schema2／session `99C7AFD6`／slots2／window600000000／record_bytes56／rings_bytes28672／no_auto_cal1。新session配合reset、banner及計數重新開始識別本epoch，不當永久唯一ID；READY收行時刻不是裝置窗口精確起點。
- 本次OLED在啟動即ACK0x3C，沒有前輪該段的MISSING文字；地址／ACK仍非目視畫面或RGB實際熄燈驗證，不由差異判前輪問題已修。
- ADXL啟動READY／ACK53／IDENTIFIED、RATE0x0A／FIFO0x9F，INIT PRE_N32／SRC83→POST_N0／SRC81；PRE是在重設定前取得，不能視為本MCU epoch新OVR。新boot首STATUS即HX OK、RX1／RAW−163252／AGE131，後續至RX123持續HX／ADXL OK、ERR0／AERR0／FIFO1；不把新計數低值當故障消失。
- WiFi由WAIT／radio0／IP NA經CONNECTING到CONNECTED／radio1／IP192.168.52.71。是啟動狀態歷程，不證外網或射頻／供電時序。
- HXF初態WAITING／captured0／next0／valid0／missed0，沒有實際DATA點位；不能填LOW／連續LOW或ADC ready。

新boot會重建RAM狀態與原HX初始化，不因USB插接保持而沿用先前offset／scale、RX／ERR或FIFO；本輪依然UNCAL。

## Session 99C7AFD6完整bundle與期限界線

[serial](../../calibration/20261028-hx711-hxdual-restart-observe-120811.serial.log)、[逐行JSONL](../../calibration/20261028-hx711-hxdual-restart-observe-120811.lines.jsonl)、[host](../../calibration/20261028-hx711-hxdual-restart-observe-120811.host.json)、[control](../../calibration/20261028-hx711-hxdual-restart-observe-120811.control.log)、[完整分析v2](../../calibration/20261028-hx711-hxdual-restart-observe-120811.analysis-v2.json)。

capture為11:58:42.866～12:08:11.706，實際568.840秒；主機起點晚於setup，planned620000ms不是實際擷取時長或裝置600秒窗口起點。218411 bytes、1429完整行＝565 STATUS＋565 WiFi＋254 HXRD＋12 HXFP＋33其他行，無尾片。SHA256 `3B046C8007E3A5F4AAED6ED12D310EFFCC0E62F3AED6D25327A74A867959FE4A`與host一致；Ampere核對原文、完整行offset、行號、全部已解析欄位及UTC／單調順序，差異0。

只送HX_DIAG STATUS（11:58:46.371）及HX_DIAG DUMP（12:08:03.690）。12:08:02.981先收到schema2／slots2／all_frozen1，才匯出；最外BEGIN_HX_CAPTURE為12:08:03.743，完整END_HX_CAPTURE為12:08:06.329，接收跨度2.586秒。ANY、LINK各一schema2區塊與唯一schema1 HXF順序正確、最外END complete1／retained1／read_only1；不以單槽或HXF結束代替完整bundle。

| 核對 | 結果／界線 |
|---|---|
| 身分／政策 | 同版hxfail2、session `99C7AFD6`，ANY mask7／LINK mask1，與本次自然boot一致，不以C7B7BD64的檢核替代。 |
| 共同期限 | 兩槽start_us516300、window600000000，名義deadline600516300；freeze_us600539418，實際CPU差600.023118秒、晚23.118ms，reason3 WINDOW、export_sealed1。CPU freeze時間可取attempt開始括弧，不是管腳實體凍結瞬間。 |
| 封存收支 | scope=frozen attempts5487=pre_overwritten5359+records127+omitted_after_freeze1，overflow0／seq_wrapped0；不能把omitted當overflow、ADC漏樣量或已保存框。 |
| 保留原文 | 各seq5360～5486共127筆，均accepted1／後檢1／flags0／ERR0，RX5359→5486；首start586734418、末end600509675µs，跨度13.775257秒。 |
| 獨立CRC及重疊 | 兩槽共254列19-word CRC、補數解碼、RX／ERR增量、序號／gap連續性獨立核對差異0；127個重疊seq逐word相同，254輸出列只127個不同attempt。 |
| CPU時間 | 兩槽read括弧187～207µs；126個可跨列開始gap15947～240081µs。首列前一attempt未保留；數值不是ADC時刻或SCK每個high／low／有效邊緣。 |
| HXF | CLOSED_NO_LINK／mode4／stop_reason3／captured0，metadata／resume為NA且resume_valid0；12點valid0／missed0、DATA及實測時間均NA。103-word CRC獨立重算`0x7E8831FE`一致；本輪無實際LOW hold，不判連續LOW／ADC ready。 |

**omitted1在總窗外，不能列入「規則未觸發」的涵蓋筆數。** Ampere重新核對公開header（SHA256仍為`A488BA5FA1149123B8CA55592E0048AED23642E486DD462E802453C875090864`）：`hxdiagObserveShared`先把完成seq記入attempts，再以record.start執行Tick；若FROZEN則omitted++並立即return，LINK／SAT／JUMP的flags分類在這之後。本次兩槽以5487封存、omitted1，最後保存seq5486的start600509475／end600509675均早於deadline，末end早6.625ms；依已部署程式路徑與封存收支，seq5487是在開始括弧600539418µs越窗而被略過，未進flags分類。這是來源契約與metadata推得的期限路徑，5487的原字／end／flags沒有保留，不能事後補造或宣稱該字正常。

因此本輪最多支持**seq1～5486的窗內完成attempt未觸發既定LINK／SAT／JUMP規則**；JUMP首筆沒有先前accepted比較，後續相鄰accepted比較為5485次。它不是5487筆全部無事件，也不是5486個字已獨立驗真：原始只保存最後127筆，其餘5359筆不能離線逐字驗證，亦不含未ready poll、未完成attempt或全部ADC轉換。兩輪session不能合併成連續無故障紀錄。

v2公開分析器從本輪serial重跑exit0，全部JSON物件與保存analysis-v2一致；CRC／解析通過僅支持資料形成後的匯出完整性，不證ADC真值。目標LINK與實際DATA點位仍未取得，ANY先凍結再LINK／busy／truncated等也不能由這次無事件窗口填實板通過。

## DUMP後警示

565個STATUS為不同RX377→5539，均IDLE／UNCAL、ERR0／AERR0；RAW快照範圍−163332～−163085 counts。約1Hz快照不是所有原始字，也不依此判靜態精度。此epoch在DUMP前FIFO1，與第一輪啟動期已增至2的情境分開，沒有跨epoch減計警示。

最外END後JSONL1418於12:08:06.468印HX STALE／AGE2739ms、ADXL MISSING／FIFO1；JSONL1420於12:08:07.469恢復HX／ADXL OK，FIFO變2，之後至末筆維持2。同步DUMP阻塞輪詢是已知行為，這些觀察與匯出擾動相容，不能回因或改寫此前已封存資料。本輪未取得新ADXL_DIAG定位第二警示來源，不推實際漏樣量或與HX同因。

本觀察OPEN前115200／8N1、DTR／RTS off與實際control DESCRIBE吻合；CLOSE／QUIT均OK，process exit0、stderr0、overflow=false／failure=null，主代理已關閉並釋放本次COM3。monitor ACK不是ESP32 ACK。

## 待補與用途限制

| ID／狀態 | 本輪界線及條件 |
|---|---|
| HXF2-R2-Q01／P2／部分通過、DATA未觸發 | 雙槽同期限到期、完整bundle／19-word及103-word CRC、重疊唯一127筆核對通過；5487含1筆越窗未分類，觸發規則涵蓋最多5486。真LINK／DATA及分離事件路徑仍未觸發。 |
| HXF2-R2-Q02／P2／初始化及匯出警示保留 | 初始化前既有OVR來源被觀察；DUMP後STALE／MISSING及FIFO1→2。保留未解，不以正常短窗／與前輪不同清警示或推已知漏樣量，與HX根因分開。 |
| HXF2-R2-Q03／P3／未測 | OLED／RGB目視及真DATA、忙碌／截斷／分離觸發等實板路徑尚無新證據。 |
| HXF2-R2-Q04／P1／量測用途未驗收 | 舊近零／巨大接受字及漂移原因未解，未建立本輪校正／回零／重放／另一重量點及動態驗證。不得以本輪無觸發直接CAL366或修RAW。 |

本報告reset／boot及完整bundle兩段capture均已由主代理關閉；Ampere未持有埠。沒有新flash或韌體修改；僅新增本次獨立觀察報告，未改原始資料、未commit／push。第二輪未捕獲DATA、舊近零／巨大字及漂移原因仍未解，維持UNCAL與校正暫停；不擅自增加重啟／重試。
