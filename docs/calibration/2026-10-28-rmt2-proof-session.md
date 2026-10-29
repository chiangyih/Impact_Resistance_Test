# 2026-10-28 RMT2 首次 bad／good proof 診斷 session

日期／時區：2026-10-28，Asia/Taipei（UTC+08:00）。主代理唯一操作COM3；Gibbs負責公開候選與host工具，Kierkegaard核對物理／時序限制，Ampere獨立QC，Planck整理公開證據。

> **最終交接：RMT2部署及完整封存子項通過，診斷用途僅條件通過，量測品質未通過。** session `437B6829`自然600s到期、schema6／全部CRC與127唯一RAW／sidecar已核；初期4good／0bad後proof停止，後續127個accepted仍有4框DATA條件模型+2。FIRST_LINK／BAD未觸發，不解除RMT1失敗；FIRST_GOOD seq1原proof valid1，但RMT reason512／DATA formatfalse，由終止marker後未定義半word觸發，原樣保留，不解釋成HX電氣異常。19:45:00.079 ERR0至19:48:44.853已ERR1屬未擷取缺口，沒有當框資料。最後90s快照ERR1持平，不能判修復。全部capture已結束，COM3於19:50:15.153完成釋放，USB保持供電／UNCAL；當下載重未知、CAL366未執行、未commit／push。本輪只補格式限制文件，不再改韌體／flag或重啟。根因仍待可信的HX端同步SCK／DOUT及供電證據。下方較早pending狀態保留為歷史；完整結果見末節及 [Ampere最終QC](../quality/reports/2026-10-28-rmt2-qc.md)。

## 授權、目的與先前失敗

使用者已授權修正逐框DRAM後編譯、成功後自動燒錄與QC，保持供電、記錄RAW與時間、排查HX，釐清原因後完成366g校正，不再詢問操作同意。此文件只保存主代理交付的公開證據與候選狀態，不自行擴展成自由落下、材料驗收、清警示或猜測載重。

[RMT1 session](2026-10-28-rmt-diagnostic-session.md)及 [RMT1 QC](../quality/reports/2026-10-28-rmt1-qc.md)已記錄PCNT正對照8次7good／1bad／ready0：seq3 accepted `FD7D56／−164522`有SCK50／DATA17、條件期望15；seq4同原字DATA15相符，不抵銷seq3。原RMT1保留政策依賴usable PCNT，導致沒有bad proof同框RMT及good參考。真LINK2827拒收−1285雖有有限RMT流，仍不能唯一定位根因；全部保留PCNT sidecar為NO_PROOF／NA，不能當0edge。

RMT2目的為補足**首次真正bad proof與首次真正good proof的同框留存**，讓先前缺失可被保留與核對；不藉保留政策修改proof有效性、RAW、clock／cal guard或量測驗收。

## 候選政策及來源核對

Gibbs在專案外`C:/Users/tseng/Documents/Codex/rmt2-wip-20261028`試作，主代理已整合下列公開來源：

- [正式來源manifest](20261028-rmt2-source.json)
- [韌體整合紀錄](20261028-rmt2-firmware-integration.json)

候選完整版本為 `v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1-hxfail5-pcnt2m1-rmt2`。Planck逐一核對manifest列出的12公開檔，當前SHA全部相符；integration列出的12檔與manifest及實際來源亦一致。私密WiFi header不在公開清單，Planck未讀秘密或含憑證建置產物。

本候選公開來源顯示：

- RMT固定3槽，依序FIRST_LINK／FIRST_BAD_PROOF／FIRST_GOOD_PROOF，RMT schema2、外層schema6、profile `DUAL600_FRAME_PCNT_RMT_PROOF_PAIR`；最多64 symbols／通道、原4096-byte狀態／runtime上限保留。
- bad／good角色以本框剛追加的原proof及其`valid`分類，核對seq、bits、accepted／postcheck與packed PCNT身份；不能僅由舊`PROOF_SOURCE`政策位或accepted與否猜分類。
- 同seq可同時請求不同角色；第一次角色一旦選定，即使RMT未arm、未綁定、逾期或缺資料，也保留明確狀態，不用後面較好看的資料替換。
- `HXAP_PROOF_SOURCE`僅在本次真的active proof時設定，修正RMT1在八次上限後仍出現請求標記的診斷歧義；沒有增加proof次數、清bad、放寬ready或將NA升成有效計數。
- 原strict proof的bad黏性、ready0、RAW接受／拒收及clock／cal guard不放寬；RMT只補觀測，API／ISR仍會改變取樣迴圈外排程。

Planck直接從當前公開`.ino`擷取LF正規化、由原`delayMicroseconds(2)`至完整postcheck及其LF的554字元區塊，SHA仍為 `08DFA86D6B694564C4227F9CCE3ABEB51607CC9763BCA848B3BCAF9052BA8E32`，與RMT1及前版相同。這只支持該明確範圍未變，不表示整個框外時序或診斷開銷相同。

## 正式編譯成功，當時host交付待完成（歷史）

[compile原文](20261028-rmt2-compile.log)及 [compile host](20261028-rmt2-compile.host.json)保存主代理18:55:12.833～18:58:41.888台北的正式編譯，exit0。沿用core3.3.12／原FQBN `esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40`；程式1,026,873／1,310,720 bytes，全域124,080／327,680 bytes，編譯容量摘要剩203,600 bytes。

Planck核對compile log SHA與host指定值一致，FQBN及專案外build目錄與source manifest一致。編譯摘要是靜態容量，不等於實板internal heap／WiFi餘裕、RMT driver／symbol有效性或已修復HX；未讀BIN／ELF／生成來源。

| 公開來源／編譯證據 | SHA-256 |
|---|---|
| `05-test/05-test.ino` | `7CB1846E85E6C22002C9AED507E156BEE1CAF8A098266C101497A771715B4485` |
| `05-test/hx_pcnt_adapter.h` | `556FD51CDB72D4E8469D6234B457A654EFE7E95E805424622BEB30E927E24ADF` |
| `05-test/hx_rmt_adapter.h` | `B2C5817B75D9558790CC965D83AFC3459D243B0F1DB8E098C95A3E9B336185D9` |
| `05-test/hx_rmt_diag.h` | `F9275D20E6F1ACC943A4BF87CC3721D45DF421389BFBCFD7F2E4F306677918DE` |
| source.json | `00CE217D069EF7FD880DE4D9CF4E11666182807EAF261D0A5E1B25729B6B6D39` |
| firmware-integration.json | `0498ACC522F12EE5A5285A712167B7A8D7BE17B9BE7AC240795D8293148E1FBE` |
| compile.log | `CBCB10F590C3A10E6A48EF015EFD922DE750ED228602CE1CD60263C081169C67` |
| compile.host.json | `DE2B02FF0A32481956D838252EF6EB6B7D5A3476AAFFA3969E9E4155D70B039F` |

| 階段 | 候選初稿時狀態（後續完成結果另列） |
|---|---|
| 公開韌體整合／來源識別 | 12檔核對相符，版本為候選RMT2 |
| 正式編譯／靜態連結 | 成功，原FQBN、exit0 |
| 最終host／parser回歸 | 尚未READY；逾期／unbound proof合法缺資料路徑仍在修訂及測試，不填通過數量 |
| 新目標核對／燒錄／首次RMT2啟動 | 尚未執行，無成功部署結論 |
| RMT2實板QC／proof pair／CRC／符號 | 未測，不能以RMT1報告或synthetic代替 |
| HX根因／量測品質／366g校正 | 未通過／未完成；既有RMT1失敗及新ERR保留 |

## 候選開發期間仍為 RMT1 的20分鐘零TX窗

- [190518 serial原文](20261028-hx711-rmt2-design-long-190518.serial.log)
- [JSONL完整行主機時間](20261028-hx711-rmt2-design-long-190518.lines.jsonl)
- [host開關埠、命令及計數](20261028-hx711-rmt2-design-long-190518.host.json)
- [主代理passive-analysis](20261028-hx711-rmt2-design-long-190518.passive-analysis.json)

檔名`rmt2-design`描述開發工作，**實板仍RMT1／session8FE7B3DC操作歷程**，未重新查版本或session；不是RMT2實板結果。capture18:45:17.200～19:05:18.099台北、實際1200.899s；1,195 STATUS／1,195不同RX8464→19364，完整行收行18:45:18.211～19:05:17.102。STATUS約1Hz快取不等全部ADC讀框，主機時間不等精確轉換時刻。

全窗IDLE／UNCAL／HX及ADXL OK、AERR0／FIFO5持平，但ERR1→3：

| 首次主機觀察界線，台北 | ERR／RX | 限制 |
|---|---|---|
| 18:46:03.403～18:46:04.407 | ERR1→2，RX8875→8884 | 不是失敗原字或精確ADC時間；快照HX仍OK不能否定累計增加 |
| 19:04:11.824～19:04:12.832 | ERR2→3，RX18773→18782 | ring早凍結，沒有本次逐框RAW／PCNT／RMT事件可填 |

RAW快照平均−164522.5498、範圍−164680～−164385 counts，最大相鄰差184 counts。沒有≥1000 counts快照跳值不等全部框正常，當下載重未確認，均值不可當空載或新校正基準，不能用HX OK抹除兩次新錯誤。

Planck獨核395,696B／2,392完整行／128B未完成尾片；JSONL原文／line_no／UTF-8 offset／已解析fields／UTC及monotonic時序差異0，SHA及bytes／行數／尾片與host吻合。STATUS數、不同RX、首末時間及RAW平均／min／max與主代理passive-analysis一致。128B尾片保留原樣、不補行、不納統計。零裝置TX，host overflow=false／failure=null、CLOSE／QUIT成功、process exit0於19:05:18.104，支持**該已完成窗**COM3已釋放。

| 公開被動窗證據 | SHA-256 |
|---|---|
| serial | `759AAF2918BE897C4BF5E02764D9603FF4D770DEE0B7846DDC47CEA182E8CF74` |
| JSONL | `E58AA415091D4B14EF8863AC7AB12E6C8F7FF1A6514C187B46123658053DCF9D` |
| host | `87F95B8D36BAD6A24C1655D423454D7ECC54062B50A6782D46A1BB1BA2EDAA46` |
| passive-analysis | `F269994AE17FCAF6463DAEF9004438B7B734223DE977BD4386423CB7F9A6B0C8` |

在190518窗結束後，主代理另開新的單次20分鐘擷取、exec14516並沿用同helper及1845工作階段stop-marker流程；該程序其後已因LINK_ERR守衛提前結束，結果另列下節，不再當仍持COM的活動程序。Planck只改本文件，不開COM、不讀秘密／build、不改其他來源／session／QC／handoff；後續部署與首QC按實際證據追加，沒有TARE／CAL或Git操作。

## RMT1 最後被動窗、guard與未擷取缺口

這些資料仍屬燒錄RMT2前的RMT1操作歷程，不能按helper延用的`pcnt2m1-preflash-guard`檔名當實板版本識別。各窗沒有重查VERSION／session，當下載重未確認，RAW不作空載或重量依據。

| 窗及證據 | 完整STATUS主機收行時間，台北／不同RX | RAW平均／範圍，counts | 已觀察錯誤及保存 |
|---|---|---|---|
| 191152：[serial](20261028-hx711-rmt2-design-long-191152.serial.log)／[JSONL](20261028-hx711-rmt2-design-long-191152.lines.jsonl)／[host](20261028-hx711-rmt2-design-long-191152.host.json) | 19:07:15.595～19:11:51.743；276／276，RX20443→22974 | −164540.7428／−164684～−164417 | ERR3→6，末HX LINK_ERR；91,418B／552完整行／128B尾片 |
| 192759：[serial](20261028-hx711-pcnt2m1-preflash-guard-192759.serial.log)／[JSONL](20261028-hx711-pcnt2m1-preflash-guard-192759.lines.jsonl)／[host](20261028-hx711-pcnt2m1-preflash-guard-192759.host.json) | 19:27:53.646～19:27:58.666；6／6，RX31962→32010 | −164481.1667／−164518～−164440 | ERR8持平／HX OK；1,980B／12完整行／0尾片 |
| 193233：[serial](20261028-hx711-pcnt2m1-preflash-guard-193233.serial.log)／[JSONL](20261028-hx711-pcnt2m1-preflash-guard-193233.lines.jsonl)／[host](20261028-hx711-pcnt2m1-preflash-guard-193233.host.json) | 19:32:27.728～19:32:32.745；6／6，RX34528→34574 | −164448.5000／−164505～−164388 | ERR9持平／HX OK；1,980B／12完整行／0尾片 |

191152的原20分鐘上限觀察實際19:07:14.838～19:11:52.737、277.899s，依序在19:09:53.253～54.258看到ERR3→4、19:11:14.590～15.593看到4→5、19:11:50.741～51.743看到5→6並HX LINK_ERR。helper記`Unexpected device state; passive recording stopped`，停止原定20分鐘並保存；不是滿20分鐘、主機資料遺失或正常HX驗收通過。

191152末19:11:51.743 ERR6至192759首19:27:53.646 ERR8是未擷取缺口新增2；192759末19:27:58.666 ERR8至193233首19:32:27.728 ERR9又有缺口新增1。不能稱19:11～19:32連續記錄，不能定位這些錯誤時刻／原字或歸因編譯。各窗IDLE／UNCAL，AERR0／FIFO5持平，ring早已凍結，沒有新ERR的逐框資料。

Planck獨核三窗raw／line_no／UTF-8 offset／fields／UTC與monotonic順序差異0，SHA及最終bytes／完整行／尾片與各host吻合。128B尾片保留不補造／不納統計；三窗皆零裝置TX、overflow=false、CLOSE／QUIT成功、exit0及port_released=true。191152的failure屬實板非OK守衛，其正常exit0只證關閉程序，與觀察失敗分列；兩個fresh guard failure=null。各已完成窗關閉不延伸到新boot後的活動觀察。

## 最終 host 交付與 PF2-10 解除

- [host manifest](20261028-rmt2-host-manifest.json)／[host release](20261028-rmt2-host-release.json)
- [Node原始log](20261028-rmt2-candidate-host-node-tests.log)／[native原始log](20261028-rmt2-candidate-host-native-tests.log)
- [較早preflight副本](20261028-rmt2-preflight-review.md)，原樣保留歷史阻擋
- [最終preflight](20261028-rmt2-preflight-final.md)／[最終副本追溯JSON](20261028-rmt2-preflight-final.json)

Gibbs完成193 Node／148 native，Ampere在固定來源另重跑17組新契約、6組safety及16組歷史parser通過。Planck核對公開log與交付記錄，未自行重跑全部測試；獨立重算12韌體及24工具／測試檔SHA與檔長均相符，host release公布檔hash亦全符。韌體與原正式編譯來源一致，parser收尾沒有藉改C改變原proof／RAW規則。

PF2-10解除只接受嚴格限定的真deadline缺口：configured RMT／NOT_BOUND、fresh first-role proof為最後proof、seq與attempts／lastseq帳相符、omitted恰1、RAW確無該seq、role metadata及payload空、真正600s WINDOW或20s POST及CPU窗條件等全部成立時，明列`RAW_UNRETAINED_AT_DEADLINE`／waveform unavailable。這不是豁免一般缺event、早期ring覆寫或把missing填0，也不改原valid／ready。完整無LINK到期及兩型截止實際exporter／逐條負測已在最終定向回歸核對。

最終preflight的SHA `02F27A6FA051CE395C851F0B113418F8C63E114AE4A4DADAB9A892D3B4926F26`與追溯JSON相符；較早`E0FA...`副本不改寫。這解除主機preflight阻擋，不代表RMT1已修、RMT2已完成實板量測驗收。

## 正式寫入與 RMT2 首次啟動

[identity](20261028-rmt2-identity.log)、[upload](20261028-rmt2-upload.log)、[upload host](20261028-rmt2-upload.host.json)、[deployment](20261028-rmt2-deployment.json)確認COM3／ESP32-D0WD-V3 rev3.1／MAC `e0:8c:fe:a8:42:bc`。19:33:17.001～19:34:19.198台北四區0x1000／0x8000／0xE000／0x10000寫入及Hash校驗、exit0，upload log SHA與host一致，deployment來源與正式manifest一致。未要求NVS／全片清除，after no-reset留download mode，USB由主代理保持供電。

[first-boot serial](20261028-rmt2-first-boot.serial.log)／[JSONL](20261028-rmt2-first-boot.lines.jsonl)／[host](20261028-rmt2-first-boot.host.json)保存19:34:44.8739138 RTS assert、19:34:45.0780809 release、DTR false、零裝置TX，為本次寫入後單次首次app boot。實板完整rmt2版本／外層schema6／RMT schema2及新session `437B6829`已捕獲；不是為湊BAD或補啟動額外重置。新boot ERR0不消除重啟前ERR9或RMT1的bad proof。

sidecar40960B成功、RMT attempted1／available1／setup1、allocated4032B、兩driver-computed resolution10MHz／idle1ms／無DMA。這是本次資源讀回，實際GPIO stage／角色選取及symbols仍待完整封存；不是物理時鐘、完整波形或HX端供電驗證。

Planck獨核9,489B／65完整行／0尾片，raw／行號／offset／UTC及monotonic順序及SHA與host相符。20 STATUS／20不同RX1→163、ERR0／AERR0、IDLE／UNCAL。非STATUS第12行OLED SAFE parsed token只含`x4..123`，原文`x4..123,y4..59;`完整保存，屬既有parser逗號截token限制；STATUS fields全符。CAPTURE_END19:35:04.8577263、CLOSE19:35:04.8674562、port_closed=true／failure=null，僅支持首boot窗已釋放。

## 本版首 QC：限定診斷可用，完整角色仍待封存

[193544 QC serial](20261028-hx711-rmt2-postflash-qc-193544.serial.log)／[JSONL](20261028-hx711-rmt2-postflash-qc-193544.lines.jsonl)／[host](20261028-hx711-rmt2-postflash-qc-193544.host.json)及 [Ampere本版QC](../quality/reports/2026-10-28-rmt2-qc.md)屬session437B6829。root實送STATUS、HX_DIAG STATUS、ADXL_DIAG、HELP、WIFI STATUS、STATUS、ADXL_DIAG、HX_DIAG STATUS八條唯讀命令，沒有清警示／TARE／CAL／WiFi切換或額外reset；只讀輸出仍會增加主迴圈／串列負載。

Planck核對18,309B／110完整行／0尾片，raw／line_no／offset／fields／UTC及monotonic順序差異0，SHA及計數與host一致。capture19:35:12.085～19:35:44.480台北、32.395s；34 STATUS／34不同RX233→517、收行19:35:12.708～19:35:43.873，全IDLE／UNCAL／HX及ADXL OK，ERR0／AERR0。RAW快照平均−164434.6765、範圍−164513～−164323 counts，非空載或重量精度驗證。

| 子項 | 本版實際證據 | 限制 |
|---|---|---|
| PCNT摘要 | proof4／good4／bad0／ready1、setup1／clockguard0 | 摘要通過，四筆原始proof與GPIO／兩unit仍待DUMP；不解除RMT1的7good／1bad失敗 |
| RMT摘要 | schema2、setup1／terminal0／pending1／closed0、4032B、兩resolution10MHz | 兩個瞬間pending不證同job卡住，角色／真symbol／CRC待補 |
| ADXL快取 | ACC_RX3005→5306，+2301；DIAG_ERR0→0 | 支持更新，不當精確ODR或零漏樣 |
| FIFO | 行11，19:35:16.727 FIFO1→行25，19:35:17.730 FIFO2 | DUMP前新增，不能歸因尚未執行的DUMP或僅按查詢順序定因 |
| WiFi／heap | CONNECTED／requested1／radio1／有效IP；first_connected free58204、current58212、largest55284、min54520 | 有限連線及資源快照，不證長期餘裕；64KiB只為pre-WiFi setup guard |
| OLED／RGB | ACK／OFF_REQ，無同窗現場目視 | 未測，不能填通過 |

首QC host pcnt_unsafe=null／failure=null、overflow=false，CLOSE／QUIT成功，exit0於19:35:44.491，支持該首QC已關埠。本節記錄首QC當時僅限定診斷條件通過、完整角色／GPIO／CRC仍待封存的狀態。root其後同boot另開最多620s觀察、自然600s總診斷窗（原exec81122）；該窗及後90s現皆已結束，結果見下節。不額外重啟湊BAD，未觸發BAD不推成舊HX已修。

| 新公開部署／擷取證據 | SHA-256 |
|---|---|
| host-manifest | `170596D375CB5C23861C2A6699824B39459D87E3EB71BA0D136B3CD5F7A00AB9` |
| host-release | `31482865545CAD60D9E53851C1F9F57EF50708D1C89449E8011245EC25E22448` |
| preflight-final.md | `02F27A6FA051CE395C851F0B113418F8C63E114AE4A4DADAB9A892D3B4926F26` |
| upload.log | `CABD65649FC01EB11FB79CAA9F2BB593155EA7D13706CDA4A3C98E7C612D8AEE` |
| deployment | `F9F7165AB24AD80622F77BF9F3B95A88B6C0EC788ABC2A1D945A3DEA63B6AF2C` |
| first-boot serial | `3294436225AD488A12A4717C404D2E19B3CDE092D8ACBC1EC3CF94C3D31EC435` |
| first-boot JSONL | `7A5150A1FC67A6F4C75F85E73CEE06BE58ED34B494590C3E9F22DA5E4E537BC1` |
| first-boot host | `C76B470B62667926EEE473639BBADDD2198525998BDA35BCEA81D3B15D64068E` |
| QC serial | `63EDF8D3D8524B3049A448821E87E342D1229C822B39997E9384D98DA5AAA12F` |
| QC JSONL | `D155D6661BCF25B647D6564DECF699F712D6D03BE1801A5C6F96A912E3949D9C` |
| QC host | `8012377063B41268D576032668293B4DF4DC34609820A47DB7C5599D08A44BF7` |

首QC整理時Planck只更新本session，不改其他文件／來源／QC，不開COM或讀秘密／build；後續已按本版完整新capture追加下節，未CAL、未commit／push。

## 自然 WINDOW 封存與全部完整性核對

- [194500完整serial](20261028-hx711-rmt2-continuous-observe-194500.serial.log)
- [逐行主機時間JSONL](20261028-hx711-rmt2-continuous-observe-194500.lines.jsonl)
- [host命令、計數與關閉](20261028-hx711-rmt2-continuous-observe-194500.host.json)
- [analysis-v6](20261028-hx711-rmt2-continuous-observe-194500.analysis-v6.json)

同session437B6829，主機OPEN_ACK19:36:53.472～CAPTURE_END19:45:00.249台北；這是主機擷取窗，不等於從boot起600s全部被主機持續記錄。root只送19:36:56.979 HX_DIAG STATUS及自然封存後19:44:47.226 HX_DIAG DUMP，無新reset／TARE／CAL／清警示或WiFi切換。初期首boot、首QC與此窗之間各有主機擷取缺口。

Planck獨核249,588B／1,568完整行／0尾片，JSONL原文／line_no／UTF-8 offset／所有已解析fields／UTC及monotonic順序差異0，SHA及bytes／行數與host一致；由原文重跑v6，整個分析物件與封存analysis-v6 deepStrictEqual。host expected／observed session一致、run_conflict=null、overflow=false／failure=null，CLOSE／QUIT成功、process exit0於19:45:00.258，該窗已關閉。

兩RAW槽各127列seq5293～5419、逐canonical word完全相同，sidecar亦同seq／逐word重複，共**127唯一框**，不是254獨立量測。共同start610764us、freeze600752492us，經過600141728us，service封存比600s晚141728us；原read_start截止判準沒有延長。frozen attempts5419＝5292覆寫＋127保存，omitted0／overflow0／seq_wrapped0、trigger0／flags0，freeze_reason WINDOW。只能說到原診斷hook的5419個完成attempt沒有觸發既定LINK／SAT／相鄰accepted跳≥1000；不能離線重驗已覆寫5292框或當全部ADC轉換無誤。

Planck解析重跑的CRC／身份／順序驗證通過；Ampere另用獨立table式CRC32從原始canonical words核算：

| 區塊 | 最終核對結果 |
|---|---|
| RAW雙槽 | 254個十九word row CRC全通、逐word重複，127唯一accepted |
| sidecar雙槽 | 254個二十word row CRC全通，兩整槽均`0x8E4BF1E9` |
| HXF | 103word CRC `0x7E8831FE`，CLOSED_NO_LINK／captured0，12點NA |
| PCNT | 405word CRC `0xE5E72A31`，四proof及原GPIO欄自洽；NO_LINK／HOLD兩unit均NA |
| RMT | 三trace CRC `0xCEDF6381`／`0xCEDF6381`／`0x36EB02F8`；634word整塊CRC `0x3F9F2B37` |
| 外層 | schema6、RMT schema2、同版本／session與完整END／區塊順序通過 |

匯出自洽不等於ADC真值或全診斷用途通過；未觸發HXF／HOLD是未取樣NA，不當0edge、LOW或無故障。

## 四筆 accepted DATA 條件模型差與角色限制

原proof僅seq1～4，SCK均50、DATA11／11／11／13與條件期望相同，4good／0bad／ready1後不再新增proof。127保留框全部accepted／flags0、PCNT sidecar usable，SCK均50；其中4框有條件DATA+2：

| seq | bits／RAW counts | DATA實測／條件期望 |
|---|---|---|
| 5305 | FD7DB0／−164432 | 13／11 |
| 5313 | FD7DA6／−164442 | 15／13 |
| 5410 | FD7DEF／−164369 | 11／9 |
| 5414 | FD7E33／−164301 | 11／9 |

它們發生於原proof已停止之後，不是actual bad proof，因此FIRST_BAD空白符合本版保留政策，**不等於全窗沒有DATA模型差**；這4框沒有同框RMT時序。count窗含lead-in／tail、離散電位與API括弧，不能直接把+2定位在25pulse內、判毛刺／丟位或修RAW。

FIRST_LINK與FIRST_BAD均未觸發，零canonical是未選／NA，不以RMT1事件補本版實板路徑。FIRST_GOOD為原proof index0／seq1／valid1，嵌入RAW及packed PCNT與proof表嚴格吻合；雖seq1已不在末端ring，仍由完整proof表對接。當時global proof_ready尚0的sidecar NO_PROOF不否定這筆原proof自身valid1，兩者適用範圍不同。GPIO僅BASE／DRIVER／READY已讀回，LINK／BAD EVENT stage未觸發，不能當全部event腳位已驗。

127框read CPU括弧191～246µs；PCNT前置63～226µs（中位64）、後置67～207µs（中位75），不是SCK脈寬／ADC週期，也未包含全部RMT前後開銷。4good不能抵銷RMT1 seq3 bad或既有HX品質失敗。

## GOOD reason512：終止 marker 後半word的格式限制

FIRST_GOOD seq1的原proof valid1，但RMT trace reason512、bounded_symbols_consistent=false、DATA format_verified=false，兩種判定不矛盾。SCK安全複製25 symbols、49正duration前綴（25HIGH／24LOW），按driver10MHz名義換算HIGH2.7～10.6µs、LOW3.6～7.0µs；首末censored，不能推出完整輸入或HX端所有HIGH均短於60µs。

DATA共6 symbols，前5word的正duration前綴可保留為H54.0／L8.0／H6.9／L6.9／H37.0／L7.3／H7.9／L8.0／H29.0／L17.6µs（名義換算）。**第6word index5原始值為`0x00B08000`：低half `0x8000`是duration0 marker，後upper `0x00B0`非零。** 當前source先見0設ended，再見後half非零，回HXR_SYMBOL512；prefix解析器在第一個0停止，沒有把後half加成波形。

Gibbs與Kierkegaard核對固定SDK／TRM後確認，終止marker後unused半word沒有有效或必清零契約；所以這個formatflag不能直接當HX電氣異常、額外edge、17.6µs新run或原+2根因。duration0也不轉成實測1ms idle。原始word／reason512／formatfalse保留，不事後改reason0；本輪只補格式限制說明，**不再修改韌體、flag或重啟**。有界前綴與CRC不證雙RX同步相位、物理頻率、全輸入無丟失或HX端類比／供電。

## FIFO及同步DUMP的時間分列

此窗478 STATUS／478不同RX1154→5473，完整收行19:36:54.153～19:45:00.079，ERR0／AERR0；RAW快照平均−164411.8389、範圍−164515～−164270 counts，含快取觀察、非校正基準。

| 首次主機觀察界線，台北 | 已觀察現象 | 限制 |
|---|---|---|
| 首QC末19:35:43.873 FIFO2→本窗首19:36:54.153 FIFO3 | +1在未擷取缺口發現 | 時刻／原因未知，不能稱連續 |
| 行19，19:36:57.362 FIFO3→行21，19:36:58.365 FIFO4 | DUMP前新增 | 不歸尚未發生的DUMP，不以附近只讀查詢推因果 |
| 行958，19:44:47.285 BEGIN→行1556，19:44:54.927 END | DUMP同步輸出7.642s | 來源明示blocks_polling，CRC通過不取消干擾 |
| 行1557，19:44:55.062 HX STALE／ADXL MISSING／AGE7799ms／FIFO4→行1559，19:44:56.068 OK／OK／FIFO5 | DUMP事後新增 | 與前述警示分列，恢復OK不證根因已修或動態完整 |

## 最後90秒窗、ERR新缺口及最終關埠

[195015 serial](20261028-hx711-postdump-passive-195015.serial.log)／[JSONL](20261028-hx711-postdump-passive-195015.lines.jsonl)／[host](20261028-hx711-postdump-passive-195015.host.json)／[passive-analysis](20261028-hx711-postdump-passive-195015.passive-analysis.json)是最後零TX有界窗，capture19:48:44.757～19:50:15.147台北、90.390s。90 STATUS／90不同RX7519→8331，完整收行19:48:44.853～19:50:14.157，全IDLE／UNCAL／HX及ADXL OK，ERR1／AERR0／FIFO5持平。RAW平均−164308.3778、範圍−164397～−164192 counts，當下載重未知。

Planck獨核29,732B／180完整行／32B尾片，raw／line_no／UTF-8 offset／fields／UTC及monotonic順序差異0，SHA及bytes／行數／尾片與host相符，STATUS與RAW統計吻合passive-analysis。32B尾片原樣保留，不拼成STATUS、不納統計。

前完整窗最後19:45:00.079為ERR0，到此窗首19:48:44.853已ERR1，**224.774s未擷取缺口內發現至少一次新HX錯誤**。原ring／proof／RMT已封存，沒有該次seq／拒收bits／同框RAW或PCNT／RMT；不能拿後窗accepted cache−164318冒充故障原字，也不能把舊NO_LINK改成新事件。後90秒沒有再增只描述該窗，不能結案或判與DATA+2／格式flag同因。

最後窗CLOSE／QUIT成功、overflow=false／failure=null、process exit0於19:50:15.153，port_released=true。root確認所有capture已完成、COM3已關閉釋放，USB保持供電；原exec81122不再活動。沒有額外reset、清警示、TARE／CAL或commit／push，當下物理載重未重新確認。

| 最終公開證據 | SHA-256 |
|---|---|
| 194500 serial | `26E17882769FBE4A0F5AEC96CA695093EF99587649296183587132E69BD80F39` |
| 194500 JSONL | `9E9C1B533A421FB1FCD214993EDFF1B06E9041913841C5B99DED54F9E9644038` |
| 194500 host | `C2D01314EB97CD2E756EBEEE4DC5A2CD93B6B31B83C409AE2A4190A0600DF215` |
| analysis-v6 | `61CAD3B3FFC30A792D18E4628C6E9A89C8821CC1B5CBF27D33C5EB90EF1806F7` |
| 195015 serial | `E46A8240A07DFD01298DEEDD4FF609BB72F8F7E2BADA2E43F4A08DA6B29798E4` |
| 195015 JSONL | `2F23EB78D88A87F2B6DEFD4BEF65C67EF21583DA988F59333463577229217F5A` |
| 195015 host | `3569BC0D4CC94F3D9AACB03177B5C7A40139F91B1A8F2AD7A27248D313AE8382` |
| 195015 passive-analysis | `82A7801FBA135C79202BA5DE010EDAE49906D0813EEB34C1309B3DD12F6777B1` |
| Ampere最終RMT2 QC | `DFF0449DFD4D9A250069DD3DF1F34C48DAA74C9075C333824E4CB6CD9F85E521` |

## 最終用途與未解項

DRAM超額、正式建置／寫入、首次boot、限定唯讀、GPIO復原及完整schema6／CRC匯出子項已完成；本版僅供條件性診斷用途。FIRST_GOOD身份已核，但其RMT形狀一致性未過，原512／formatfalse保留；FIRST_LINK／BAD真事件、目視及實板故障注入未測。4個accepted DATA+2、FIFO各段新增、後缺口ERR0→1及歷史近零／拒收／漂移仍開，量測品質未通過、材料未測、366g校正未執行。

Gibbs與Kierkegaard本輪決定只補格式限制，不再修改韌體或重啟；根因仍需可信HX端同步SCK／DOUT及供電證據，不能靠MCU快取、條件模型或短窗無錯宣布釐清。Planck只修改本文件，未碰handoff／README／QC／源碼、未開COM或讀秘密／build，未commit／push。
