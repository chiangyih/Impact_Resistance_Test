# hxfail2 同版授權重啟觀察（2026-10-28，第1輪）

本輪是使用者授權保持USB插接、重啟既有hxfail2並重新記錄，**沒有新韌體修改、編譯或燒錄，不是新的燒錄QC**。[原hxfail2部署及QC](2026-10-28-hxfail2-qc.md)作歷史追溯，本報告分開記本次reset、新boot及新診斷資料。主代理統一操作COM3及重啟；Ampere只讀公開證據、撰寫本報告，未開埠、重啟、讀秘密或私有build。

**結論：舊證據保存、單次授權重啟、新boot／版本／session及本輪雙槽到期封存／完整bundle匯出子項通過；目標LINK未捕獲，DATA實際取樣仍未觸發。** 初始化暫時OLED MISSING與DUMP前FIFO1→2、DUMP後2→3分列待解，不能判完整量測通過。新boot為UNCAL；計數重設或有限窗口無觸發不是修復證據，原量測異常及校正暫停不變。

## 版本與舊證據保存

已部署版本為`v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1-hxfail2`。本輪重算公開核心來源，與前版正式部署一致：

| 檔案 | SHA256 |
|---|---|
| `05-test/05-test.ino` | `06D3066BE9B64CBD012DC1C66B5AB9CF190D57D821FC1D647008092BCFC71139` |
| `05-test/hx_read_diag.h` | `A488BA5FA1149123B8CA55592E0048AED23642E486DD462E802453C875090864` |
| `05-test/hx_failure_diag.h` | `F210D64AC515A903354CC056CDD1D7670DFD44CCC217F5413A944241D872E818` |

這些來源hash不是本次新的flash映像校驗。前次Arduino CLI／core／FQBN及四區寫入成功只在歷史部署成立，不冒充本輪重做。

舊session `B17323B9`的[已封存原始bundle](../../calibration/20261028-hx711-hxdual-observe-112033.serial.log) SHA256仍為`3DFC48A4E883BAC56DC131AA2BD4EDD9E70717628A01539F009EB7FE847704A7`；[既有完整分析](../../calibration/20261028-hx711-hxdual-observe-112033.analysis-v2.json)仍保存。重啟會重新開始RAM狀態，沒有刪除或改寫舊公開原文；舊127個保留框不是舊600秒全框資料。

## 重啟前唯讀保存

[serial](../../calibration/20261028-hx711-before-authorized-restart-114509.serial.log)、[逐行JSONL](../../calibration/20261028-hx711-before-authorized-restart-114509.lines.jsonl)、[host](../../calibration/20261028-hx711-before-authorized-restart-114509.host.json)、[control](../../calibration/20261028-hx711-before-authorized-restart-114509.control.log)。

11:44:58.872～11:45:09.590（台北UTC+08），10.718秒；5746 bytes、33完整行，無尾片。SHA256 `7A67CFD566C0EC9FC2CA8B53335557D333CDA1E6A5115A51C6928EB9C947267D`與host一致；原文、完整行byte offset、行號、全部已解析欄位及UTC／單調順序獨立核對差異0。

12筆STATUS為IDLE／UNCAL／HX及ADXL OK、ERR0／AERR0／FIFO2，RX18890→18980。只送STATUS及HX_DIAG STATUS；舊session仍`B17323B9`，兩槽FROZEN／reason3／export_sealed1、records127，HXF CLOSED_NO_LINK／captured0。查詢列的是scope=live attempts18928、pre_overwritten5346、omitted13455，滿足18928=127+5346+13455；不能與舊DUMP的scope=frozen attempts5473混成計數錯誤。

OPEN前115200／8N1、DTR／RTS off由實際control DESCRIBE核對；CLOSE／QUIT OK、exit0、overflow=false／failure=null，主代理已結束此保存窗。不將本次查詢當重新凍結、重arm或再讀完整ring。

## 單次授權reset與完整性

[reset原文](../../calibration/20261028-hxdual-authorized-restart-1145.serial.log)、[host控制及chunk紀錄](../../calibration/20261028-hxdual-authorized-restart-1145.host.json)、[逐行映射JSONL](../../calibration/20261028-hxdual-authorized-restart-1145.lines.jsonl)。

主代理以.NET SerialPort，OPEN前115200／8N1、Handshake None、DTR=false／RTS=false；11:46:36.9032828開埠。重啟前先被動收到舊epoch三筆IDLE／UNCAL，RX19782→19791→19801，ERR0／AERR0／FIFO2，11:46:40.4472032 guard通過，沒有以靜默猜待機。

| host事件 | 台北主機時間／紀錄 |
|---|---|
| RTS assert | 11:46:40.4483617，rts=true、dtr=false，僅一次 |
| RTS release | 11:46:40.6629161，rts=false、dtr=false，僅一次 |
| assert到release | 目標等待200ms；單調時間實際214.5919ms、主機UTC差214.5544ms。是主機設定呼叫界線，不是實測管腳脈寬。 |
| CAPTURE_END | 11:46:55.6776769，7108 bytes |
| CLOSE | 11:46:55.6817139，host port_closed=true、failure=null |

RTS切換是本次已獲授權的reset動作，與一般唯讀窗不切換DTR／RTS分開；DTR在所記設定／assert／release均false。host事件沒有裝置文字命令TX；本輪未TARE／CAL／UNCAL／ARM／STOP／CLEAR或切WiFi。USB插接及保持供電是主代理操作條件，沒有獨立電源波形。

原始7108 bytes與host一致，SHA256 `C430147EB1A282EAAA61E2F7E95483B17E787EC4569EC4E83D3E01604EE58E98`；48個RX_CHUNK的offset／length從0連續涵蓋全部位元組。56個完整行、無尾片，與JSONL逐行原文／byte offset／行號／順序一致；每行完成時間均對應包含該行LF最後位元組的RX_CHUNK時刻，差異0。JSONL是接收chunk的主機時間映射，不是MCU boot事件、DATA點位或ADC時刻；同chunk多行時間相同不代表同時發生。

## 新boot與初始化觀察

| 證據 | 本輪可支持與限制 |
|---|---|
| Boot／reset訊息 | JSONL9於11:46:40.7165476收到`rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)`。如實保留裝置訊息；不由此判USB曾拔插、電源斷續或電氣根因。 |
| 同版識別 | JSONL22於11:46:41.1593293收到目標hxfail2自然banner及UNCAL／SCK25 DATA33等設定。本次有自然啟動證據，不能回填前次燒錄時缺失的banner。 |
| 新session／窗口 | JSONL26收到HX_DIAG_READY schema2／session `C7B7BD64`／slots2／window600000000／record_bytes56／rings_bytes28672／no_auto_cal1。與舊B17323B9、reset紀錄及RX重新開始共同識別新epoch；隨機session不是永久唯一ID。READY收行時刻不直接當裝置窗口起點。 |
| 初始HX | 先WAIT／RAW=NA／RX0／AGE0，後首筆OK為RX7／RAW−163090／AGE3。NA不填成0公克或有效新資料；後續至RX121持續OK、ERR0／AERR0。 |
| WiFi | 新boot先WAIT／radio0／IP NA，再CONNECTING及CONNECTED／radio1／IP192.168.52.71。此為啟動連線歷程，不誤列成既有連線故障或保證外網。 |
| OLED | 11:46:41.1280946先印MISSING/no ACK，初始OLED0x00；11:46:44.5348301印ACK0x3C，隨後STATUS地址0x3C。是暫時初始化未回應後重試成功，未定位原因；ACK／地址不代替現場目視。 |
| ADXL／FIFO | Boot印READY、ACK53／IDENTIFIED、RATE0x0A／FIFO0x9F及INIT PRE_N32/SRC83→POST_N0/SRC81；PRE在重設定前，不能判本MCU epoch新OVR。 |
| 新epoch第二FIFO警示 | 11:46:44.6771884 RX28／AGE627／FIFO1，11:46:45.6562370 RX36／AGE155／FIFO2，ERR仍0／AERR0。此時尚未DUMP，不能沿用前次匯出因果；沒有本輪ADXL_DIAG快取，原因／實際漏樣量未定位，也不與HX異常混因。 |
| DATA初態 | HX_FAIL為WAITING／captured0／valid0，沒有實際DATA點位。後續待完整雙槽及profile封存；未觸發不填LOW或已修復。 |

本次開機的RX／ERR／FIFO重新開始屬新epoch，不能從舊末值減新初值當錯誤改善。既有setup也會執行原HX初始化；USB插接保持不等於ADC／RAM狀態連續。歷史offset／scale不沿用，保持UNCAL。

## Session C7B7BD64完整雙槽bundle

[serial](../../calibration/20261028-hx711-hxdual-restart-observe-115650.serial.log)、[逐行JSONL](../../calibration/20261028-hx711-hxdual-restart-observe-115650.lines.jsonl)、[host](../../calibration/20261028-hx711-hxdual-restart-observe-115650.host.json)、[control](../../calibration/20261028-hx711-hxdual-restart-observe-115650.control.log)、[分析v2](../../calibration/20261028-hx711-hxdual-restart-observe-115650.analysis-v2.json)。

capture為11:47:27.034～11:56:49.868，實際562.834秒；主機晚於boot開監控，planned620000ms不是實際擷取時長或裝置窗口起點。216434 bytes、1417完整行＝559 STATUS＋559 WiFi＋254 HXRD＋12 HXFP＋33其他行，無尾片。SHA256 `9B8EEBA1F2E7E6BFB00C151A63ED40A0FD940DEA20A7DDC289826F3E0355E42E`與host一致；Ampere按原始位元組核對完整行、offset、行號、全部已解析欄位及UTC／單調順序，差異0。

只有HX_DIAG STATUS（11:47:30.537）及HX_DIAG DUMP（11:56:41.844）兩條唯讀TX。先於11:56:41.202收到schema2／slots2／all_frozen1通知，才送DUMP；外層BEGIN_HX_CAPTURE為11:56:41.898，完整END_HX_CAPTURE為11:56:44.484，接收跨度2.586秒。ANY／LINK／唯一HXF依序且最外END complete1、retained1、read_only1，未以單槽結束冒充整體匯出。

| 核對 | 本輪結果與界線 |
|---|---|
| 身分／政策 | 同版hxfail2／session `C7B7BD64`，ANY mask7、LINK mask1，與本次自然boot一致；不跨用舊B17323B9資料。 |
| 共同期限 | start_us88386、window600000000、freeze_us600123406，兩槽reason3 WINDOW、export_sealed1；跨度600.035020秒，目標晚35.020ms。是CPU服務期限，不是硬即時／管腳波形。 |
| 無trigger呈現 | 兩槽trigger_flags0、pre0／post0／post_complete0；無效trigger_seq／time零欄不當時刻0事件。 |
| 收支／保留範圍 | 各scope=frozen attempts5476=pre_overwritten5349+records127+omitted0，overflow0／seq_wrapped0。各保留seq5350～5476、127筆accepted／後檢1／ERR0／flags0，RX5349→5476；不是完整600秒原始字。 |
| 獨立CRC／同來源 | Ampere另以獨立CRC實作重算兩槽共254列19-word CRC、補數解碼、RX／ERR增量及seq／gap連續性，差異0；127個重疊seq的19words全同，254輸出列只127不同attempt。 |
| 裝置時間 | 保留首start586248420、末end600051610µs，跨度13.803190秒；read括弧187～222µs，126個跨列開始gap15623～237116µs。首列前一attempt在保留窗外；CPU括弧不驗SCK逐脈衝或ADC時刻。 |
| DATA | CLOSED_NO_LINK／mode4／stop_reason3／captured0，resume_valid0且metadata／resume為NA；12點valid0／missed0、DATA／before／end均NA。103-word CRC獨立重算`0x7E8831FE`一致；無hold，不判LOW、連續LOW或ADC ready。 |

v2工具從原serial重跑exit0，全部JSON物件與保存分析完全相同。本窗口依首觸發契約只能說到達hook的5476次完成attempt未觸發LINK／SAT／相鄰accepted差≥1000規則；未保留的5349字不能事後逐字驗真，也不涵蓋未ready poll、未完成attempt或全部ADC轉換。本次目標LINK未捕獲；真DATA及ANY先凍結再LINK等路徑仍未驗，不能解除既有Q05或據此CAL366。

## 初始化與DUMP後警示分開

本capture起始已為FIFO2，承接前述同epoch啟動期1→2；不是本窗DUMP才出現第二次警示。559個STATUS的RX422→5528皆不同、IDLE／UNCAL、ERR0／AERR0；RAW快照均值−163163.5868、範圍−163287～−163037 counts，屬約1Hz快照，非全部已接受字或精度驗收。

最外END後JSONL1406於11:56:44.622印HX STALE／AGE2739ms、ADXL MISSING、FIFO2；JSONL1408於11:56:45.625恢復HX／ADXL OK，FIFO變3並維持至末筆。同步DUMP停止輪詢是已知行為，此次匯出後警示與其觀測擾動相容，與啟動期已出現的1→2分開；沒有本輪ADXL_DIAG定位來源，不推實際漏樣量或與HX根因同因，也不回因改寫已封存資料。

OPEN前115200／8N1、DTR／RTS off與實際control DESCRIBE一致；CLOSE／QUIT OK、process exit0、stderr0、overflow=false／failure=null，第一輪bundle capture已關閉並釋放COM3。工具ACK不是ESP32 ACK。

## 待追蹤及後續資料

| ID／狀態 | 證據／界線 | 負責人與條件 |
|---|---|---|
| HXF2-R1-Q01／P2／初始化及匯出警示待解 | 暫時OLED MISSING後ACK恢復；啟動期DUMP前FIFO1→2，DUMP後STALE／MISSING及FIFO2→3。 | Gibbs＋主代理：分開初始化／顯示重試與匯出時段，需快取或適當證據才能縮小原因，不清警示／盲改或推已知漏樣量。 |
| HXF2-R1-Q02／P2／部分通過、DATA未觸發 | 雙槽600秒到期／CRC／重疊唯一計數與完整bundle通過；HXF captured0、真LINK與DATA點位未觸發。 | Ampere／Gibbs：本輪不能填真LINK／DATA通過；後續有界觀察分新session記錄，不用舊QC替代，也不以無觸發視為原因已解。 |
| HXF2-R1-Q03／P3／目視及未觸發路徑未測 | OLED／RGB實際畫面無同輪證據；沒有DATA實際點位、busy／truncated等。 | 主代理／Kierkegaard：有具體需要及授權才補驗，不由ACK或靜默假定通過。 |
| HXF2-R1-Q04／P1／量測用途未驗收 | 舊−80／近零／巨大字及漂移原因未解，新boot UNCAL；本輪重啟不是修復或校正。 | Gibbs＋Kierkegaard＋主代理：待原因及必要修復驗證後，才做近時新TARE／已知366g CAL及載卸／另一點；不沿舊RAM、不用無觸發窗解除暫停。 |

本報告三段已完成capture（重啟前保存、.NET reset／boot、第一輪bundle）均已由主代理關閉；Ampere未持有埠。本輪沒有新燒錄，只新增本報告，未修改工程檔／原始資料或commit／push。主代理另安排的第二次有界重啟屬新session，須另記觀察，不與C7B7BD64合併計數或宣稱本輪所有後續操作均已結束。
