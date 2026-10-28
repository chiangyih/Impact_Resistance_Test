# 2026-10-28 HX711 PCNT數位邊緣診斷session

日期／時區：2026-10-28，Asia/Taipei（UTC+08:00），主機時間採24小時制。主代理獨占COM3；Gibbs與Kierkegaard負責診斷設計與物理限制，Ampere負責本版獨立QC，Planck整理公開追溯。

**pcnt1第02輪`DF497E3D`已可信捕獲首LINK後MCU數位計數：有效窗內SCK0／DATA4，SCK時窗包絡DATA；HXF另有8個有效LOW離散點／4個missed。** ANY仍接受−641／ERR0，之後LINK拒絕−529408並ERR0→1；捕獲與CRC通過不解除量測品質失敗或定位根因。主代理已停止循環重啟，boot、bundle及後90秒擷取均已關閉、COM3已釋放，USB保持供電／UNCAL，未TARE／CAL、新flash或Git操作。每讀框PCNT僅在只讀評估可行設計，尚未執行；首部署自然boot缺證不由第二次畫面回填。

## 前情、目的與觀察限制

前版[續錄session](2026-10-28-hxdual-until-link-session.md)已在hxfail2／`A1C20320`保存被接受的−320及稍後LINK拒絕−160，取得9個有效LOW離散點、3個missed／NA，實際hold273.810ms。捕獲目標達成後停止循環重啟；9點之間仍有約171ms盲區，不能判連續LOW或根因。前版[完整QC](../quality/reports/2026-10-28-hxfail2-continuous02-qc.md)判DATA捕獲可信但量測品質未通過。

Gibbs／Kierkegaard依既有授權設計並交付pcnt1，觀察既有GPIO25 SCK／GPIO33 DATA的MCU端數位邊緣，以補充離散觀察；不改線不代表GPIO設定完全未受影響。SDK driver設定曾暫改pull，設計包含復原與讀回；正對照、API及等待也會改排程，不能稱零干擾。

PCNT邊緣總數不等於HX端類比波形、脈衝寬度、電壓幅度或供電量測；即使有效計數0，也不證連續類比LOW或ADC ready。CPU／API括弧、MCU數位觀察與主機收行UTC分開，不以內建診斷直接定位根因、修RAW或判校正通過。

本次寫入重啟另立epoch `EC44F494`，不跨boot合計RX／ERR／FIFO或沿用`A1C20320`及更早RAM係數；新計數歸零不是舊異常修復。自然boot／reset原因未捕獲，執行中版本不代替boot證據，也不為補畫面額外reset。

## 公開主機驗證、正式編譯與寫入

- [公開主機驗證摘要](20261028-pcnt1-public-host-validation.json)
- [正式compile log](20261028-pcnt1-compile.log)
- [晶片identity log](20261028-pcnt1-identity.log)
- [正式upload log](20261028-pcnt1-upload.log)

主代理交付Arduino CLI 1.5.1、ESP32 core3.3.12／IDF5.5.5，沿用FQBN `esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40`，正式compile／upload均exit0。compile摘要為程式1013665／1310720 bytes、全域123928／327680 bytes、剩餘203752 bytes；此為編譯器容量摘要，非實測runtime heap。

identity與upload同為COM3、ESP32-D0WD-V3 revision v3.1、MAC `e0:8c:fe:a8:42:bc`，符合目標。0x1000／0x8000／0xE000／0x10000四區各有`Hash of data verified`，最後正常RTS hard reset並返回COM3。主機編譯／部署與實板功能／量測品質分列，不互相代替。

公開驗證摘要記錄Gibbs交付293項Node、18項新portable native、15項實際adapter mock檢查通過及SDK header語法檢查。該JSON的pending compile／device_validation=false是正式部署前的主機交付狀態，原樣保留；後續正式編譯及實板結果依本文件各節，不改寫舊metadata。Planck未重跑整套主機測試、未讀秘密或BIN／ELF／含憑證build。

Planck於正式部署交付時重新計算以下9個公開來源SHA256，均與當時公開驗證摘要一致；後續離線工具修正另列，不覆寫本表。來源hash不等於獨立讀回的實板映像hash。

| 公開來源 | SHA256 |
|---|---|
| `05-test/05-test.ino` | `FA8B56417FE50B305904C831CF1DF8534549A4AA7C94D81896CD8E625ED9B7BF` |
| `05-test/hx_pcnt_diag.h` | `0DA4AF29B91D68FFECA6D017CBA718C0BC7CF7FFF56A4889221731F9A56D67A6` |
| `05-test/hx_pcnt_adapter.h` | `9E93F7D35BCDD0FBC0E2EFB9B139EB228C1197342CBDDE53ABF69AAFDD146FBB` |
| `05-test/hx_read_diag.h` | `A488BA5FA1149123B8CA55592E0048AED23642E486DD462E802453C875090864` |
| `05-test/hx_failure_diag.h` | `F210D64AC515A903354CC056CDD1D7670DFD44CCC217F5413A944241D872E818` |
| `05-test/manual_optical.h` | `71782ABCDFD85C74DC2D5B7EB3BED7645B9A8E80394969A864854B1FEFF254B1` |
| `05-test/network_wifi.h` | `53F1C85E2E25010F4852C69828D805F968F5231976CC7CC7BFD9B5C5C244615F` |
| `tools/hx-failure-diagnostic.cjs` | `EE5F30553F3FC32528B29A03AEEA4746918ABC251C2979DAEFD3A1FCCA8A8FDD` |
| `tools/hx-pcnt-diagnostic.cjs` | `68398554C77F1C0EE66663F7A33309A526F8861DF893CE134209CDC0A5A1D89A` |

| 公開部署／主機紀錄 | SHA256 |
|---|---|
| compile.log | `B4B49A4C5CAF7C38D1DCCB1AAE1EE38C41711A90DA5FB2E8F243870FAFE8F40E` |
| identity.log | `FEF0D358972FD8D4576FAF8156D2DBB70F2E17B505150D26D357A5F456869183` |
| upload.log | `6C44B9842D13FA264C9AADF99CD9C82D9B7C8115E26704461FAA59B44164161F` |
| public-host-validation.json | `4733F4152B2B62944A58A98E6440F06134DE19CCE603C246513BFB43FF934F8D` |

## 首窗限定QC：摘要已讀回、個別proof待bundle

- [首窗serial](20261028-hx711-hxpcnt-postflash-qc-134539.serial.log)
- [完整行JSONL](20261028-hx711-hxpcnt-postflash-qc-134539.lines.jsonl)
- [host設定、命令及收尾](20261028-hx711-hxpcnt-postflash-qc-134539.host.json)
- [control原文](20261028-hx711-hxpcnt-postflash-qc-134539.control.log)
- [Ampere pcnt1獨立QC](../quality/reports/2026-10-28-pcnt1-qc.md)

兩次HX_DIAG STATUS讀回完整版本`v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1-hxfail3-pcnt1`、session `EC44F494`。serial-monitor v0.15.0，OPEN前115200／8N1、DTR／RTS off；OPEN ACK～CAPTURE_END為13:45:07.467～13:45:39.833，32.366秒。monitor ACK不是ESP32 ACK。

| 首窗項目 | 已發生證據及限制 |
|---|---|
| STATUS | 13:45:08.072～13:45:39.550，34行／33不同RX，539→826；不把重印列當新ADC框。 |
| RAW快照 | 全STATUS均值−163667.4706、範圍−163748～−163585 counts，含重印，非逐框或精度證據。 |
| phase／CAL／近期健康 | IDLE／UNCAL／HX及ADXL OK，ERR0／AERR0／FIFO1持平；不跨epoch解讀為已清除缺陷。 |
| ANY／LINK摘要 | schema2、mask7／1，均LIVE、records127／capacity256、export_sealed0，未觸發。 |
| HXF摘要 | schema1、profile `SCK_LOW_250MS_PCNT`、WAITING／captured0／next0／valid0／missed0，尚無真LINK或DATA採點。 |
| PCNT摘要 | profile `PAD_EDGES_PROOF4OF8`，mode_id0、setup_code1、clock_blocked0、proof_count4／proof_good4／proof_bad0／proof_ready1、linked0／sealed0、snapshot_bytes1620。 |
| 唯讀TX | STATUS／HX_DIAG STATUS／ADXL_DIAG／HELP／WIFI STATUS／STATUS／ADXL_DIAG／HX_DIAG STATUS，共8條，無TARE／CAL、ARM／CLEAR或WiFi切換。 |

PCNT的4 good／ready只表示韌體摘要回報，不能先填個別proof的GPIO設定、SCK／DATA邊緣數或CRC均已獨立通過。完整schema3 bundle需ANY／LINK、HXF、HX_PCNT及最外END_HX_CAPTURE；PCNT 405-word CRC、個別proof與事件綁定均待實際取得，不以摘要或主機synthetic fixture代替。

首窗serial16598 bytes／106完整行、無尾片；Planck逐列核對原文／行號／UTF-8 byte offset／STATUS已解析欄位差異0，serial SHA與host一致。只核對公開資料，未讀含憑證生成來源或私有live目錄。

| 首窗公開檔 | SHA256 |
|---|---|
| serial.log | `57A54B9ACA6FC98DDE94D9547CDE73896EB080979D2760DD3FBBBDB54904214A` |
| lines.jsonl | `79CC678F669EEBE44959A56DBFB953406A8E91CB9032C08D07129B04306E1D06` |
| host.json | `03AA28D2596F90C8F9034935A05D0B8F532C0C8809F5AF37B71FCC164FB0C170` |
| control.log | `20C89E60163C5035CC067814700B842114C0904728EDC0C73935360346B806A5` |

Ampere目前判部署及限定IDLE唯讀子項通過，實際GPIO／counter／事件bundle驗證待補；量測品質及根因未解。不能把pcnt1新計數、摘要ready或後續有限數位觀察當舊已接受異常字已修復，亦不直接CAL366。

## 首窗收尾與後續觀察

首窗CLOSE／QUIT成功、process exit0、overflow=false／failure=null，該連線已釋放；主代理其後另開同epoch有界觀察，完整bundle結果已取得，另列如下。首窗當時的摘要與後來個別proof驗證分開，不回填當時已取得CRC。未TARE／CAL，保持供電及UNCAL，不沿舊RAM係數。

## PCNT首輪完整bundle：雙槽WINDOW／未捕獲LINK

- [觀察serial](20261028-hx711-hxpcnt-continuous-observe-135419.serial.log)
- [完整行JSONL](20261028-hx711-hxpcnt-continuous-observe-135419.lines.jsonl)
- [host設定、session guard及收尾](20261028-hx711-hxpcnt-continuous-observe-135419.host.json)
- [control原文](20261028-hx711-hxpcnt-continuous-observe-135419.control.log)
- [完整analysis-v3](20261028-hx711-hxpcnt-continuous-observe-135419.analysis-v3.json)

本窗OPEN ACK為13:46:18.856，capture結束13:54:19.491；host expected_session與observed_session均`EC44F494`。只送13:46:22.360 HX_DIAG STATUS與13:54:11.468 HX_DIAG DUMP兩條唯讀TX。13:54:10.503先收到schema3／all_frozen1／pcnt_sealed1且兩槽reason3通知；外層BEGIN／END_HX_CAPTURE收行13:54:11.525～13:54:14.375，完整跨度2.850秒，不以HXF或PCNT自己的END代替整包結束。

| 雙槽／HXF項目 | 核對結果及限制 |
|---|---|
| 共用窗口 | start_us517561、目標600000000µs、freeze_us600615532，實際600.097971秒，晚97.971ms；CPU服務期限不是硬即時或GPIO時刻。 |
| ANY／LINK | 同pcnt1／`EC44F494`，均WINDOW／reason3、trigger_flags0，各127列seq5372～5498、accepted127／rejected0。 |
| 固定封存收支 | 每槽5498＝5371 overwritten＋127 retained，omitted0、overflow0／seq_wrapped0；不跨槽相加完成attempt數。 |
| 重疊／CRC | 共254個HXRD列，127個seq逐word及原始整列完全重疊，只有127個不同attempt；兩槽19-word CRC與完整契約通過。 |
| HXF | CLOSED_NO_LINK／captured0、12個未取樣NA，valid0／missed0；metadata／resume NA，103-word CRC `0x7E8831FE`通過，本輪沒有hold或失敗後DATA採點。 |

依已部署觸發契約及封存收支，這個有限窗口未觸發規則；不是5498個原字經獨立驗真、全部ADC conversion或故障已修。5371筆已覆寫，原字只保存末127筆；不能補造完整600秒逐框資料，亦不以無LINK解除舊已接受異常字或恢復CAL。

## 個別PCNT正對照及405-word CRC

PCNT封存schema1／profile `PAD_EDGES_PROOF4OF8`、setup_code1、clock_blocked0、proof_count4／good4／bad0／ready1，linked0。4筆皆來自本boot既有accepted讀框，SCK／DATA兩unit有效、overflow0，其數位邊緣總數如下：

| proof seq | SCK valid_count | DATA valid_count／expected_data | proof有效性 |
|---|---|---|---|
| 1 | 50 | 11／11 | valid1 |
| 2 | 50 | 9／9 | valid1 |
| 3 | 50 | 9／9 | valid1 |
| 4 | 50 | 9／9 | valid1 |

`END_HX_PCNT`收行13:54:14.368，CRC `0x8DE1FCB4`／words405通過。Planck從原serial重跑v3.0.0工具exit0，全部JSON與保存analysis-v3全物件一致，兩槽、HXF與PCNT完整CRC／資料契約通過。Ampere正作完整獨立QC核查，這些檔案核對不代替尚未觸發的LINK事件路徑驗證。

正對照只支持本MCU計數器與既有軟體讀框的數位轉換數相符；SCK50不證HX端收到25個有效脈衝，DATA預期數與RAW同源，不是獨立ADC真值。未LINK時event計數無有效採樣，derived valid_count為null；內部raw_count0不能解讀成有效「0個事件邊緣」。沒有失敗後PCNT窗口，不能判盲區內無跳變、連續LOW或供電正常。

## 首輪快照、DUMP干擾及完整性

476 STATUS／476不同RX1191→5546，收行13:46:19.691～13:54:18.537；RAW平均−163688.9706、範圍−163807～−163555 counts。全窗IDLE／UNCAL、ERR0／AERR0，約1Hz快照非逐框或精度證據。13:54:14.515於同步DUMP後印HX STALE／AGE3008ms、ADXL MISSING／FIFO1；13:54:15.520近期OK並FIFO升2，其後持平。此為封存後已知匯出阻塞條件，不回因至PCNT正對照或指定HX根因；未另取ADXL_DIAG定位第二來源，不推實際漏樣量。

serial192436 bytes／1293完整行、無尾片；Planck逐列核對原文／行號／UTF-8 byte offset／STATUS已解析欄位差異0，serial SHA與host一致。主機UTC為完整行接收時間，不貼成各CPU／GPIO原始取樣時刻，無效值保留NA／null。

| 首輪bundle公開檔 | SHA256 |
|---|---|
| serial.log | `E9C72D260B7F8DE4DCFD74A2A80FEC0A828D9C51F09695E94F1F0EB098CF9730` |
| lines.jsonl | `40C2CAE7D26497885A731A9DEB6878FDB89E1DC0428D712D16C4B50B4BFFD4C8` |
| host.json | `CB5BE88B4CAC9C1173E81B09F7C68D71E29DBD273AA185DB24A93D6F6096B533` |
| control.log | `20C89E60163C5035CC067814700B842114C0904728EDC0C73935360346B806A5` |
| analysis-v3.json | `1B70299DC8039B8DF7037201B4BCE7FC055C25BE69943DF962F685BEBB423D6D` |

首輪觀察CLOSE／QUIT成功、process exit0、overflow=false／failure=null，主機確認該連線COM3已釋放。[Ampere本版完整QC](../quality/reports/2026-10-28-pcnt1-qc.md)現已獨立核對GPIO setup復原、四筆PCNT正對照、雙槽WINDOW及CRC／匯出有限子項通過；真LINK後PCNT計數窗未觸發、量測用途未通過，首部署自然boot仍缺證。其後依原授權的安全續輪已實際進行，另列如下；不以首輪無事件解除舊異常或進CAL。

## 第02輪前檢：首輪雙槽與PCNT已封存

- [續輪前檢serial](20261028-hx711-hxpcnt-next-precheck-135734.serial.log)
- [完整行JSONL](20261028-hx711-hxpcnt-next-precheck-135734.lines.jsonl)
- [host設定、命令及收尾](20261028-hx711-hxpcnt-next-precheck-135734.host.json)

OPEN前115200／8N1、DTR／RTS off；13:57:25.365 OPEN ACK後，只送13:57:28.870 STATUS、13:57:29.975 HX_DIAG STATUS，capture結束13:57:34.089。9個STATUS收行13:57:26.247～13:57:33.330，RX7265→7330，IDLE／UNCAL／HX及ADXL OK、ERR0／AERR0／FIFO2。

讀回pcnt1／`EC44F494`，ANY／LINK均FROZEN／reason3／records127／export_sealed1，HXF CLOSED_NO_LINK／captured0；PCNT setup1／clock_blocked0／proof4 good4 bad0 ready1、linked0／sealed1。這是已保存首輪的封存狀態，不是新事件。前檢CLOSE／QUIT成功、process exit0、failure=null，該連線已關閉。

## 第02輪同版重啟：DF497E3D自然boot已捕獲

- [第02輪boot serial](20261028-hxpcnt-continuous-02-boot.serial.log)
- [逐行接收JSONL](20261028-hxpcnt-continuous-02-boot.lines.jsonl)
- [SerialPort設定、guard及RTS事件host](20261028-hxpcnt-continuous-02-boot.host.json)

同版續輪未新flash；主代理以.NET SerialPort、115200／8N1、handshake none開COM3，開埠前DTR／RTS false，USB未拔。守衛實際保存3筆舊epoch IDLE／UNCAL狀態、RX7644→7662前進，通過後只作一次RTS assert／release。

| 已發生操作／自然輸出 | 主機時間（台北24小時制） |
|---|---|
| SerialPort OPEN | 13:58:06.5160920 |
| PRE_RESET_GUARD_PASS | 13:58:10.0560258，status_count3／last_rx7662 |
| 單次RTS assert／DTR false | 13:58:10.0569082 |
| RTS release／DTR false | 13:58:10.2725575；目標200ms，主機單調呼叫間隔215.6626ms，不是獨立管腳電壓波形 |
| 自然ROM | 13:58:10.3034272印`rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)`；不據此判USB曾斷電 |
| 自然應用版本 | 13:58:10.7686689，`v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1-hxfail3-pcnt1`、UNCAL |
| HX_DIAG_READY | 13:58:10.8321927，schema3／session `DF497E3D`／slots2／pcnt_schema1／window600000000µs／pcnt_setup1／clock_blocked0 |
| 初始HXF／PCNT | 13:58:10.8645513，HXF WAITING／captured0；PCNT setup1／clock0、proof_count0／good0／bad0／ready0、linked0／sealed0 |
| boot擷取結束／CLOSE | 13:58:25.2878847／13:58:25.2952263，port_closed=true／failure=null |

新boot的初始proof0只是該時刻狀態，不能沿用首輪4 good當作本輪個別proof已驗證，也不推成後續始終0。boot檔17筆STATUS包含重啟前3筆及新boot14筆；新boot先HX WAIT／RAW NA／RX0，後續HX OK到RX121、ERR0／AERR0。FIFO於13:58:14.2646665 RX28仍1、13:58:15.2586260 RX37已2，其後至boot檔末持平；修正先前簡述僅列FIFO1的遺漏。這是DUMP前增加，不歸因後來匯出。NA不補0，RX與RAM不跨boot串接；本次自然banner只補足本次啟動，不能回填首部署缺證。

前檢4958 bytes／28完整行，boot7376 bytes／57完整行，均無尾片；Planck逐列核對原文／行號／UTF-8 byte offset與JSONL差異0，serial SHA與各host保存值一致。boot JSONL沒有預解析fields，內容依原文判讀，主機收行UTC不是ADC時刻。

| 第02輪前檢／boot公開檔 | SHA256 |
|---|---|
| precheck serial.log | `E12FD450F13C2F81296CF88338869FE577755DFBBAB3F46ABEC6A475ABB6C437` |
| precheck lines.jsonl | `A1117B4EC4D4CCAA0C2984F21DD5D49781D4B6BB0DEE95A85A0645B8261C6670` |
| precheck host.json | `8B610873ADD34AA49DB2F80CFEA53F1199B42BF1E9A6B373D3523CAD4B80D598` |
| boot serial.log | `B426223EB8B965F1D5D4256FCED65EAB592DE506902C6C54D0293653F668A10E` |
| boot lines.jsonl | `F9A577888E15DB588202BEF501F4907EB52C690B200D743CCAF7DEBB460147C5` |
| boot host.json | `68EC5CC85202B8E8FCED03FAB26D6BE8AEBBC000271614CF6C5E188585FC56D6` |

前檢及boot擷取各已收尾；主代理其後取得同epoch真LINK、PCNT及完整bundle，結果另列如下，不回填為boot當時已完成。

## 第02輪DF497E3D：ANY接受異常字、LINK拒絕及完整bundle

- [第02輪bundle serial](20261028-hx711-hxpcnt-continuous-observe-140454.serial.log)
- [完整行JSONL](20261028-hx711-hxpcnt-continuous-observe-140454.lines.jsonl)
- [host設定、session guard及收尾](20261028-hx711-hxpcnt-continuous-observe-140454.host.json)
- [control原文](20261028-hx711-hxpcnt-continuous-observe-140454.control.log)
- [原analysis-v3](20261028-hx711-hxpcnt-continuous-observe-140454.analysis-v3.json)
- [修正analysis-v3.0.1](20261028-hx711-hxpcnt-continuous-observe-140454.analysis-v3.0.1.json)
- [Ampere本輪正式QC](../quality/reports/2026-10-28-pcnt1-continuous02-qc.md)

同一pcnt1版本、session `DF497E3D`，host expected／observed_session吻合、failure=null。本窗OPEN ACK為13:58:53.511，capture結束14:04:54.219；只送13:58:57.026 HX_DIAG STATUS及14:04:43.701 HX_DIAG DUMP。14:04:42.758先收到兩槽all_frozen1／pcnt_sealed1／reason1通知；完整外層BEGIN／END_HX_CAPTURE收行14:04:43.754～14:04:48.923，跨度5.169秒。

| 槽 | 保存與事件 |
|---|---|
| ANY | seq2303～2558共256列，127前＋trigger2430＋128後，accepted256、rejected0、JUMP4／post_complete1／overflow0。 |
| LINK | seq3322～3577共256列，127前＋trigger3449＋128後，accepted255、rejected1、LINK1／post_complete1／overflow0。 |
| 重疊／CRC | 兩槽無重疊，512列／512個不同attempt；逐列19-word CRC、HXF103-word、PCNT405-word及外層完整契約通過，非512個ADC字獨立驗真。 |

| seq／原始bits／RAW | 原接受與計數 |
|---|---|
| 2429，`0xFD8074`／−163724 | accepted1、RX2428→2429、ERR0 |
| **2430，`0xFFFD7F`／−641** | **accepted1、postcheck HIGH，RX2429→2430、ERR0，JUMP4** |
| 2431，`0xFD8003`／−163837 | accepted1、RX2430→2431、ERR0，JUMP4 |
| 3448，`0xFD7FF4`／−163852 | accepted1、RX3447→3448、ERR0 |
| **3449，`0xF7EC00`／−529408** | **accepted0、postcheck LOW，RX3448→3448、ERR0→1，LINK1** |
| 3450，`0xFD8058`／−163752 | accepted1、RX3448→3449、ERR1，已受診斷等待介入 |

14:02:37.251的STATUS亦印出RAW−641／RX2430／ERR0，異常被接受的失敗有原始逐讀與快照雙重保存，不修值、不判正常重量。兩個trigger read_end為266302824及377976872 boot µs，相差111.674048秒；CPU時刻不貼成精確主機UTC。不能由鄰值、後檢HIGH或下一筆回近鄰值定位根因。

## 真LINK後HXF與PCNT：有效窗SCK0／DATA4

本boot4個正對照seq1～4均valid，SCK有效計數各50，DATA／expected_data為7／7、9／9、9／9、9／9；setup_code1／clock_blocked0、proof4 good4 bad0 ready1，GPIO復原與正對照經Ampere核對通過。這是本epoch自己的證據，未沿用首輪proof。

HXF COMPLETE／captured1綁定seq3449、`0xF7EC00`／−529408、RX3448／ERR0→1，原拒絕保留。anchor read_end為**377976872µs**，probe_end378228522，hold251650µs；resume為seq3450／start378228684。8個有效離散DATA讀值均LOW，目標2／10／50／5000µs四點missed、時間與值NA，不補成LOW。首個有效目標200µs實際讀於anchor後378～383µs，最後250000µs目標讀於251645～251650µs；兩點間電位未知。

PCNT linked1、failure_seq3449，兩unit API mask15／valid1／overflow0。下列括弧為相對anchor的CPU API呼叫時間（µs），不是管腳波形：

| HOLD計數unit | START括弧 | STOP括弧 | valid_count／首末讀值 |
|---|---|---|---|
| SCK | 296～334 | 251765～251767 | 0；首末皆LOW |
| DATA | **343～346** | 251669～251699 | **4**；首末皆LOW |

SCK START已完成才開始DATA，DATA STOP已完成才停止SCK，保守API括弧仍支持SCK觀察時窗包絡DATA。這是**可信MCU數位計數窗內SCK0／DATA4**，DATA開始前343～346µs保留早期盲區；4是邊緣總數，沒有各edge時刻、方向或脈寬，不能直接換成幾個HIGH事件、證HX端類比／供電原因，亦不能以離散LOW推全程連續LOW。未觀察到SCK數位edge不等於獨立證明HX端絕無有效clock。

PCNT記的是原拒絕後等待窗，不是異常讀框全程；hold／API會改變後續排程，`low_hold_intervention_applied=true`／timing=true。有效PCNT與HXF／LINK綁定、CRC可信，不代表RAW取得正確或366g校正可開始。HXF CRC `0x19E8B0B0`、PCNT405-word CRC `0x4E0D8C75`與原文吻合。

## 離線analysis-v3.0.1修正與保存

原[analysis-v3](20261028-hx711-hxpcnt-continuous-observe-140454.analysis-v3.json)將啟動期PROOF的elapsed_start_us誤當相對晚期LINK anchor，形成無號回捲大數；保留原檔不覆寫。Gibbs只修正離線scope／時間呈現，未改FW或重燒。

Planck比較完整JSON：差異限工具版本、8個PROOF unit的elapsed_start_us改null及新增PROOF／HOLD scope標記；原始boot時間、HOLD時間／count、READ／HXF、CRC均不變。從原serial重跑v3.0.1 exit0，與[修正analysis-v3.0.1](20261028-hx711-hxpcnt-continuous-observe-140454.analysis-v3.0.1.json)全物件一致。部署時工具hash留在前表，修正工具另列如下，不把呈現修正當HX根因修復。

## 第02輪STATUS、啟動與DUMP條件

本窗355 STATUS／355不同RX397→3631，收行13:58:54.401～14:04:54.074；RAW平均−163279.2986、範圍−163884～−641 counts，均值包含已接受異常值，不用來稱重。全窗IDLE／UNCAL、AERR0、ERR0→1；約1Hz快照不是全部讀框或ADC時刻。

boot於13:58:10.7383406先印OLED MISSING，13:58:14.1417983後印ACK=0x3C，ACK不代替目視。啟動FIFO1→2已在13:58:14.2646665～15.2586260觀察到，與較後的DUMP分開。同步匯出後14:04:49.057為HX STALE／AGE5320ms、ADXL MISSING／FIFO2，14:04:50.059近期OK並FIFO升3；這是封存後DUMP阻塞條件，不回因至ANY或LINK，也不以DUMP解釋尚未DUMP的啟動增加。警示來源／漏樣量及與HX是否同因仍未定位。

## 捕獲後零TX 90秒與完整性

- [後90秒serial](20261028-hx711-pcnt-event-postdump-passive-141043.serial.log)
- [完整行JSONL](20261028-hx711-pcnt-event-postdump-passive-141043.lines.jsonl)
- [host事件與最終保存數](20261028-hx711-pcnt-event-postdump-passive-141043.host.json)
- [control原文](20261028-hx711-pcnt-event-postdump-passive-141043.control.log)

零裝置TX，capture14:09:12.906～14:10:43.369、90.463秒；90 STATUS收行14:09:13.027～14:10:42.379，90不同RX5997→6814，IDLE／UNCAL／HX及ADXL OK、ERR1／AERR0／FIFO3持平。RAW平均−163773.7222、範圍−163869～−163662 counts；未再查版本／session，依操作追溯承接本epoch，與前窗有缺口且非逐框，不判修復或精度。

bundle179320 bytes／1309完整行、無尾片；後窗最終29796 bytes／180完整行及**96-byte尾片**，尾片原樣保留，不補行、不納90 STATUS。後窗PASSIVE_CAPTURE_END快照為29764 bytes，至關閉多收32 bytes；最終host stored_bytes／raw／SHA均29796，事件快照與最後檔長分列，不把32 bytes當新完整行或遺失資料。Planck核對兩窗原文／行號／UTF-8 offset／STATUS已解析欄位與JSONL差異0；boot三檔hash亦與先前保存一致。

| 第02輪bundle／分析與後窗 | SHA256 |
|---|---|
| bundle serial.log | `3AB0F921F4732C305120F03ACCCA6E93D220143A3F9EBBA2B485DA29B8E9383E` |
| bundle lines.jsonl | `1C62AFF12EBC3BE4EF9440A10DBE2002CBDA59CF41FE792916D8319DFC468106` |
| bundle host.json | `C32DB68F1F4B64BF60E57D55D7D2E5068530CAE6D79E0B402679B5CC987DB6DC` |
| 原analysis-v3.json | `BDA9F714BFFAB9D2763C9078CFBAAD38E2F4B7ED76FAD28ECD8DDDC196F96377` |
| analysis-v3.0.1.json | `61A34F827BB91258C06CE1B262AD9FB880BA58BB6350F311437E3FB508EBD778` |
| 修正failure解析器 | `F56C7838F15CA05FA3CB03C22E01A8649602CECAAFFAAD21C7F5EC9645FF8DC1` |
| 修正PCNT解析器 | `64B43AB135EB0E4178EFCAF55441C26BE64CB9D5FCC0FDCD92E1AFD52A377F85` |
| 後窗serial.log | `CC3E8A9969B7A27B3656DD0240BEC36DA1DAE34E104CE3A48E222EE09782589C` |
| 後窗lines.jsonl | `3990F5DE7E07DF7B44E7DAA50C90BD70DDFB987FBF919BC87B641414E2A16C91` |
| 後窗host.json | `61170DE122028F4EED7B18608C6653F353A9BCC0B95A81B1815AEEE8F8CD0B07` |
| 兩窗control.log各自相同 | `20C89E60163C5035CC067814700B842114C0904728EDC0C73935360346B806A5` |

[Ampere正式第02輪QC](../quality/reports/2026-10-28-pcnt1-continuous02-qc.md)確認GPIO／proof、512列＋103／405-word CRC、時窗及真LINK數位捕獲可信；Q04新accepted−641與既有量測失敗仍阻擋CAL，根因未定。PCNT事件目標已達，主代理已停止循環reset，保持USB插接／供電與UNCAL。各boot、bundle及後窗已關閉，monitor CLOSE／QUIT成功、process exit0、overflow=false／failure=null，COM3已釋放；未TARE／CAL、新flash、commit／push。

Gibbs正在設計每次讀取的PCNT附加記錄（每框sidecar），**尚未正式編譯、燒錄或於實板執行此後續方案**，不填新邊緣、波形或修復結果。Planck只更新本文件，不開COM、讀秘密或改handoff／QC／韌體。

## 每框PCNT設計期間：5分鐘零TX快照及缺口新增ERR

- [被動serial原文](20261028-hx711-pcnt-frame-design-passive-142706.serial.log)
- [完整行JSONL](20261028-hx711-pcnt-frame-design-passive-142706.lines.jsonl)
- [host設定、計數及收尾](20261028-hx711-pcnt-frame-design-passive-142706.host.json)
- [control原文](20261028-hx711-pcnt-frame-design-passive-142706.control.log)

此窗仍為已部署pcnt1的被動STATUS觀察，不是每框sidecar實板結果。capture14:22:04.952～14:27:05.955，實際301.003秒；299 STATUS收行14:22:05.874～14:27:04.958，299不同RX13079→15815。全窗IDLE／UNCAL／HX及ADXL OK、ERR3／AERR0／FIFO3持平，零裝置TX；RAW平均−163829.4983、範圍−163982～−163732 counts。

前一被動窗末14:10:42.379為ERR1，本窗首14:22:05.874已為ERR3。兩個累計錯誤增加只在未監看缺口中被發現，無法定位各次發生時間、RAW或原因；HX OK與本窗ERR3持平不能否定新增錯誤。ring早已凍結，沒有這兩次新錯誤的逐框／PCNT事件，不補造紀錄，也不歸因設計工作或先前DUMP。

serial最終99065 bytes、598完整行及**96-byte未完成尾片**，與host／END事件計數一致；尾片保留原樣，不補行、不納299 STATUS統計。Planck逐列核對原文／行號／UTF-8 byte offset／STATUS已解析欄位與JSONL差異0，serial SHA與host一致。

| 公開檔 | SHA256 |
|---|---|
| serial.log | `8764DDCB2C40127D460B3639F6957592D666DBCBB7D7D7C09CC5B54651969D93` |
| lines.jsonl | `99AC569CF0529982EDB8AC724F90A96D40504EE5D027ECE8CAA891D33C3E7D7E` |
| host.json | `B8C3454F2EA08EBFF4E24A0C700FBFD10FCFDD2381F3859CF116A8D271E100BB` |
| control.log | `20C89E60163C5035CC067814700B842114C0904728EDC0C73935360346B806A5` |

約1Hz快照、非逐框，未再查版本／session，只依操作追溯承接既有epoch；不能由此解除accepted異常、判修復或進CAL366。CLOSE／QUIT成功、process exit0、overflow=false／failure=null，主機確認該窗COM3已釋放。每框sidecar仍在設計、未正式編譯／燒錄；保持供電／UNCAL，根因及校正未解，未TARE／CAL、commit／push。Planck此次僅更新本文件，未開COM或改其他檔。

## 設計期間新增快照異常：RAW−1282／ERR3未增加

- [5分鐘被動serial](20261028-hx711-pcnt-frame-design-passive-145034.serial.log)
- [完整行JSONL](20261028-hx711-pcnt-frame-design-passive-145034.lines.jsonl)
- [host設定、計數與收尾](20261028-hx711-pcnt-frame-design-passive-145034.host.json)
- [control原文](20261028-hx711-pcnt-frame-design-passive-145034.control.log)

capture14:45:33.593～14:50:34.164，實際300.571秒、裝置零TX。299 STATUS／299不同RX25977→28716，收行14:45:34.074～14:50:33.169；全窗IDLE／UNCAL／HX及ADXL OK，ERR3／AERR0／FIFO3持平。全STATUS RAW範圍−164145～−1282、平均−163463.9331 counts，平均包含異常值，不當空載基線或重量精度。

| 主機完整行時間（台北24小時制） | RAW／RX | ERR／HX／AGE_ms |
|---|---|---|
| 14:46:30.283 | −164000／26491 | 3／OK／167 |
| **14:46:31.285** | **−1282／26498** | **3／OK／250** |
| 14:46:32.285 | −164028／26508 | 3／OK／143 |

兩側數值算術右移7位均為−1282，僅是事後數值候選，不證丟7位、實際clock／DATA行為或可還原RAW。相鄰快照RX增加7及10、該筆AGE250ms只是伴隨資料，不推ADC漏樣量或因果。**HX OK與ERR3未增不能將−1282判為有效重量或資料品質已修復。**

ring及失敗後PCNT快照早已凍結，本次沒有這個新異常的逐框原始bits、accept／postcheck、LINK綁定或事件邊緣窗。約1Hz STATUS不是逐框，不能由前後兩個正常近鄰快照補造其間ADC框，亦不把舊LINK3449的PCNT數據套到這次異常。主機收行UTC不是精確異常發生時刻；原文原值保留，不修補或替換。

serial99095 bytes、598完整行及**128-byte未完成尾片**，與host最終保存／END計數一致。Planck逐列核對原文／行號／UTF-8 byte offset／STATUS已解析欄位與JSONL差異0，serial SHA與host一致；尾片保留不補行、不納299 STATUS統計。

| 公開檔 | SHA256 |
|---|---|
| serial.log | `749B4CEAEDE14D58DEA972E3C1FD70191B7CA055F8BF6A9EC9ABDBEF818565E1` |
| lines.jsonl | `655CCDA04EC6EC20BFF71BA0E4C20C31A318BCD251EA62C1B50EEEDE4E0BD6BD` |
| host.json | `AEDC693B0FA37204C1A47E3620CFA01C3E0A1D9EECAB8ED6043F6A24D8CA0373` |
| control.log | `20C89E60163C5035CC067814700B842114C0904728EDC0C73935360346B806A5` |

本窗CLOSE／QUIT成功、process exit0、overflow=false／failure=null，主機確認COM3已釋放；當時獨立QC進行中，之後Ampere已在[同版正式報告](../quality/reports/2026-10-28-pcnt1-continuous02-qc.md)追加原文／時間／保存核對，仍保留−1282、缺口與量測品質失敗，不補逐框或修RAW。每框sidecar仍在開發、未READY，不把本次被動窗當該方案實板結果或宣稱完成。保持供電／UNCAL、根因與366g校正未解，未TARE／CAL、新flash、commit／push；Planck此次僅更新本文件，未開COM或改其他檔。

## 設計期間後續5分鐘零TX窗：14:55～15:00

- [被動serial原文](20261028-hx711-pcnt-frame-design-passive-150003.serial.log)
- [完整行JSONL](20261028-hx711-pcnt-frame-design-passive-150003.lines.jsonl)
- [host設定、計數及收尾](20261028-hx711-pcnt-frame-design-passive-150003.host.json)
- [control原文](20261028-hx711-pcnt-frame-design-passive-150003.control.log)

capture14:55:03.124～15:00:03.267，實際300.143秒、裝置零TX。298 STATUS／298不同RX31192→33920，收行14:55:04.138～15:00:02.273；全窗IDLE／UNCAL／HX及ADXL OK、ERR3／AERR0／FIFO3持平。RAW平均−164121.4463、範圍−164241～−163982 counts；297對相鄰快照最大絕對差212 counts，未見≥1000 counts跳變，1000僅沿用異常檢索門檻、不是量測驗收標準。

本窗約1Hz、非逐框，未查版本／session，依既有操作追溯而非重新識別。ring早已凍結，快照未再見大跳或ERR增加不能解除先前−1282、舊accepted異常或根因，亦不能補驗未觀察到的ADC框／PCNT邊緣。

serial最終98998 bytes、598完整行及**96-byte未完成尾片**，與host／END事件計數一致。Planck逐列核對原文／行號／UTF-8 byte offset／STATUS已解析欄位與JSONL差異0，serial SHA與host一致；尾片保留不補行、不納298 STATUS統計。

| 公開檔 | SHA256 |
|---|---|
| serial.log | `1855C0008C496E2D8AD16B264FD769CEF7C3CEB5238492EB9CDF505A059B8A47` |
| lines.jsonl | `284F3A41022226495F0E2F17D5BBAB5A465CEDB80036000121DB164437333828` |
| host.json | `A87033961DBD5FE778B7EE0F7671E35703AE450D9E9CE45FE51FDE991611CAF4` |
| control.log | `20C89E60163C5035CC067814700B842114C0904728EDC0C73935360346B806A5` |

CLOSE／QUIT成功、process exit0、overflow=false／failure=null，主機確認該窗COM3已釋放。保持供電／UNCAL，既有−1282及366g校正仍未解；不把此窗當每框sidecar已部署或修復證據，未TARE／CAL、新flash、commit／push。Planck本次僅更新本文件，未開COM或改其他檔。
