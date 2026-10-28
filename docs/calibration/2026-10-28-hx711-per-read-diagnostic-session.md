# 2026-10-28 HX711逐讀診斷部署session

日期／時區：2026-10-28，Asia/Taipei（UTC+08:00）。主代理為唯一COM操作者，Gibbs負責新韌體，Ampere負責成功燒錄後獨立QC，Planck整理公開證據及追溯。主機測試、正式編譯／寫入、實板功能及量測資料品質分開判定。

**hxread1已正式編譯、正確目標四區寫入校驗並重啟；新session為238002AF，仍UNCAL。** Ampere判限定IDLE唯讀及事件凍結／匯出子項通過，量測資料品質未通過：逐讀資料保存拒絕字−317，隨後卻接受巨大異常字−4718593。CRC通過不抵消該失敗；原−594／−156與本次是否同根因未知，366g校正仍未完成，不沿用舊offset／scale。

## 授權與epoch界線

本次後續授權允許為逐讀診斷重啟／燒錄，優先於[既有校正紀錄](2026-10-27-hx711-authorized-auto-calibration.md)與[離線候選設計](2026-10-28-hx711-per-read-diagnostic-design.md)中當時「保持執行狀態、不部署」的歷史限制；不改寫那些已保存的操作事實。

保持USB／外部供電不代表執行中RAM能跨重啟保留。本次寫入後重啟已另立執行epoch，校正、RX／ERR／AERR及診斷序號分開記錄；不把新計數歸零當成舊異常修復，不跨epoch合計增量或沿用歷史offset／scale。執行中HX_DIAG STATUS及DUMP均讀回版本`v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1`與session `238002AF`；session是隨機執行識別，不是永久唯一ID。自然boot banner、reset原因及HX_DIAG_READY未捕獲，保留待補，不為補畫面額外reset。

下節燒錄前檔屬**部署前舊epoch觀察窗**，擷取識別為`20261028-hx711-before-per-read-flash-085733`；這是主機檔案識別，不是裝置匯出的boot ID。該窗沒有逐讀／失敗嘗試原始位元或SCK管腳波形，不能把STATUS稱為逐框記錄；其後hxread1資料另依新session分開。

## 燒錄前零TX被動證據

- [host設定、控制事件及收尾](20261028-hx711-before-per-read-flash-085733.host.json)
- [serial原文](20261028-hx711-before-per-read-flash-085733.serial.log)
- [完整行接收JSONL](20261028-hx711-before-per-read-flash-085733.lines.jsonl)
- [控制通道原文](20261028-hx711-before-per-read-flash-085733.control.log)

目標COM3 USB-SERIAL CH340，serial-monitor v0.15.0（commit 13b8e80），OPEN前115200／8N1、DTR／RTS off。主機控制通道有設定與OPEN／CLOSE等事件，**裝置命令TX為空**；沒有TARE／CAL、重置、WiFi切換或燒錄。

| 項目 | 實際觀察 |
|---|---|
| PASSIVE_CAPTURE_START／END | 08:56:03.448～08:57:33.806，實際90.358秒 |
| 完整STATUS接收時間 | 08:56:03.467～08:57:32.820 |
| STATUS／不同RX | 90／90，RX483012→483830 |
| RAW平均（counts） | −161802.4111 |
| RAW最小～最大／跨度 | −161906～−161726／180 counts |
| phase／CAL／近期感測器狀態 | IDLE／UNCAL／HX OK／ADXL OK |
| ERR／AERR／FIFO_FULL | 37／2／0，全窗未觀察到增加 |
| AGE_ms／WiFi | 124～213／CONNECTED |
| 相鄰快照掃描 | 89對，最大絕對RAW差123 counts，沒有≥1000 counts跳變 |

JSONL時刻是主機收到完整行的時間，不是ADC轉換或真實錯誤瞬間。約1Hz STATUS只是快取快照；不同RX不代表已觀察全部ADC轉換。這90筆未見大跳及計數增加，不能排除快照間其他框異常、推SCK脈寬、估漏樣率或判舊問題已解。UNCAL資料只報counts，不套歷史校正換算重量。

## 燒錄前檔保存核對與關埠

Planck核對serial最終30002位元組，與host的received／stored／結束事件計數一致；180完整行與JSONL逐行`raw_line`一致、差異0，含90 STATUS與90 WiFi。另**32位元組不完整尾片段**原樣保留，不補字、不納90筆或相鄰差統計。整個原始serial的SHA256與host吻合：

`DC76973D91BD148326A868D070BEE9F5EDBB5AC894386A2E23BF06036BF87A34`

host failure=null、overflow=false；CLOSE／QUIT ACK均OK、process exit0、COM3已正常釋放，沒有背景持有或常駐監控。這是保存／通訊收尾核對，不是逐讀診斷功能、硬體時序或量測精度通過。

## 正式編譯、目標與寫入校驗

依使用者授權，由主代理正式編譯、識別目標及燒錄；Planck沒有讀BIN／ELF或含憑證生成來源。編譯exit0及上傳exit0由主代理工具交付，原始公開證據及[Ampere獨立QC報告](../quality/reports/2026-10-28-hxread1-qc.md)分開保存：

| 項目 | 實際證據與結果 |
|---|---|
| 主機回歸 | Gibbs交付239項Node及10項原生C++檢查通過；不是實板QC，Planck未重跑整套回歸 |
| 正式編譯 | [compile log](20261028-hxread1-compile.log)：966941／1310720 bytes程式、107464／327680 bytes靜態全域，剩餘220216 bytes是編譯容量摘要，不是實測heap |
| 建置設定 | 主代理／QC交付Arduino CLI 1.5.1、ESP32 core 3.3.12、FQBN `esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40` |
| 目標識別 | [identity log](20261028-hxread1-identity.log)：COM3、ESP32-D0WD-V3 revision v3.1、MAC e0:8c:fe:a8:42:bc，與[upload log](20261028-hxread1-upload.log)一致 |
| 寫入／校驗 | 0x1000、0x8000、0xE000、0x10000四區各寫入並取得Hash of data verified，最後Hard resetting via RTS及COM3返回；這次重啟屬已授權燒錄流程 |

Planck重新計算下列公開來源SHA，與QC交付一致；它們識別公開來源，不是實板flash映像hash，也不涵蓋私密設定：

| 公開來源 | SHA256 |
|---|---|
| [05-test.ino](../../05-test/05-test.ino) | `A97652404C5A899E2E4ACF2B2593E8168DE7CE4D5E104062AA106AA64A6DA768` |
| [hx_read_diag.h](../../05-test/hx_read_diag.h) | `0B50F6C10D928EB97A74934250DFF0025BFC70B2E7291DE6D954D4ABC4B0E564` |
| [manual_optical.h](../../05-test/manual_optical.h) | `71782ABCDFD85C74DC2D5B7EB3BED7645B9A8E80394969A864854B1FEFF254B1` |
| [network_wifi.h](../../05-test/network_wifi.h) | `53F1C85E2E25010F4852C69828D805F968F5231976CC7CC7BFD9B5C5C244615F` |

公開compile／identity／upload log的SHA依序為`2005918F75F9CF91E920C83A3C4DB6E3359D1881EE24164DBCDED2F7A1D05B47`／`CBB61F9203655173BD6BD8BC7A129015B54D8B38115B0066B2DEEA783A448A57`／`307F958B84B1ED9EE3A45CB6080E8BF6A3F9C2D5122897D38DF737D82BFFDD43`，供公開檔案追溯；未提交私有建置產物。

## 新epoch：燒錄後限定唯讀QC

[QC serial](20261028-hx711-per-read-postflash-qc-091637.serial.log)、[JSONL](20261028-hx711-per-read-postflash-qc-091637.lines.jsonl)、[host](20261028-hx711-per-read-postflash-qc-091637.host.json)記錄09:16:04.807～09:16:37.179，共32.372秒。主代理成功燒錄後立即交Ampere獨立QC；本窗8條唯讀命令為STATUS、ADXL_DIAG、HELP、WIFI STATUS、HX_DIAG STATUS、STATUS、ADXL_DIAG、HX_DIAG STATUS。

34個不同RX239→532，均IDLE／UNCAL／HX OK／ADXL OK、ERR0、AERR0，FIFO_FULL1持平；RAW平均−162061 counts、範圍−162169～−161959，WiFi CONNECTED。ADXL快取ACC_RX2815→5229，FULL_OBS1、DEPTH_OBS0、OVR_OBS1、DIAG_ERR0不增；警示已存在，不能稱動態無漏樣。HX_DIAG兩次均為schema1、session238002AF、LIVE、capacity256／record_bytes56，attempts309→531；records127、pre_overwritten182→404是正常ring滾動保留，不是ADC漏樣數。

執行中版本、更新與唯讀回應子項通過；自然boot／reset原因／HX_DIAG_READY及新版OLED／RGB同窗目視仍缺。完整判定與缺項以Ampere報告為準，不將短窗ERR0視為舊故障修復。

## 同session逐讀事件：拒絕後仍有被接受的巨大字

[observe serial](20261028-hx711-per-read-observe-091925.serial.log)、[JSONL](20261028-hx711-per-read-observe-091925.lines.jsonl)、[host](20261028-hx711-per-read-observe-091925.host.json)為09:17:08.337～09:19:25.444，共137.107秒。只送HX_DIAG STATUS及HX_DIAG DUMP各一次。09:19:17.422先收到FROZEN通知，09:19:17.917才送DUMP；BEGIN／END完整行於09:19:17.961／09:19:20.315收到，沒有強制凍結或清除資料。

[公開離線分析結果](20261028-hx711-per-read-observe-091925.analysis.json)為單一capture陣列；Planck以[公開解析器](../../tools/hx-read-diagnostic.cjs)重新解析原文，exit0、CRC及契約檢查通過，輸出與保存JSON一致。schema1、同版本／session238002AF、seq1795～2050共256筆，127前史＋觸發1922＋128後史；freeze_reason1、post_complete1、overflow0、seq_wrapped0。

| attempt seq／serial行 | bits24／RAW | accepted／DATA後檢 | RX前→後 | ERR前→後 | flags | CPU括弧（µs）／耗時 |
|---|---|---|---|---|---|---|
| 1921／398 | FD86A7／−162137 | 1／1 | 1920→1921 | 0→0 | 0 | 204540454～204540649／195 |
| **1922／399** | **FFFEC3／−317** | **0／0** | **1921→1921** | **0→1** | **LINK=1** | 204722499～204722715／216 |
| **1923／400** | **B7FFFF／−4718593** | **1／1** | **1921→1922** | **1→1** | **JUMP=4** | 204761482～204761697／215 |
| 1924／401 | FD8684／−162172 | 1／1 | 1922→1923 | 1→1 | JUMP=4 | 204816454～204816653／199 |

1922的原DATA後檢拒絕，未覆寫有效RAW／RX；下一次1923後檢HIGH卻仍接受巨大異常字，1924再回近鄰值。不能只報255 accepted／1 rejected而忽略1923，也不能將accepted等同已證實正確ADC字。約1Hz STATUS沒有印到該巨大接受字；它在09:19:03.887只顯示HX LINK_ERR、RX1921、ERR1及先前有效RAW−162137，故STATUS無大跳不代表逐讀無大跳。

256列CRC32（19個little-endian uint32）全數通過、無缺／重排record。CRC在凍結匯出時計算，只支持資料形成後匯出完整性，不是ADC自帶CRC，也不證明GPIO取得字或匯出前RAM正確。bits24與RAW同源，一致不是獨立真值；[協定與欄位限制](2026-10-28-hx711-read-diagnostic-protocol.md)另載。

CPU時間為boot低32位micros括弧，不是GPIO波形或ADC轉換時刻。保留窗首start191172503、觸發end204722715、freeze／末筆end218266720µs，保存跨度27.094217秒、後史13.544005秒；不把DUMP匯出host UTC貼回每個attempt。全窗括弧195～238µs也不能證明各SCK high／low符合規格，25個軟體脈衝不證明25個有效物理邊緣。

匯出時attempts2056=pre_overwritten1794+records256+omitted_after_freeze6；overflow0僅表示沒有非預期ring容量異常。滾動捨棄及凍結後省略是診斷記錄範圍，不是ADC漏樣量；凍結後原HX輪詢仍繼續。

### 受限序列模型的界線

Gibbs回報一個數值相容的受限假說：以未量測的候選字FD86B7（−162121）作輸入，首取HIGH後9個clock位置DATA不前進，可形成FFFEC3並在後檢LOW時拒絕；下一次把剩餘B7及尾端HIGH串接，可形成B7FFFF並通過後檢。模型簡化了多餘clock、轉換及恢復機制，另有不同分段候選，**不能將9稱為實測錯位次數，不能把候選字當作還原真RAW或根因證明**。

分析JSON的candidates空陣列，是工具對accepted序列作雙鄰右移篩選的結果，不排除涉及rejected字的其他受限序列模型。原−594／−156與本次拒絕／巨大接受字是否同根因未知；未據模型左移修值、濾除、改解碼或放寬接受門檻。

## DUMP干擾與匯出後90秒被動窗

DUMP標示output_blocks_polling=1，同步大量輸出阻塞輪詢。END後09:19:20.453 STATUS顯示HX STALE／AGE2497ms、ADXL MISSING、FIFO_FULL1；09:19:21.459恢復近期HX／ADXL OK，FIFO_FULL升至2，ERR仍1、AERR仍0。此現象與匯出阻塞契約相容，未作獨立硬體因果驗證；它不改較早已凍結的256列，但匯出後間隔不能當無輸出的自然排程，也不能抹除第二個FIFO警示。

另存[postdump serial](20261028-hx711-postdump-passive-092158.serial.log)、[JSONL](20261028-hx711-postdump-passive-092158.lines.jsonl)、[host](20261028-hx711-postdump-passive-092158.host.json)。空載被動09:20:27.544～09:21:58.023，實際90.479秒，無裝置TX；90個不同RX2672→3484，STATUS接收09:20:27.712～09:21:57.039。均IDLE／UNCAL／HX OK／ADXL OK，ERR1、AERR0、FIFO_FULL2本窗不增，WiFi CONNECTED；RAW平均−162122.5444、範圍−162224～−162032 counts。這仍是快照，不是凍結後每框匯出；計數不增不抵消事件或FIFO2，也不恢復366g校正。

## 最後唯讀ADXL快取與未記錄間隔內新增ERR

另存[09:30 cache serial](20261028-hx711-postdump-adxl-cache-093022.serial.log)、[JSONL](20261028-hx711-postdump-adxl-cache-093022.lines.jsonl)、[host](20261028-hx711-postdump-adxl-cache-093022.host.json)。實際接收09:30:11.811～09:30:22.525，只送STATUS及ADXL_DIAG；12個不同RX8008→8100，完整STATUS時間09:30:11.927～09:30:21.974，均UNCAL／HX OK、ERR2、AERR0、FIFO_FULL2，本窗不增。前一被動窗末09:21:57.039為ERR1，本窗首已ERR2，新增發現於未記錄間隔，不能定時刻／原因，也不能歸因09:19 DUMP。首筆ERR2在本窗命令送出前，沒有將它認作本次查詢造成。

| 最新ADXL快取 | 實際值與界線 |
|---|---|
| FULL_OBS／DEPTH_OBS／OVR_OBS／DIAG_ERR | 2／1／2／0，開機累計觀察計數，不是漏樣數 |
| FIFO_DEPTH_MAX／POLL_GAP_MAX | 32／2521078µs；不是HX SCK高電位時間 |
| LAST事件 | CPU_US221329074、N32、SRC0x83、phase0 |
| FIRST事件 | CPU_US516804、N4、SRC0x81、phase0 |
| 初始化快照 | PRE_N32／SRC0x83、POST_N0／SRC0x81，非同步 |

依Gibbs核對，第二警示來源的快取已取得，顯示深度32並帶OVR來源旗標；與同步DUMP約2.354秒輸出及其後長輪詢間隔條件相容，不再列為「未取得第二警示來源」。這仍不是實測物理因果或漏樣量，亦不能與缺口新增HX ERR2混為同一原因。

初始化PRE快照取於adxl.begin／resetFifo之前，N32／SRC0x83已存在；POST為0／0x81、後續FIRST為4／0x81。來源旗標不能直接當作MCU新epoch內新發生的物理OVR證明，啟動附近與匯出後情況分別追蹤。Planck核對4499位元組、32完整行、無尾片，原文／JSONL差異0、SHA與host一致；CLOSE／QUIT、exit0及COM釋放成功。

## 保存雜湊、QC分工與目前狀態

Planck核對新四組serial與JSONL為94／538／180／32完整行，逐行raw_line差異0，位元組與host一致、均無尾片；各整檔SHA如下。analysis JSON含一個capture，其SHA另列：

| 公開證據 | bytes／完整行 | SHA256 |
|---|---|---|
| postflash-qc-091637 serial | 14098／94 | `790D9F027E1CB23CA0D2CA120A7A5A35C8227A26504F78DF442D7DC1D197AF8C` |
| per-read-observe-091925 serial | 72691／538 | `833E2FA3A27D08B43A712BE050842C8687DD64771E5BDC6C933447F88E6B1C1B` |
| postdump-passive-092158 serial | 29700／180 | `F9E32450DC29A6C78A68B96EF8A88A226FF36E948275AE3FC2CAA24057566B62` |
| postdump-adxl-cache-093022 serial | 4499／32 | `1557BDAC30AFF539C5D55450ECD84A54EC4B2E9CC71A12D24E2879EE8CA15E19` |
| observe analysis JSON | 離線摘要 | `31A551B7E53D7A15CCBD286ECA676A030730CEC08FE865B172712BB7535794FA` |

原燒錄前舊epoch的32-byte尾片繼續保留，不與新epoch資料拼接。各host failure=null、overflow=false，CLOSE／QUIT成功、process exit0、COM3已釋放；沒有背景持有或常駐監控。完整QC分工以Ampere報告為準：

| 追蹤 | 結論／責任 |
|---|---|
| HXREAD1-Q01、Q04 | 自然boot及新版目視待補；主代理／Planck追溯，不額外reset補畫面 |
| HXREAD1-Q02 | 量測資料品質未通過；Gibbs與Kierkegaard依逐讀證據縮小原因，不能用CRC／一窗無錯或修值關閉 |
| HXREAD1-Q03 | 初始化前來源旗標、啟動附近警示與DUMP後FIFO1→2分條件追蹤；第二警示已取得N32／SRC83快取，Gibbs／主代理仍不推物理OVR時刻、漏樣量或單一根因 |
| HXREAD1-Q05、Q06 | 未觸發路徑及366g／241g、回零／重放／量測驗收保留未測；後續新版本需重新交Ampere QC |

| 項目 | 本次更新時狀態 |

| 項目 | 本文件建立時狀態 |
|---|---|
| 使用者重啟／燒錄逐框診斷授權 | 已提供 |
| 燒錄前舊epoch被動證據 | 已保存及核對，COM已關閉 |
| 新韌體版本／公開來源與建置 | hxread1正式交付／編譯成功；公開來源雜湊已核對 |
| 本次新韌體燒錄 | 正確目標四區寫入校驗成功，已依授權重啟 |
| 新epoch自然啟動／實板版本與逐讀資料 | 執行中版本及session238002AF已確認；自然啟動待補，事件256列已保存 |
| 新版燒錄後QC | Ampere已依[QC清單](../quality/post-flash-qc.md)完成報告；限定功能／匯出子項通過，量測資料品質未通過，缺證另列 |
| 366g校正與量測驗收 | 未完成；不由新部署或計數重置自動通過 |

本文只更新本session，未開COM、未讀秘密或含憑證建置產物、未使用OLED、未改handoff／韌體／QC報告或歷史原始證據，未commit／push。燒錄重啟與其後無額外reset分開記錄；新boot保持UNCAL，待根因及必要修復／驗證，未完成366g校正。

## 最後補記：09:31～09:33零TX被動快照

[最後serial](20261028-hx711-postdump-passive-093308.serial.log)、[JSONL](20261028-hx711-postdump-passive-093308.lines.jsonl)、[host](20261028-hx711-postdump-passive-093308.host.json)記錄09:31:37.781～09:33:08.602，實際90.821秒；裝置TX為空。90筆不同RX8796→9612，完整STATUS接收09:31:38.256～09:33:07.623，均UNCAL／HX OK／ADXL OK、ERR2、AERR0、FIFO_FULL2，本窗未觀察到計數增加。RAW平均−162244.6889 counts，範圍−162331～−162180、跨度151 counts。

這是約1Hz快取快照，不是凍結後每個讀框，不能排除未輸出的異常或判量測品質已恢復。09:21:57末ERR1至09:30首ERR2的既有缺口仍然未知，不能用這最後90秒計數持平回填其增量時刻／原因或稱全程連續觀測。

Planck核對29700位元組、180完整行與JSONL逐行一致、差異0，無不完整尾片，整檔SHA256與host吻合：`38C1B8225D5C89294B4E4544D948B1F0C6208C27158BC5E06E7A1EC76375ECD6`。host failure=null、overflow=false，CLOSE／QUIT成功、process exit0，COM已釋放；未TARE／CAL或重置，保持供電與UNCAL，366g校正仍未完成。
