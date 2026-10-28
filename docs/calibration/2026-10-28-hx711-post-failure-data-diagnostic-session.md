# 2026-10-28 HX711失敗後DATA狀態診斷session

日期／時區：2026-10-28，Asia/Taipei（UTC+08:00）。主代理獨占COM3操作；Gibbs設計韌體，Ampere負責成功燒錄後獨立QC，Kierkegaard核對物理與時序限制，Planck整理公開證據。

**hxfail2 session `B17323B9`已完成共同600秒目標的雙槽到期封存及完整bundle匯出，部署與限定封存／匯出QC子項通過。** ANY／LINK各127列完全重疊，只有127個不同attempt；本窗無觸發，DATA為CLOSED_NO_LINK／12個未取樣NA，未執行250ms hold。真LINK的DATA路徑及先ANY後LINK情境仍未測，量測用途未驗收；舊hxfail1已接受−80的失敗與根因未解。匯出後90秒被動窗已完成，所有本版已保存連線COM3均已釋放。保持供電／UNCAL，未TARE／CAL、commit／push，366g校正未完成；各epoch的RAM與計數分開記錄。

## 授權、前情與epoch

使用者最新授權允許保持供電下重啟／燒錄失敗後DATA狀態診斷，繼續排查，待原因釐清後完成366g校正。實際部署證據另列於後節，授權本身不證明根因。

既有hxread1部署、session `238002AF`及凍結事件見[上一輪session](2026-10-28-hx711-per-read-diagnostic-session.md)與[同版獨立QC](../quality/reports/2026-10-28-hxread1-qc.md)。既有ring已凍結，後續ERR增量沒有對應的新逐讀ring可供回溯。診斷記錄、失敗後處理及同步DUMP會改變排程與觀察條件，須連同結果記錄，不能直接當成未介入的ADC行為。

本次90秒窗仍屬舊hxread1執行epoch；窗內沒有VERSION／HX_DIAG查詢，版本及epoch承接既有部署操作追溯，沒有在本窗重新讀回。後續若實際重啟／燒錄，必須另記版本、boot／session、啟動缺資料及QC，不跨epoch合計RX／ERR或沿用RAM校正。USB保持供電不代表RAM跨重啟保留；目前UNCAL，不套用歷史offset／scale。

## 燒錄前零TX被動證據

- [serial原始位元組](20261028-hx711-before-per-read-flash-101902.serial.log)
- [完整行、主機接收時間及欄位JSONL](20261028-hx711-before-per-read-flash-101902.lines.jsonl)
- [host設定、控制事件與收尾](20261028-hx711-before-per-read-flash-101902.host.json)
- [monitor控制通道原文](20261028-hx711-before-per-read-flash-101902.control.log)

主代理以serial-monitor v0.15.0（commit `13b8e80`）開COM3；OPEN前DESCRIBE／設定核對115200／8N1、DTR／RTS off。OPEN ACK與loopback TCP連線是監控器／埠回報，不是ESP32命令ACK。控制通道有設定與OPEN／CLOSE／QUIT，裝置命令TX為空；本窗沒有重置、TARE／CAL或WiFi切換。

host的`prior_empty_tray_confirmed=true`承接先前使用者空托盤確認，沒有新增同步物理量測或現場見證。檔名中的`before-per-read-flash`為主機擷取命名，不能解讀成後續燒錄已完成。

| 項目 | 核對結果 |
|---|---|
| PASSIVE_CAPTURE_START／END | 10:17:31.585～10:19:02.362，實際90.777秒 |
| 完整STATUS主機收行時間 | 10:17:32.005～10:19:01.369 |
| STATUS／不同RX | 90／90，RX33980→34800 |
| RAW平均 | −162622.6556 counts |
| RAW最小～最大／跨度 | −162750～−162533／217 counts |
| phase／CAL／近期感測器狀態 | IDLE／UNCAL／HX OK／ADXL OK |
| ERR／AERR／FIFO_FULL | 3／0／2，全窗未觀察到增加 |
| AGE_ms | 125～216 ms，為快取年齡而非ADC週期 |
| 相鄰快照 | 89對，最大絕對RAW差168 counts，沒有≥1000 counts跳變 |
| 裝置命令TX／主機overflow／failure | 0／false／null |

此前[09:31～09:33被動窗](20261028-hx711-postdump-passive-093308.serial.log)末筆09:33:07.623為ERR2，本窗首筆10:17:32.005已為ERR3。兩窗之間未觀測，只能記錄累計增加1在缺口中被發現，無法定位發生時間、原因或對應RAW；不能由本窗HX OK／ERR不增否定先前異常，也不能由既有凍結ring補造新事件。

## 完整性、保存及限制

serial共29886 bytes，與host記載一致；180個完整行對應180列JSONL。逐列核對原文、行號、UTF-8 byte offset及STATUS已解析欄位，差異均為0。**末96 bytes未完成片段原樣保留於serial，不補造行、不納入STATUS統計。** serial SHA256與host的`raw_sha256`一致。

| 公開檔 | SHA256 |
|---|---|
| serial.log | `26F7EB8C1FEAB14882F37F7F9CF04C2153A2D27C109295B7B9F84436B93F6367` |
| lines.jsonl | `D1BC4FD0DD67760795A5F767CDF4C78AE8EFD1F5F72D229DA54EB5AEF3EF8374` |
| host.json | `676D3C26C17628DB247538584B2F0C621246EB98D8896E70C643EDF3B6879776` |
| control.log | `20C89E60163C5035CC067814700B842114C0904728EDC0C73935360346B806A5` |

主機時間為完整行接收時間，並非HX711轉換或讀框UTC。約1Hz STATUS是快取快照，不涵蓋全部已接受讀值、被拒絕嘗試、失敗後DATA狀態或GPIO波形；此90秒未見大跳不能證明異常消失、量測精度合格或新診斷有效。未使用先前OLED數值。

host與控制原文均有CLOSE／QUIT成功，process exit0，`port_released_by_process_exit=true`，本窗COM3已釋放。此90秒窗結束時尚待新韌體設計交付及實際編譯／寫入；後續實際部署另列，不改寫此窗操作。此次文件整理未操作COM、未改韌體／handoff／QC報告，未commit／push。

## 補充：另一個燒錄前4分鐘零TX被動窗

- [serial原文](20261028-hx711-before-dout-diagnostic-102356.serial.log)
- [完整行JSONL](20261028-hx711-before-dout-diagnostic-102356.lines.jsonl)
- [host設定、控制事件及關埠](20261028-hx711-before-dout-diagnostic-102356.host.json)
- [monitor控制原文](20261028-hx711-before-dout-diagnostic-102356.control.log)

此窗仍是既有hxread1執行epoch的燒錄前被動觀察；不代表新DATA狀態診斷已編譯或部署。OPEN前維持115200／8N1、DTR／RTS off，裝置命令TX為空。

| 項目 | 核對結果 |
|---|---|
| PASSIVE_CAPTURE_START／END | 10:19:55.950～10:23:56.514，實際240.564秒 |
| 完整STATUS主機收行時間 | 10:19:56.595～10:23:55.524 |
| STATUS／不同RX | 239／239，RX35304→37494 |
| RAW平均／最小～最大 | −162656.1004／−162786～−162522 counts |
| RAW跨度／相鄰快照最大絕對差 | 264／182 counts；238對未見≥1000 counts跳變 |
| phase／CAL／近期感測器狀態 | IDLE／UNCAL／HX OK／ADXL OK |
| ERR／AERR／FIFO_FULL | 3／0／2，全窗未觀察到增加 |
| AGE_ms | 124～215 ms |
| 裝置TX／主機overflow／failure | 0／false／null |

前窗末STATUS 10:19:01.369至本窗首10:19:56.595相隔55.226秒；兩個擷取窗之間有缺口，不能稱連續監看或逐框資料。ERR3持平只涵蓋各自所見快照，不補足09:33後ERR2→3的未觀測事件，亦不能排除快照間異常或解除資料品質問題。

serial共79173 bytes、478個完整行；JSONL同為478列，原文／行號／UTF-8 byte offset／STATUS已解析欄位差異均0，bytes及SHA與host一致。**尾64 bytes未完成片段保留，不納入統計。**

| 公開檔 | SHA256 |
|---|---|
| serial.log | `1A72B6899904D4DEB9C894C4A26AC2101A22BA7107BF895A766C09CD2D854B72` |
| lines.jsonl | `32973DD234AA629FB187C132EF2B383DB2812C2E68C8AEA75EA185CCA870CB28` |
| host.json | `F7C44E086BF29371EF70324BE67F958B1771A7D0B868CF5F161B1F401115E864` |
| control.log | `20C89E60163C5035CC067814700B842114C0904728EDC0C73935360346B806A5` |

CLOSE／QUIT成功，process exit0且主機確認本窗COM3已釋放；此窗未TARE／CAL、重啟／燒錄或切換WiFi，沒有使用舊OLED數值。此窗結束時新診斷尚未部署；後續結果如下。

## hxfail1最終來源與正式部署

主代理交付Arduino CLI 1.5.1、ESP32 core 3.3.12，沿用FQBN `esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40`。正式compile／upload均exit0；公開文字及本輪獨立QC支持以下部署結果，Planck沒有重編、重燒或讀取含憑證建置產物。

- [正式編譯log](20261028-hxfail1-compile.log)：程式969957／1310720 bytes，全域107664／327680 bytes；剩餘220016 bytes為編譯器摘要，非實測runtime heap。
- [晶片識別log](20261028-hxfail1-identity.log)：COM3、ESP32-D0WD-V3 revision v3.1、MAC `e0:8c:fe:a8:42:bc`，符合既有目標。
- [正式upload log](20261028-hxfail1-upload.log)：同一MAC；0x1000、0x8000、0xE000、0x10000四區各有`Hash of data verified`，最後正常RTS hard reset並返回COM3。

前一`B04A650C…`候選由主代理中止編譯，未燒錄；不能當作實板部署來源。下表為Planck重新計算的最終公開來源SHA256，與交付一致。來源hash不等於獨立讀回的實板映像hash。

| 公開來源 | SHA256 |
|---|---|
| `05-test/05-test.ino` | `01A732FBF0E472FBC00BA5760C90DF0266F74BA05570FB338E95A9379F309C8C` |
| `05-test/hx_failure_diag.h` | `F210D64AC515A903354CC056CDD1D7670DFD44CCC217F5413A944241D872E818` |
| `05-test/hx_read_diag.h` | `0B50F6C10D928EB97A74934250DFF0025BFC70B2E7291DE6D954D4ABC4B0E564` |
| `05-test/manual_optical.h` | `71782ABCDFD85C74DC2D5B7EB3BED7645B9A8E80394969A864854B1FEFF254B1` |
| `05-test/network_wifi.h` | `53F1C85E2E25010F4852C69828D805F968F5231976CC7CC7BFD9B5C5C244615F` |

Gibbs交付259項Node及25項native主機檢查通過；此為開發者主機測試結果，與正式編譯、四區校驗及實板QC分開，不代表尚未觸發的DATA路徑已由實板通過。

| 公開部署log | SHA256 |
|---|---|
| compile.log | `C156B587B57D4C1891F17AC453715047BA8888A37390D8C6135E11C91326AE54` |
| identity.log | `FEF0D358972FD8D4576FAF8156D2DBB70F2E17B505150D26D357A5F456869183` |
| upload.log | `C7D269471F03C48A44725799C44E2F194E1E9748A813307871CE4CA0440C3D0F` |

## 新epoch首窗限定實板QC

- [首窗serial](20261028-hx711-hxfail-postflash-qc-104141.serial.log)
- [完整行JSONL](20261028-hx711-hxfail-postflash-qc-104141.lines.jsonl)
- [host操作與收尾](20261028-hx711-hxfail-postflash-qc-104141.host.json)
- [control原文](20261028-hx711-hxfail-postflash-qc-104141.control.log)
- [Ampere本版獨立QC](../quality/reports/2026-10-28-hxfail1-qc.md)

兩次HX_DIAG STATUS讀回版本`v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1-hxfail1`、session `0462E7C9`，與舊`238002AF`分開。自然boot banner、reset原因及HX_DIAG_READY沒有捕獲，保留待補，不用來源／執行中識別代替，也不為補畫面額外重啟。

| 項目 | 本窗已發生證據 |
|---|---|
| capture | 10:41:09.041～10:41:41.431，32.390秒 |
| STATUS收行時間／筆數 | 10:41:09.148～10:41:41.320；35 STATUS、35不同RX，306→598 |
| RAW平均／最小～最大 | −162764.8571／−162841～−162689 counts；相鄰最大絕對差144 counts |
| phase／CAL／近期健康 | IDLE／UNCAL／HX OK／ADXL OK |
| ERR／AERR／FIFO_FULL | 0／0／1，全窗未觀察到增加；不得與舊epoch相減當作修復 |
| ring摘要 | LIVE、records127、capacity256；attempts379→590，pre_overwritten252→463，overflow0、seq_wrapped0，未觸發 |
| DATA摘要 | 兩次均schema1、profile `SCK_LOW_250MS`、WAITING、captured0、next0、valid0、missed0，points12 |
| 裝置命令 | STATUS／ADXL_DIAG／HELP／WIFI STATUS／HX_DIAG STATUS／STATUS／ADXL_DIAG／HX_DIAG STATUS，共8條唯讀 |

OPEN前115200／8N1、DTR／RTS off；無TARE／CAL／UNCAL／ARM／STOP／CLEAR或WiFi開關。98個完整行、14833 bytes，無尾片；Planck逐列核對原文／行號／UTF-8 byte offset／STATUS已解析欄位差異0，serial SHA與host一致。

| 首窗公開檔 | SHA256 |
|---|---|
| serial.log | `6A428E9BEA5F6FE8A88FE43EF19DFED70BCD0F4532D9C20FA3ED55DC02354BCC` |
| lines.jsonl | `DA2B0E392430B36185AFCDF0DADAC51EBD0E8F3046B9205252DA3C9CFD2300CE` |
| host.json | `462A754C55DD77F4E2330D7E3BC3578790AA1C2AF17AC0BFC2CAE6BD71A7107F` |
| control.log | `20C89E60163C5035CC067814700B842114C0904728EDC0C73935360346B806A5` |

Ampere判定部署與首窗IDLE唯讀子項通過；HXFAIL1-Q01自然啟動缺證、Q02失敗後DATA路徑尚未觸發、Q03 FIFO條件未解、Q04其他實板路徑未測、Q05量測用途未驗收。首窗沒有LINK或大跳，不關閉前版已接受極端字的資料品質問題；新診斷的等待／hold會改變失敗後排程，後續若取得結果也須保留介入界線。

首窗CLOSE／QUIT成功、process exit0、overflow=false、failure=null，該連線COM3已釋放。其後主代理另開有界觀察窗，已取得下節凍結結果；只有主代理操作COM。CPU括弧／DATA離散讀取不等於GPIO波形，主機匯出時間不等於讀框UTC。新boot保持UNCAL，不套歷史係數；未完成366g校正。

## 同epoch ANY凍結：已接受−80異常字

- [觀察serial原文](20261028-hx711-hxfail-observe-104516.serial.log)
- [完整行JSONL](20261028-hx711-hxfail-observe-104516.lines.jsonl)
- [host操作及收尾](20261028-hx711-hxfail-observe-104516.host.json)

依Ampere核對capture為10:42:00.982～10:45:16.177、195.195秒；planned620000ms是上限，非實際時長。兩條唯讀TX為10:42:04.488 HX_DIAG STATUS及10:45:08.150 HX_DIAG DUMP。10:45:07.429先收到同session `0462E7C9`的FROZEN通知，並非DUMP強制凍結。

ring保存seq2220～2475共256列：127前史、觸發2347、128後史，freeze_reason1／trigger_flags4（JUMP），post_complete1、overflow0、seq_wrapped0。匯出時attempts2481＝pre_overwritten2219＋records256＋omitted6，收支一致；256筆全部accepted、rejected0，但accepted不等於資料正確。

| seq | bits24／RAW | 原接受／計數／旗標 | CPU讀取括弧／相鄰開始間隔 |
|---|---|---|---|
| 2346 | `0xFD83F6`／−162826 | accepted1、postcheck HIGH；RX2345→2346、ERR0→0、flags0 | 191µs／46927µs |
| **2347** | **`0xFFFFB0`／−80** | **accepted1、postcheck HIGH；RX2346→2347、ERR0→0、flags4** | **199µs／180053µs** |
| 2348 | `0xFD8403`／−162813 | accepted1、postcheck HIGH；RX2347→2348、ERR0→0、flags4 | 201µs／252000µs |
| 2349 | `0xFD8404`／−162812 | accepted1；RX2348→2349、ERR0→0、flags0 | 199µs／25956µs |

兩側正常近鄰數值算術右移11位均得−80，只是事後數值候選，不證明丟11位、不能還原／修RAW，也不確定與舊−594／−156或上一版−317事件同因。CPU括弧不是SCK高低脈寬或有效邊緣；所印25 pulses不是獨立量到25個有效clock。

獨立離線重跑公開解析器通過256列CRC／解碼與完整性，與完整新版分析JSON全物件一致。CRC只支持資料形成後的匯出完整性，不提供獨立ADC真值。保留讀取跨度為243114337～271364530 boot µs，trigger end257284592µs；主機匯出各列UTC不是當時讀框UTC。

本窗193個STATUS、193不同RX781→2528，收行10:42:01.401～10:45:15.813，STATUS RAW範圍−162918～−162654、平均−162788.9793 counts。約1Hz快照沒有印到逐框已接受的−80，不得用快照範圍冒充全部已接受字或排除異常。

## DATA未介入、完整匯出及工具更正

HX_FAIL匯出為CLOSED_NO_LINK（mode4、stop_reason3、captured0），failure metadata與resume均NA；12個HXFP索引及目標完整，每點valid0／missed0、時間及dout為NA。這是12個未取樣點，不是12個LOW或12個逾點。ANY先因JUMP凍結，沒有可介入LINK，停止後不rearm；不能推成整個boot永久沒有LINK。

**本次250ms DATA hold未執行。** seq2348開始間隔252000µs不能據此歸因新hold。103-word canonical CRC `0x7E8831FE`及END_HX_FAIL完整；NA欄位在CRC內部採零的規則不把它變成GPIO實測0。

原ring BEGIN／END主機收行10:45:08.206／10:45:10.559，HX_FAIL BEGIN／END為10:45:10.575／10:45:10.663，兩區塊完整跨度2.457秒。匯出為同步阻塞；Ampere記錄其後10:45:10.795 HX STALE／AGE2606ms、ADXL MISSING，10:45:11.797恢復近期OK並FIFO1→2。這是凍結後的介入條件，不能回因至seq2347；第二警示來源／漏樣量未由本窗ADXL_DIAG獨立定位。

主代理最初分析管道提早終止，保存的[原analysis.json](20261028-hx711-hxfail-observe-104516.analysis.json)僅202 bytes、JSON.parse失敗，截於read.accepted之後，尚未到timing欄。原檔保留不覆寫，不能稱完整舊摘要；原始serial／JSONL無損。另最初工具固定輸出timing=true的呈現問題，由Gibbs修正離線工具，未改FW或重燒。

後續採用[完整analysis-v1.1.0.json](20261028-hx711-hxfail-observe-104516.analysis-v1.1.0.json)。Planck從原serial重跑`node tools/hx-failure-diagnostic.cjs <serial>` exit0，輸出與保存JSON全物件一致：`profile_can_delay_next_attempt=true`只描述能力，`low_hold_intervention_applied=false`及`changed_next_attempt_timing=false`才是本次未介入結果。工具修正不消除seq2347量測失敗。

| 觀察窗／衍生資料 | SHA256 |
|---|---|
| observe serial.log | `DBE28C9020DC26AEC39D217AE6C3C6CFB14F7DB0A0A0621F0A58735DE8FF776B` |
| observe lines.jsonl | `37AF5B5BAAADB600A6BEE9842C0720B73FC9A2CED17C448C72137B75D24D52C0` |
| observe host.json | `6D2B9B68A8A124CC44EF5E555AEC7E3814FB9E71EF61305CF1E86216C252CC2E` |
| 不完整analysis.json | `4A08620549A3F442D93EB3373508C63D21271A891B3FC63BC4517A5A1BFC8BB9` |
| 完整analysis-v1.1.0.json | `8FFE1C98C68C7FDC260E21B76A9D2637B67A8CD746955046298E9089249E4A06` |
| 解析工具v1.1.0 | `BA9CA82BB424DD40A91EF8F2404CEB5B6B7E7B4A733CEBDF87A0765A0411257E` |

觀察serial共93358 bytes、674完整行，無尾片；Planck核對原文／行號／UTF-8 byte offset／STATUS解析欄位差異0、serial SHA與host一致。原始資料、工具及衍生輸出分別識別，不將原分析管道中止誤寫成serial擷取失敗。

## 匯出後零TX 90秒與目前交接

- [被動serial](20261028-hx711-postdump-passive-104807.serial.log)
- [完整行JSONL](20261028-hx711-postdump-passive-104807.lines.jsonl)
- [host操作與關埠](20261028-hx711-postdump-passive-104807.host.json)

capture10:46:36.431～10:48:07.333，90.902秒；STATUS收行10:46:37.123～10:48:06.476，90筆／90不同RX3270→4080。RAW平均−162800.4889、範圍−162939～−162692 counts；全窗IDLE／UNCAL／HX及ADXL OK、ERR0／AERR0／FIFO2不增，裝置TX為空。29700 bytes／180完整行，無尾片；原文／行號／offset／STATUS欄位差異0、SHA與host一致。

| 被動窗公開檔 | SHA256 |
|---|---|
| serial.log | `8719CAF75DF0640AA28B819E0CA27D3780C768EFA2A74B947517C85AD4D20766` |
| lines.jsonl | `72E5F7F160D8A5712A921EAF4EE0953EA73E70184A7764490CAD042E0014930B` |
| host.json | `08FDA8460C7999FA803D8163C8B11B6464168956808F2A946E65B8C35D7028E2` |

此窗與前窗有未擷取缺口，僅快照，不再查詢版本／session；依操作追溯承接本boot，不能冒充連續逐框或精確ADC時序。短窗ERR0不增不抵消已保存的−80。

[Ampere hxfail1版QC](../quality/reports/2026-10-28-hxfail1-qc.md)結論：部署／限定唯讀／凍結匯出通過，HXFAIL1-Q05／P1量測資料品質未通過，Q02實際DATA路徑未觸發；自然boot及其他缺項保留。當時Gibbs與Kierkegaard正實作下一版ANY與LINK雙槽，尚未燒錄；其後實際hxfail2部署另列於末節，不預寫新功能成功。現有hxfail1資料已保存，不修RAW或以安靜窗恢復校正。

本版首窗、觀察及匯出後被動三窗均CLOSE／QUIT成功、process exit0、overflow=false、failure=null，COM3已關閉／釋放。保持供電與UNCAL，無TARE／CAL、commit／push；Planck僅修改本session，未開COM、讀秘密、改handoff／韌體／QC。

## 凍結後10分鐘零TX被動續查

- [serial原文](20261028-hx711-hxfail1-frozen-passive-110219.serial.log)
- [完整行JSONL](20261028-hx711-hxfail1-frozen-passive-110219.lines.jsonl)
- [host設定、操作及收尾](20261028-hx711-hxfail1-frozen-passive-110219.host.json)
- [monitor控制原文](20261028-hx711-hxfail1-frozen-passive-110219.control.log)

主代理於10:52另開單一有界連線，OPEN前115200／8N1、DTR／RTS off；零裝置TX，沒有額外版本／session／DUMP查詢。依操作追溯承接hxfail1已凍結epoch，與前窗之間有未擷取缺口，不接成全程連續記錄。

| 項目 | 核對結果 |
|---|---|
| PASSIVE_CAPTURE_START／END | 10:52:19.318～11:02:19.707，實際600.389秒 |
| STATUS主機收行時間 | 10:52:19.407～11:02:18.721 |
| STATUS／不同RX | 598／598，RX6387→11842 |
| RAW平均／最小～最大 | −162868.8796／−162985～−162711 counts |
| RAW跨度／相鄰掃描 | 274 counts；597對最大絕對差211 counts，無≥1000 counts跳變 |
| phase／CAL／近期感測器狀態 | IDLE／UNCAL／HX OK／ADXL OK |
| ERR／AERR／FIFO_FULL | 0／0／2，全窗未觀察到增加 |
| 裝置TX／主機overflow／failure | 0／false／null |

serial共197574 bytes、1196完整行，與host一致；1196列JSONL的原文／行號／UTF-8 byte offset／STATUS已解析欄位差異0，serial SHA與host一致。**末32 bytes未完成片段原樣保留，不補造、不納統計。**

| 公開檔 | SHA256 |
|---|---|
| serial.log | `13BE76129B00B53DC05150F5465C75D2D5D09DAB6072ACFEEF5CB053D788A14C` |
| lines.jsonl | `89510ED6992F8A33D0CA66B3EA050C08DC95D2FCF380261C1DA7335FA1EFE6E6` |
| host.json | `94EF039F27D99AAE8AFE394E5C405EDC6BD522D1ED5198BDB362373965116F65` |
| control.log | `20C89E60163C5035CC067814700B842114C0904728EDC0C73935360346B806A5` |

ring早已凍結，此窗約1Hz STATUS不是逐框，沒有重新保存每次嘗試或失敗後DATA。未見相鄰大跳／ERR增加，不能排除快照間已接受異常字、判定修復或解除既有−80及UNCAL問題；量測未通過與DATA路徑未觸發的結論保留。

CLOSE／QUIT成功、process exit0、主機確認`port_released_by_process_exit=true`，最新連線COM3已實際釋放。未TARE／CAL、重啟／燒錄、切WiFi、commit／push；本次只更新本文件，未開COM或改其他文件。

## hxfail2雙槽版正式部署：另立新epoch

主代理沿用Arduino CLI 1.5.1、ESP32 core 3.3.12及FQBN `esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40`，正式compile／upload均exit0。此為使用者既有保持供電下重啟／燒錄診斷授權的後續執行；新epoch不恢復舊RAM校正或將新計數歸零當作異常已修復。

- [正式compile log](20261028-hxfail2-compile.log)：程式999741／1310720 bytes，全域122088／327680 bytes，剩餘205592 bytes為編譯器容量摘要，非實測runtime heap。
- [晶片識別log](20261028-hxfail2-identity.log)：COM3、ESP32-D0WD-V3 revision v3.1、MAC `e0:8c:fe:a8:42:bc`，符合目標。
- [正式upload log](20261028-hxfail2-upload.log)：同一MAC，0x1000／0x8000／0xE000／0x10000四區各有Hash校驗通過，正常RTS hard reset及COM3返回。

Planck重新計算下列公開來源SHA256，與Gibbs／主代理交付一致；沒有讀BIN／ELF、生成來源或秘密檔。來源hash、編譯及寫入驗證與實板功能QC分開判斷。

| hxfail2公開來源 | SHA256 |
|---|---|
| `05-test/05-test.ino` | `06D3066BE9B64CBD012DC1C66B5AB9CF190D57D821FC1D647008092BCFC71139` |
| `05-test/hx_read_diag.h` | `A488BA5FA1149123B8CA55592E0048AED23642E486DD462E802453C875090864` |
| `05-test/hx_failure_diag.h` | `F210D64AC515A903354CC056CDD1D7670DFD44CCC217F5413A944241D872E818` |

| 部署log | SHA256 |
|---|---|
| compile.log | `FD4E46B2C640113D19AFFC4682F57E5F9854A0D08B48489929B2164A6C2B98FD` |
| identity.log | `730EDBFAE10AAB7BD009DCF88A874BAF18C29C3C1AC20FB20B0012CB09D393D4` |
| upload.log | `8B2502F09B7D1A0FCAC4A568BECAAB1C4F2D35181291F6FF1CFB6035B14271C3` |

Gibbs交付272項Node及39項native主機檢查通過，非實板DATA路徑驗證。雙槽設計為ANY trigger_mask7與LINK trigger_mask1各保留256列、共用本boot attempt序號，DATA只綁LINK；外層schema2 bundle須兩槽封存並完整收到END_HX_CAPTURE才能判完整匯出。此處記錄設計與首窗摘要，尚未取得實板bundle，不填事件、CRC或實際hold結果。

## hxfail2新epoch首窗：兩槽LIVE／DATA WAITING

- [首窗serial](20261028-hx711-hxdual-postflash-qc-111127.serial.log)
- [完整行JSONL](20261028-hx711-hxdual-postflash-qc-111127.lines.jsonl)
- [host設定、操作及收尾](20261028-hx711-hxdual-postflash-qc-111127.host.json)
- [control原文](20261028-hx711-hxdual-postflash-qc-111127.control.log)

兩次HX_DIAG STATUS的ANY與LINK均讀回版本`v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1-hxfail2`、session `B17323B9`，與hxfail1 `0462E7C9`分開。自然boot banner／reset原因／HX_DIAG_READY仍缺證，執行中版本不代替自然boot證據，不為補畫面額外重啟。

| 項目 | 已保存首窗證據 |
|---|---|
| OPEN ACK～CAPTURE_END | 11:10:54.682～11:11:27.030，32.348秒 |
| STATUS完整收行 | 11:10:54.893～11:11:26.030；34筆／34不同RX，287→569 |
| RAW平均／最小～最大 | −162945.8824／−163035～−162829 counts |
| phase／CAL／近期感測器狀態 | IDLE／UNCAL／HX OK／ADXL OK |
| ERR／AERR／FIFO_FULL | 0／0／1，全窗未觀察到增加，不能跨epoch解讀成清除缺陷 |
| ANY摘要 | schema2、trigger_mask7、LIVE、records127／capacity256、export_sealed0，未觸發 |
| LINK摘要 | schema2、trigger_mask1、LIVE、records127／capacity256、export_sealed0，未觸發 |
| 兩槽live計數 | attempts360→570，pre_overwritten233→443，omitted0／overflow0／seq_wrapped0；各槽收支一致 |
| DATA摘要 | schema1、WAITING／captured0／next0／valid0／missed0，12 points，未實際取樣 |
| 裝置唯讀TX | STATUS／ADXL_DIAG／HELP／WIFI STATUS／HX_DIAG STATUS／STATUS／ADXL_DIAG／HX_DIAG STATUS，共8條 |

OPEN前115200／8N1、DTR／RTS off。serial16159 bytes、104完整行，無尾片；Planck核對原文／行號／UTF-8 byte offset／STATUS欄位差異0，serial SHA與host一致。這些核對僅支持首窗保存與摘要事實，不能代替正在進行的新版獨立QC。

| 首窗公開檔 | SHA256 |
|---|---|
| serial.log | `B3768C04AD7548DA029BF5590369B24DD8CEEADDC5DE8C227490A3D32ED9A15B` |
| lines.jsonl | `0484C32C4A68EC62E13848A61A56FDDAC8F5165E5F74DCFD2202C4DC906DDB3E` |
| host.json | `F8C4553403D278214DA9F96FBA7389F5C72FC7E5F2DBE6C8BC60698621D2A518` |
| control.log | `20C89E60163C5035CC067814700B842114C0904728EDC0C73935360346B806A5` |

首窗CLOSE／QUIT成功、process exit0、overflow=false、failure=null，該連線已釋放。其後主代理另持COM3有界觀察，完整bundle及新版QC結果現已取得，另列於下節；Planck未開埠。資料品質／根因仍待釐清，舊−80失敗不因新boot或首窗ERR0而關閉；維持UNCAL，未完成366g校正。

## hxfail2共同600秒到期封存與完整bundle

- [觀察serial](20261028-hx711-hxdual-observe-112033.serial.log)
- [完整行JSONL](20261028-hx711-hxdual-observe-112033.lines.jsonl)
- [host操作及收尾](20261028-hx711-hxdual-observe-112033.host.json)
- [control原文](20261028-hx711-hxdual-observe-112033.control.log)
- [完整analysis-v2](20261028-hx711-hxdual-observe-112033.analysis-v2.json)
- [Ampere hxfail2獨立QC](../quality/reports/2026-10-28-hxfail2-qc.md)

主機capture為11:11:46.968～11:20:33.180、526.212秒，晚於裝置setup開始，不能當裝置600秒窗口。只送11:11:50.470 HX_DIAG STATUS及11:20:25.154 HX_DIAG DUMP兩條唯讀TX；11:20:24.412已先收到`all_frozen=1`且兩槽reason3的通知，DUMP沒有強制凍結。

| 封存項目 | 獨立核對及限制 |
|---|---|
| 身分與共同期限 | 兩schema2槽及schema1 HXF同為hxfail2／session `B17323B9`；start_us516298、目標600000000µs，freeze_us600604451，實際600.088153秒，比目標晚88.153ms；是主迴圈服務期限，不是硬即時或ADC時鐘。 |
| 兩槽狀態 | ANY mask7／LINK mask1均FROZEN、reason3 WINDOW、export_sealed1、trigger_flags0；pre0／post0／post_complete0為無觸發到期路徑，不能當事件前後史失敗或造時刻0事件。 |
| 固定封存計數 | 每槽attempts5473＝pre_overwritten5346＋records127＋omitted0，overflow0／seq_wrapped0。這是封存計數，不須追上DUMP後live RX。 |
| 保留列及重疊 | 每槽seq5347～5473、127列，accepted127／rejected0、ERR0／flags0。兩槽輸出共254列，但127個seq及逐word完全重疊，只有127個不同attempt，不算254個ADC框或兩次獨立量測。 |
| 完整性 | 兩槽CRC、補數解碼及資料契約通過；Planck另比較原始HXRD整列，127組各恰重複2次，與解析器canonical重疊結果一致。 |
| 保存時間範圍 | 首start586796482～末end600461616 boot µs，所保留跨度13.665134秒；不是完整600秒每個原始字。 |

按**已部署的觸發契約及封存摘要**，本窗5473次完成hook沒有觸發LINK／SAT或相鄰已接受值差≥1000 counts。此為受限軟體監測結果，不是5473次全部ADC conversion、全窗原始字獨立驗真或可靠度／精度合格；已覆寫的5346個字沒有完整匯出，不能補造。舊−80可能被接受且未由STATUS看到的證據仍成立，這次無觸發不能判修復。

外層BEGIN_HX_CAPTURE收於11:20:25.208、END_HX_CAPTURE於11:20:27.794，完整跨度2.586秒；ANY BEGIN／END為25.227／26.444，LINK為26.463／27.684，HXF END為27.786。最外層END包含complete1／retained1／read_only1，不能只收第一槽或HXF尾行就稱整包完成。

HXF為CLOSED_NO_LINK／captured0，12個HXFP均valid0／missed0、begin／end／dout NA，metadata與resume為NA。103-word CRC `0x7E8831FE`通過，支持無事件封存及NA匯出，不是12個LOW。**本次未執行250ms hold**，`low_hold_intervention_applied=false`、`changed_next_attempt_timing=false`；真LINK後DATA採點／resume及先ANY凍結後LINK仍能取得事件的實板情境尚未觸發。

Planck從原serial重跑v2.0.0解析器exit0，整份JSON（1個bundle的陣列）與保存analysis-v2全物件一致。CRC為匯出完整性，CPU讀取括弧不是SCK實際邊緣，主機收行UTC不是原讀框時刻；沒有獨立ADC真值。

| 觀察窗／分析識別 | SHA256 |
|---|---|
| serial.log | `3DFC48A4E883BAC56DC131AA2BD4EDD9E70717628A01539F009EB7FE847704A7` |
| lines.jsonl | `479C3A6700E3B1EE779986D3A8604A88E4C075C724BE5F16BE3CD0067A33C046` |
| host.json | `8F63C0CBFE40524A7A0003A067D85B2F43A3D20AFC8126ED26C8EEC6EC3CA05D` |
| control.log | `20C89E60163C5035CC067814700B842114C0904728EDC0C73935360346B806A5` |
| analysis-v2.json | `122B8CF0346A9E68094378271D71DEED3D67D0228287E815C66CFCCBF88B0E9A` |
| failure解析器v2.0.0 | `4A27686D7E1DAC6C5D5B98D3915F594368CFF1F64B35BB1C23CE3755B17CA953` |
| read解析器 | `2312CF63C68E532D246470D82CD1C74ED1F700E75E09B62CB870EF862ACA037F` |

原serial204595 bytes／1345完整行、無尾片；Planck核對原文／行號／UTF-8 byte offset／STATUS已解析欄位差異0、SHA與host一致。本窗523 STATUS／523不同RX760→5527，收行11:11:47.160～11:20:32.952，RAW平均−162966.9006、範圍−163095～−162831 counts；這是快照統計，不能補足未保存讀框。

## hxfail2 DUMP後阻塞及90秒零TX窗

同步DUMP輸出會暫停輪詢。外層END後，11:20:27.932 STATUS出現HX STALE／AGE2738ms、ADXL MISSING、FIFO1；11:20:28.933近期狀態恢復OK並FIFO升為2，ERR0／AERR0未增。這是凍結後匯出介入的觀察條件，不回因至封存前資料。本窗沒有另取ADXL_DIAG，第二警示來源及漏樣量未獨立定位，不把它與HX同因。

- [匯出後被動serial](20261028-hx711-postdump-passive-112258.serial.log)
- [完整行JSONL](20261028-hx711-postdump-passive-112258.lines.jsonl)
- [host與收尾](20261028-hx711-postdump-passive-112258.host.json)
- [control原文](20261028-hx711-postdump-passive-112258.control.log)

capture11:21:27.384～11:22:58.269，90.885秒；90筆STATUS收行11:21:28.156～11:22:57.510，90不同RX6032→6845。全窗IDLE／UNCAL／HX及ADXL OK、ERR0／AERR0／FIFO2持平，設備零TX；RAW平均−162997.5667、範圍−163088～−162913 counts。serial29700 bytes、180完整行，無尾片；原文／行號／offset／STATUS欄位差異0、SHA與host一致。

| 被動窗公開檔 | SHA256 |
|---|---|
| serial.log | `7EC85A83FBD7EDFAD378E1376011EB131665EE521E994EFE80203AB05C15C485` |
| lines.jsonl | `38041CE33308075B2673D6FB41633990D64322CCA97819E4FAE7146126D5236D` |
| host.json | `3F659C75BF4271E4E868E8102F87599142574E6BB4B449846D2580A1ACA62666` |
| control.log | `20C89E60163C5035CC067814700B842114C0904728EDC0C73935360346B806A5` |

此窗與前窗有缺口、未再讀版本／session，僅依本輪操作追溯；約1Hz快照不涵蓋每個字，不判全部ADC正確或故障消失。[hxfail2獨立QC](../quality/reports/2026-10-28-hxfail2-qc.md)判部署及到期封存／bundle有限子項通過，Q02真LINK DATA／分離觸發仍未測，Q03匯出及FIFO條件未解，Q05量測用途未驗收；不解除舊−80問題或恢復CAL。

本版首窗、bundle觀察及匯出後被動三窗CLOSE／QUIT成功、process exit0、overflow=false、failure=null，COM3均已釋放，當前無本輪擷取持有。保持供電及UNCAL，未TARE／CAL、commit／push。Planck只更新本session，未開COM、讀秘密或更改handoff／韌體／QC。
