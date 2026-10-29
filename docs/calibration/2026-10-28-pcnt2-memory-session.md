# 2026-10-28 逐框 PCNT 記憶體修正與 HX 診斷 session

日期／時區：2026-10-28，Asia/Taipei（UTC+08:00）。主代理唯一操作 COM3；Gibbs 修改公開韌體與測試，Kierkegaard 核對物理及時序限制，Ampere 依新版保存證據獨立 QC，Planck 整理公開追溯。本文件依實際完成階段增補，較早的待驗狀態保留為歷程。

> **17:48 後交接狀態**：實板仍為正式編譯／四區寫入校驗／首次啟動成功的 PCNT2m1，session `71F2BE52`、40,960-byte sidecar 配置成功。完整 schema4 已捕獲 seq2376 拒收−642／ERR0→1，frame SCK50、DATA8對條件模型6，hold SCK0／DATA2；兩槽逐word重複為256唯一框，CRC／seq／版本自洽。診斷用途條件通過，根因未知、量測品質未通過、366g未校正。後窗依序有缺口ERR1→2、17:36:06.396～07.404首次觀察ERR2→3／LINK_ERR，以及17:36:07.404～17:38:59.058缺口發現ERR3→4；ring已凍結，這三次新錯誤沒有逐框資料。17:43～48約五分鐘PCNT2m1被動快照ERR4持平，不能解除異常。RMT僅專案外候選試作，未部署；主代理另開下一個五分鐘被動窗並唯一持有COM3，不能用已完成窗close推稱當下埠已關。USB保持，未再reset／清警示／TARE／CAL。詳下方各窗及 [Ampere 本版報告](../quality/reports/2026-10-28-pcnt2m1-qc.md)。

## 本輪授權與起點

使用者要求修正逐框診斷韌體 DRAM 超額，正式編譯成功後自動燒錄並完成 QC，保持供電，繼續記錄 RAW 與時間、排查 HX 異常，釐清原因後完成 366 g 校正，不再詢問操作同意。此授權涵蓋必要的軟體修改、編譯、目標核對、燒錄、診斷與條件成立後的 TARE／CAL；實際物件放置及卸載仍須現場操作證據，不能由歷史放置猜測目前載重。

最新交接的實板版本為 `v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1-hxfail3-pcnt1`，USB 保持供電、UNCAL、COM3 已釋放。這是前次交接狀態；本輪燒錄前新擷取另列於下節，未查 VERSION 或 session，不能當重新識別版本。366 g 沒有成功 `CAL 366` 記錄；重啟後不可套用任何舊 RAM 零點或比例。既有 accepted 近零字、LINK 拒絕及漂移尚未解除，短窗沒有再次觸發不能當根因已解。

前一版 PCNT1 的部署與真事件見 [PCNT1 session](2026-10-28-hxpcnt-diagnostic-session.md)及 [PCNT1 真事件 QC](../quality/reports/2026-10-28-pcnt1-continuous02-qc.md)。MCU 端接收窗內 DATA 4 edge／SCK 0 edge，僅支持該數位窗內有 DATA 變化；不能推出 HX 端類比電壓、脈寬、實際有效 clock 或供電根因。原觀察窗沒有覆蓋失敗讀框的 25 pulse，是逐框方案要補充的證據。

## 舊 PCNT2 候選與備份核對

舊候選版本 `v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1-hxfail4-pcnt2` 在主機檢查後，正式連結因 `.dram0.bss` 超出 40,400 bytes 失敗，見 [原編譯失敗紀錄](20261028-pcnt2-compile-failed.log)。候選沒有燒錄或實板 QC。公開來源恢復 PCNT1 後，依原設定重新 [編譯成功](20261028-pcnt1-restored-compile.log)：程式 1,013,665 bytes、全域 123,928 bytes，前輪未重燒。

候選備份位於專案外 `C:/Users/tseng/Documents/Codex/pcnt2-wip-20261028`。Planck 本輪唯讀逐一核對 `manifest.json` 所列 38 項公開檔的檔長與 SHA-256，全部一致、差異 0；未讀秘密或建置產物。manifest SHA-256 為 `D7AD7B5956805E351223612E0A92DAF562995AFDA5D36C823F30CD2B4D61FD47`。

備份 `docs/calibration/20261028-pcnt2-public-host-validation.json` 是舊主機交付摘要，其 `formal_arduino_link_performed=false`、`device_validation=false`，即使舊 status 含 ready，也不得代表本輪正式編譯或部署。其資源記錄為既有 RAW 雙槽 28,672 bytes、逐框 sidecar 雙槽 40,960 bytes、pending 80 bytes、診斷物件預算 75,000 bytes；主機預算斷言不等於 ESP32 可連結或 runtime 可用記憶體。

## 新修訂的待驗證證據

| 項目 | 必須保存的本輪證據 | 初稿時狀態（後續結果另列） |
|---|---|---|
| 記憶體修正 | 最終方案、容量／所有權、配置或壓縮失敗處理、診斷與原 trial／cal 資料隔離；避免只將連結問題轉成 runtime 配置失敗 | 待 Gibbs 修訂與檢查 |
| 來源與主機檢查 | 新版完整版本、公開來源 SHA-256、相關 native／Node 測試結果；不讀私密檔或含憑證產物 | 待本輪結果 |
| 正式編譯 | 沿用 core 3.3.12／IDF 5.5.5及 FQBN `esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40`；成功返回、程式與全域容量、專案外產物位置 | 待本輪結果 |
| 正確目標與寫入 | COM3、ESP32-D0WD-V3、目標 MAC `e0:8c:fe:a8:42:bc`；完整寫入各區校驗及正常返回 | 待本輪結果 |
| 啟動與限定功能 QC | 自然啟動／版本、IDLE及校正狀態、WiFi／IP、首末 RAW／RX／AGE、ADXL 快取、ERR／AERR／FIFO、觀察窗內重啟跡象 | 待 Ampere 新版 QC；缺目視不可補作通過 |
| runtime 記憶體與時序 | setup 前後與最低 heap／最大可配置區、診斷可用性；逐框 API 額外 pre／post 開銷、GPIO 配置與正對照及失敗路徑 | 待實板數據，編譯摘要不替代 |
| 逐框匯出 | 同 session 的 ANY／LINK 原字與 sidecar 逐 seq 對應、有效旗標／NA、HXF／PCNT、各 CRC 與最外 `END_HX_CAPTURE`、完整尾片保存 | 待真實 bundle，不以 synthetic fixture 替代 |
| RAW 與時間追蹤 | 原始 serial、逐行主機時間 JSONL、host 開關埠／TX／完整性紀錄、不可覆寫的衍生分析與原文雜湊 | 待本輪擷取 |
| HX 根因與校正 | 區分已證觀察與候選假說，必要修復經驗證；新 boot 近時空載 TARE、實際 366 g 放穩、單次 CAL 與載卸／回零驗證 | 未完成；不能用後續無錯解除舊異常 |

逐框方案會增加原 SCK train 前後 API、鎖定、時間戳及複製開銷，即使原 train body 未改，ready 到 clock 延遲與排程條件仍改變。SCK 計數 50 不等於證明 HX 端收到 25 個有效 clock，DATA 計數也受前後窗及內部觀察電位限制；不能從有效 CRC 推出原 ADC 字正確。DUMP 同步輸出可能造成 STALE／MISSING 與 FIFO 警示，須與讀框異常分開記錄。

本輪編譯、燒錄、實板捕獲、QC、校正及 Git 結果，待主代理交付新公開證據後補入。Planck 僅建立本文件，未開 COM3、修改韌體、讀秘密、執行燒錄或校正；不據此聲稱埠目前仍關閉、供電連續性或部署成功。

## 燒錄前第一個零 TX 被動窗：17:09～17:10

- [serial 原文](20261028-hx711-before-per-read-flash-171037.serial.log)
- [完整行主機時間 JSONL](20261028-hx711-before-per-read-flash-171037.lines.jsonl)
- [host 設定、控制事件及收尾](20261028-hx711-before-per-read-flash-171037.host.json)

這個窗仍屬原 PCNT1 的燒錄前快照，不是修正後逐框方案的結果。主代理以 serial-monitor v0.15.0 在 OPEN 前確認 115200／8N1、DTR／RTS off，裝置命令陣列為空，未以軟體切換控制線。capture START 17:09:06.854 至 END 17:10:37.700（台北），實際 90.846 秒；90 STATUS 收行為 17:09:07.694～17:10:37.003，90 個不同 RX 104805→105623。

全窗 IDLE／UNCAL／HX OK，ERR 11／AERR 0／FIFO_FULL 3 持平；RAW 平均 −164267.0222、範圍 −164386～−164180 counts，89 對相鄰快照最大絕對差 122 counts。未在本窗看到 ≥1000 counts 跳值；這是沿用的異常檢索條件，不是重量精度或校正前檢的通過門檻。交接中上一已存窗末 ERR 5，本窗首已 11；只知未擷取間隔內累計增加 6，不能定位時間、原字或原因。

Planck 獨立核對 serial 29,970 bytes、180 完整行、0 尾片；JSONL 180 列的原文、行號、UTF-8 byte offset 及已解析欄位差異 0，UTC／monotonic 時間順序無回退，serial SHA 與 host raw_sha256 一致。host 的 overflow=false、failure=null，CLOSE／QUIT 均成功，process exit0、port_released_by_process_exit=true；這支持該窗結束時主代理已釋放 COM3，不延伸到其後正在進行的其他窗。

| 公開檔 | SHA-256 |
|---|---|
| serial.log | `F35797C94A2771A4C438610DE4214598C92EC64EE67856672552A11E9E305AFA` |
| lines.jsonl | `60D3054B70216FFD8FCB1488330FDFF8C6104AFFE4C73F1B4BB494EE0B205F76` |
| host.json | `4FE3688EC8F801CB23E0D66CFC31B710B403227B06E62C8B163A216AF7C8394F` |

約 1 Hz STATUS 是快取快照而非全部讀框；主機完整行接收時間不等於 HX 轉換時刻，無 boot／VERSION／session 新識別。host 的 `prior_empty_tray_confirmed=true` 只追溯過往現場回報，**當下物理載重未重新確認**，本窗不得寫成已確認空載或拿 RAW 均值作新 TARE 基準。此時本輪仍未正式編譯、燒錄或 CAL；HX 原因與 366 g 校正未解，未將本窗無大跳／ERR 持平當修復證據。

## 燒錄前後續三個零 TX 被動窗

以下各窗分別開啟及關閉串列埠，窗間沒有擷取，不能合稱連續觀察。仍承接 PCNT1 操作歷程，沒有新 VERSION／session／boot 識別；當下載重未重新確認，各 RAW 均值不作空載、重量或新校正基準。

| 窗名與證據 | capture START～END，台北時間／實長 | STATUS 首末完整收行時間，台北 | STATUS／不同 RX 與首末 RX |
|---|---|---|---|
| 171433：[serial](20261028-hx711-before-per-read-flash-171433.serial.log)／[JSONL](20261028-hx711-before-per-read-flash-171433.lines.jsonl)／[host](20261028-hx711-before-per-read-flash-171433.host.json) | 17:13:03.394～17:14:33.829／90.435 s | 17:13:03.523～17:14:32.840 | 90／90；106962→107779 |
| 171750：[serial](20261028-hx711-before-per-read-flash-171750.serial.log)／[JSONL](20261028-hx711-before-per-read-flash-171750.lines.jsonl)／[host](20261028-hx711-before-per-read-flash-171750.host.json) | 17:16:19.419～17:17:50.286／90.867 s | 17:16:20.219～17:17:49.557 | 90／90；108762→109577 |
| 172101：[serial](20261028-hx711-before-per-read-flash-172101.serial.log)／[JSONL](20261028-hx711-before-per-read-flash-172101.lines.jsonl)／[host](20261028-hx711-before-per-read-flash-172101.host.json) | 17:19:30.536～17:21:01.206／90.670 s | 17:19:30.909～17:21:00.220 | 90／90；110506→111323 |

| 窗名 | RAW 平均／範圍，counts | 最大相鄰快照絕對差，counts | ERR／AERR／FIFO_FULL，均為全窗持平 | serial bytes／完整行／未完成尾片 bytes |
|---|---|---|---|---|
| 171433 | −164263.9444／−164371～−164164 | 157 | 11／0／3 | 30,034／180／64 |
| 171750 | −164261.0222／−164388～−164161 | 174 | 11／0／3 | 29,970／180／0 |
| 172101 | −164270.1333／−164367～−164146 | 126 | 12／0／3 | 30,002／180／32 |

三窗 STATUS 均 IDLE／UNCAL／HX OK，未見 ≥1000 counts 快照跳值，各窗沒有 ERR 增量；但 **17:17:49.557 的末筆 ERR 11 至 17:19:30.909 的首筆 ERR 12 之間存在未擷取缺口**。這個新增錯誤沒有逐框原字、確切時刻或原因，不能因前後各窗持平就忽略，亦不歸因編譯或額外查詢。零 TX 與 1 Hz 快照不能驗證全部 HX 讀框。

Planck 分別核對三窗：每個 JSONL 均 180 列，原文／行號／UTF-8 offset／已解析欄位差異 0，UTC／monotonic 時序無回退；檔長／完整行／尾片與 host 相符、serial SHA 與 host raw_sha256 一致。64及32-byte 尾片原樣保留、不補成完整行、不納統計。三窗 host 均 device_commands_sent 空、overflow=false、failure=null，CLOSE／QUIT 成功、process exit0、port_released_by_process_exit=true，支持各窗終點 COM3 已釋放；不保證窗間供電或物件狀態的新現場證據。

| 窗名／檔案 | SHA-256 |
|---|---|
| 171433 serial | `32F5B89BBE5549EDCFCC2E33F651571CBCA3ED792027C26CDDD6600C91CAEED0` |
| 171433 JSONL | `D269346C82C433AE1215C32279CEA9EC832DA3E1C768DBC18206DF73B9D6BB10` |
| 171433 host | `98EBDEE1DB1C13CC8A491E5E1582C745F5127131DAB43902CBC1B41D26E329A8` |
| 171750 serial | `7034668C6512B2F45054EDB21056B7596EDF5BF4A3B8874FB27A385EFEF27CEC` |
| 171750 JSONL | `F7F3C822621217F7B3D5254EBA9A1DB8CF6BF2B90084B47E6275417C10C7A74D` |
| 171750 host | `3CB16B7731473FB65D7D58616AB6ABCD4BF752FDDCD1672C4DD979316DB192EC` |
| 172101 serial | `98C28F6E7310BD5B5C9B12EA29C1C3DEB66C2DC1C1F3FA3454962F70B9E72D84` |
| 172101 JSONL | `E0943B4DB6F23CC9B763D0271E51DDB2458DC25EC59C0A5D090BB534242AE56A` |
| 172101 host | `0760E6B9E1614D8F47532D56A3B2BBEC18B5790A993BA42A7D5F421BD98981C4` |

## PCNT2m1 來源封存與主機驗證

Gibbs 交付並凍結版本 `v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1-hxfail4-pcnt2m1`，正式編譯由主代理另行執行；本節只記錄來源凍結當時的公開來源及主機證據，當時尚無成功編譯／部署／實板 QC 結論，後續完成結果另列下節。

原靜態的 40,960-byte sidecar 雙槽改成 setup 內單次 `MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT` 配置，容量不變，不依賴 PSRAM。兩個各 20,480-byte 區塊及配置後 65,536-byte 可用餘裕都成立才公布兩個指標；第一／第二塊配置或餘裕失敗時，回收已配置區塊、保留明確 sticky 失敗狀態，不在 loop／讀框重試。RAW 雙槽與 trial／cal 容量不縮減，失敗時仍持續原 RAW 記錄，關閉逐框 counter API，匯出專用降級 schema3／`DUAL600_FRAME_PCNT_UNAVAILABLE`，不把缺 sidecar 包成完整 schema4。

公開 `HX_MEMORY` 新欄位保存配置前／後、當下與首次實際 WiFi CONNECTED 的 internal 8-bit free／largest／minimum；尚未連線時首次連線欄為 NA。65,536-byte 守衛是 setup 配置當下的條件，不能當連線後或長時間運作的記憶體保證，實板 WiFi、最低餘裕及時序開銷仍需新 QC。

- [公開來源 manifest](20261028-pcnt2m1-source.json)：Planck 逐一核對所列 10 項公開來源 SHA-256，差異 0；未讀私密 WiFi header 或編譯產物。
- [本輪主機驗證摘要](20261028-pcnt2m1-host-validation.json)：`device_validation=false`，編譯、燒錄與實板驗證另列。
- [Node 原始測試輸出](20261028-pcnt2m1-node-tests.log)：190 tests，190 pass／0 fail／0 skipped。
- [native 原始測試輸出](20261028-pcnt2m1-native-tests.log)：7 組共 111 cases，彙總與各組 6＋16＋32＋18＋10＋15＋14 一致；Gibbs 交付零編譯 warnings，Planck 核對公開 log 與摘要，未獨立重跑編譯器。

Gibbs 的 actual C++ exporter 主機合成驗證包含正常 schema4、第一／第二次配置失敗及 reserve 失敗的三種降級 schema3，兩個 RAW 槽各 256 列且 canonical 原字一致。摘要另記原 25-pulse loop／postcheck 與 Git HEAD 相同；這些屬主機來源／模型證據，不能替代實板 PAD 邊緣、真 LINK 及資料品質驗證。

| 本輪公開來源／證據 | SHA-256 |
|---|---|
| `05-test/05-test.ino` | `557329421EC8E7FCFCEF5BE448C2951DDB5604D3B6F888173848C3BECC55414A` |
| `05-test/hx_attempt_storage.h` | `9125DDDCCBA233D278ECCED2D8AFE7C2A4326CB1DFC72BEC89ADC3F36D4FABD8` |
| source.json | `1FB9A265E17319FC66379100F10DE9201EB5CA12B4ACC770BF1878E94B1412F6` |
| host-validation.json | `F8209044F5937937216F34556A80C27359793B439C07D2ED3638877CCB8F1746` |
| node-tests.log | `F41947C7C7C33E9F17499D47BA01BFC8C6334BEAC0BAD7DAE96394C7C9AEE9EE` |
| native-tests.log | `BDA750E05E48F8671DD610A3826295B1E89FEED4B474D943867A7807F1CFA9C6` |

上述主機交付時主代理正在正式編譯，當時不能宣稱編譯、寫入、實板配置、WiFi／heap、逐框診斷或 CAL 成功。既有 HX 異常與燒錄前新缺口 ERR 增量均保留，不因記憶體修正而關閉。

## 第五個燒錄前被動窗與操作守衛

[172352 serial](20261028-hx711-before-per-read-flash-172352.serial.log)、[JSONL](20261028-hx711-before-per-read-flash-172352.lines.jsonl)、[host](20261028-hx711-before-per-read-flash-172352.host.json)為另一次零 TX 窗。capture 17:22:22.331～17:23:52.846 台北、90.515 s，STATUS 17:22:22.513～17:23:51.854，共90行／90不同 RX 112077→112896，RAW平均 −164259.5333、範圍 −164372～−164182 counts，最大相鄰絕對差138。全窗 IDLE／UNCAL／HX OK、ERR12／AERR0／FIFO3 持平；沒有當下載重、版本或 session 的新現場確認，不稱空載或 HX 已修。

Planck 核對 30,003 bytes、180完整行、33-byte 未完成尾片，JSONL原文／行號／offset／fields／時間順序差異0，serial SHA及各項計數與host一致。尾片保留不補造／不納統計，CLOSE／QUIT成功、exit0及port_released_by_process_exit=true。主代理以該窗末兩筆完整 IDLE／UNCAL STATUS 作後續目標操作前守衛，見 [preflash-host-check](20261028-pcnt2m1-preflash-host-check.json)，沒有把尾片當完整狀態。

| 第五窗公開檔 | SHA-256 |
|---|---|
| serial | `EE3E94EDBB3909E09CAAC6F938E0FA3F4C1E3D2A7F96F38CD57AEBD81D845D82` |
| JSONL | `C3B0C10DA2728EE7ADA5E3D9592D5EE981B3B9DACA7A104CECC6E0AC07FEE4DC` |
| host | `0409AD15EAE8407C804F6742278A2F036ABEE2F83B02B04B2D99924CC3856815` |

## 正式編譯、目標核對與完整寫入

- [正式 compile 原文](20261028-pcnt2m1-compile.log)／[compile host](20261028-pcnt2m1-compile.host.json)
- [晶片 identity 原文](20261028-pcnt2m1-identity.log)
- [正式 upload 原文](20261028-pcnt2m1-upload.log)／[upload host](20261028-pcnt2m1-upload.host.json)
- [部署摘要及來源對應](20261028-pcnt2m1-deployment.json)

正式編譯17:19:10.523～17:22:50.450 台北、exit0；沿用 ESP32 core3.3.12／IDF5.5.5 及原 FQBN `esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40`。程式 1,018,545／1,310,720 bytes，全域 124,072／327,680 bytes、摘要剩203,608 bytes。靜態連結超額已解除；編譯容量不等於 runtime internal heap 餘裕。產物仍在專案外，Planck 未讀 BIN／ELF／生成來源。

identity與upload均為 COM3、ESP32-D0WD-V3 revision v3.1、MAC `e0:8c:fe:a8:42:bc`；upload17:24:30.932～17:25:32.465、exit0，0x1000／0x8000／0xE000／0x10000四區均實際寫入及 `Hash of data verified`。未請求 NVS 或全片清除，USB 由操作者保持插接／供電。identity後留於bootloader，upload 使用 before／after no-reset，工具完成時尚未首次應用啟動；此狀態由部署摘要明示，後續單次 RTS 首次啟動另列，不把 upload 完成混作 app 已執行。

Planck 獨立核對 compile／upload log SHA 與各自 host 指定值一致，upload 原文有4次 Hash校驗及正確MAC，deployment列10來源hash與來源manifest差異0。來源hash與燒錄工具校驗分別支持來源追溯及寫入驗證，不冒稱獨立讀回完整flash映像hash。

| 正式公開證據 | SHA-256 |
|---|---|
| compile.log | `2732D0540977C212595BB0DF7F906A7BB5378E1793F4C82115161BDA4F191E1D` |
| compile.host.json | `93F8496681F72632BB46EF9E760ECB9CC95D7D5351A4E1BF6CD4F8053FFA2B0B` |
| identity.log | `FEF0D358972FD8D4576FAF8156D2DBB70F2E17B505150D26D357A5F456869183` |
| upload.log | `2AE6BE4D5388B2C1B4D61E91C736C349CE236B40ADBDE75A98AEC1F985C776DA` |
| upload.host.json | `A99BEA6B5706708A635B525AD3574F46BE7CF21D54A66BE9CF97E33DF9C8EAB3` |
| deployment.json | `EF353A74317F8DC27B36395C5FFA9CADC9D382BDD4E853D385283E0F4CFAA4D7` |
| preflash-host-check.json | `B746EDEDFB2AD881E917CBA8A7863A8EA143F8BABAF103F2D5D7B4914411A6D7` |

## 本版首次應用啟動及實際 sidecar 配置

[first-boot serial](20261028-pcnt2m1-first-boot.serial.log)、[JSONL](20261028-pcnt2m1-first-boot.lines.jsonl)、[host](20261028-pcnt2m1-first-boot.host.json)保存主代理在download mode開埠後，17:26:01.9057017 assert RTS、17:26:02.1134157 release RTS，DTR false、沒有裝置指令，為寫入後單次首次應用啟動。捕獲ROM／app啟動及完整 `hxfail4-pcnt2m1` 版本，`HX_DIAG_READY`為schema4、session `71F2BE52`、allocation_state1。ROM列 `POWERON_RESET`本身不證外部USB曾斷電；不據此沿用舊RAM係數。

實板回報 allocation_attempts1／ready1／requested及allocated40960，配置前 internal free158844／largest110580／min129908，配置後 free115828／largest69620／min86892。配置前後 free 差43,016 bytes，與40,960-byte payload不同，該差值不是單獨driver或純payload成本。`HX_PCNT_RESOURCES`的diag_static_bytes30920及driver setup前後heap另有其觀察範圍，不能與internal 8bit sidecar快照混算。WiFi首次連線在boot初始行尚未發生，wifi欄正確為NA，不補0。

Planck 核對9,083 bytes／64完整行／0尾片、JSONL原文／行號／byte offset及時間順序全吻合，SHA與host指定值一致，host port_closed=true、failure=null。20 STATUS／20不同RX1→167，ERR0／AERR0／FIFO1全窗持平，UNCAL；這是新epoch，不是把燒錄前ERR12清成已修。非STATUS的boot第12行OLED `SAFE=x4..123,y4..59;`在JSONL已解析SAFE只保留`x4..123`，屬欄位parser截取範圍；完整raw_line與serial均無損，不拿該parsed欄替代目視或整行原文。STATUS欄位獨核差異0。

## 首輪限定燒錄後 QC

[QC serial](20261028-hx711-pcnt2m1-postflash-qc-172659.serial.log)、[JSONL](20261028-hx711-pcnt2m1-postflash-qc-172659.lines.jsonl)、[host](20261028-hx711-pcnt2m1-postflash-qc-172659.host.json)與 [Ampere本版獨立QC](../quality/reports/2026-10-28-pcnt2m1-qc.md)屬相同新epoch。capture17:26:27.402～17:26:59.775 台北，32.373s；唯讀TX依序STATUS、HX_DIAG STATUS、ADXL_DIAG、HELP、WIFI STATUS、STATUS、ADXL_DIAG、HX_DIAG STATUS，無TARE／CAL／CLEAR／WiFi切換或補reset。

34 STATUS／34不同RX223→509、收行17:26:27.681～17:26:58.762，RAW平均−164261.3529、範圍−164357～−164200 counts，最大相鄰快照差115。全窗IDLE／UNCAL／HX及ADXL OK、ERR0／AERR0／FIFO1持平。ADXL快取ACC_RX2842→5144（+2302），FULL_OBS1／DEPTH_OBS0／OVR_OBS1／DIAG_ERR0持平；FIFO既有啟動觀察仍保留，不據此宣稱動態無漏樣。

WiFi CONNECTED／requested1／radio1、IP有效；HX_MEMORY顯示首次CONNECTED及兩次查詢當下free62912／largest59380／min59072，allocation1／attempt1／ready1。連線後free低於setup守衛65536並不違反其僅在配置當下的明示條件，但必須保留長期餘裕未驗的限制，不以當窗成功推成永遠充足。PCNT摘要proof_count4／good4／bad0／ready1，兩槽LIVE，僅摘要尚未獨核逐框SCK/DATA、正對照細項、開銷或CRC。

Planck 獨核17,507 bytes／108完整行／0尾片，JSONL原文／行號／UTF-8 offset／已解析欄位／時序差異0，serial SHA與host相符，CLOSE／QUIT成功及exit0支持該窗結束已釋放COM3。主代理之後再次開埠持續擷取，本節不推稱後續埠已關或資料已封存。

| 啟動及首 QC 公開證據 | SHA-256 |
|---|---|
| first-boot serial | `53779406C608DA68E1FDD82F8CA2583F4A06FD7110FFDD8094E46EB8DD9AA9BE` |
| first-boot JSONL | `E81722E209D656B94630DACAD448EC91820EEA3CC5787367301F93D9329FF247` |
| first-boot host | `C5751D58900B8325BAF6BDD5D449CEAE7F63B7A4AFCF4EFD06A6979CAB28C467` |
| QC serial | `5549A8B083CB8248DE8B8EDB13A1CB02B64CFCB7871D181AE443B651E780E764` |
| QC JSONL | `3C18CC4310A8B81FDB325790F1C3F7DEA07ED693153CDB4196929EE93D7AB571` |
| QC host | `B0400B75A9504A1E073F397CFD0B6B734FD778FF2943E38DD39F540F56FFED1C` |

首QC完成時Ampere結論為：公開來源、正式編譯、正確目標四區寫入／校驗、首次啟動、sidecar成功配置及限定IDLE唯讀子項通過；當時完整逐框匯出及真事件仍待驗，OLED／RGB目視未測，量測品質未通過，HX根因與366g校正未解。後續相同session擷取已封存並另列下節；Planck只讀公開證據，未讀私有/live資料。本輪未送TARE／CAL，Git結果由主代理另行記錄。

## 同 session 真 LINK 與完整逐框匯出

- [continuous serial 原文](20261028-hx711-pcnt2m1-continuous-observe-173057.serial.log)
- [逐行主機時間 JSONL](20261028-hx711-pcnt2m1-continuous-observe-173057.lines.jsonl)
- [host 設定、命令與收尾](20261028-hx711-pcnt2m1-continuous-observe-173057.host.json)
- [封存 v4 分析](20261028-hx711-pcnt2m1-continuous-observe-173057.analysis-v4.json)
- [Ampere 本版完整 QC](../quality/reports/2026-10-28-pcnt2m1-qc.md)

這個窗承接同一session71F2BE52，主機OPEN_ACK17:27:21.847～CAPTURE_END17:30:57.266 台北，與首QC之間有未擷取間隔。設備TX僅17:27:25.352 HX_DIAG STATUS及17:30:38.726 HX_DIAG DUMP；DUMP在封存通知後、前兩STATUS為IDLE／UNCAL才送，沒有重啟／TARE／CAL／CLEAR／WiFi切換。

Planck 獨核223,991 bytes／1,526完整行／0尾片，JSONL的原文／行號／UTF-8 offset／所有已解析欄位／UTC及monotonic時序差異0，檔長／行數／尾片與host一致、serial SHA與host相符。從原文重新執行公開v4分析器，整個結果與封存analysis-v4 `deepStrictEqual`，未覆寫歷史檔。203 STATUS／203不同RX718→2551，RAW快照均值−164258.1034、範圍−164368～−164093 counts、最大相鄰差210；拒收−642不覆寫最近accepted快取，因此STATUS沒出現該字不代表沒有真異常。

### 完整性、唯一框數與條件計數差

schema4外層END及ANY RAW→ANY sidecar→LINK RAW→LINK sidecar→HXF→PCNT順序完整，同版本／session。各槽保存seq2249～2504，127前＋觸發＋128後；兩槽所有256列逐canonical word相同，故為**256唯一attempt**，不能報成512次獨立觀察。各槽255 accepted、1 rejected；凍結attempt2504＝2248覆寫＋256保存＋0 omitted，overflow0／seq_wrapped0。

Planck重跑分析器的序列／CRC／同框綁定檢查通過；Ampere另用獨立CRC算法核對兩個RAW槽各256×19word、兩個sidecar槽各256×20word的所有row、sidecar block `0x8A2ED7A3`、HXF103word `0xBC0FADAE`、PCNT405word `0xE084B1C8`全通。這是保存與匯出自洽，不是獨立ADC真值或未保存前2248框的驗證。

| seq | 原RAW／判斷 | SCK總雙edge | DATA總雙edge／條件期望 | 限制 |
|---|---|---|---|---|
| 2287 | −164280／accepted | 50 | 13／11 | 正常accepted也有+2 |
| 2322 | −164294／accepted | 50 | 13／11 | 正常accepted也有+2 |
| **2376** | **0xFFFD7E／−642／rejected** | **50** | **8／6** | 原postcheck LOW，RX2375不增，ERR0→1 |
| 2423 | −164327／accepted | 50 | 11／9 | 正常accepted也有+2 |

seq2375 accepted−164258、seq2377 accepted−164182；−642與兩鄰右移8位的數值相容仍僅候選，不能反推原字或確診丟8位。seq2377在hold介入後，不當自然恢復證明。DATA+2不是LINK的充分條件，且count窗包含lead-in／tail與離散電位，不能定位額外edge在25pulse內、還原極性／脈寬或判定HX端有效clock。四個既有原read正對照seq1～4各SCK50、DATA9／7／9／11，條件計算相符且GPIO四階段讀回通過；proof在凍結ring前，不能造作它們位於seq2249～2504。

256個唯一sidecar均usable、無guard／unusable，SCK／DATA窗均包含原read CPU括弧且SCK包住DATA窗。原read括弧198～251µs；新增prepare至原read_start 68～169µs（median68）、read_end至finish 39～218µs（median45）。這是CPU/API開銷，不能作SCK脈寬、ADC週期或零干擾聲明。

### 同框及失敗後觀察窗

seq2376原read CPU括弧261902694～261902914µs，prepare261902598／finish261903049；frame SCK START261902650～680／STOP261903012～014，DATA START261902685～688／STOP261902932～950。frame count SCK50／DATA8均有效，仍未知各edge精確時間及HX端波形。

同seq的hold在frame已停止並深拷貝後啟動，SCK0／DATA2均有效；SCK START完成261903288≤DATA START開始261903293，DATA STOP完成262176622≤SCK STOP開始262176668，resume seq2377於262176855才開始。這支持MCU數位接收窗內DATA有變化、沒有觀察到SCK edge，不能推HX端供電或類比電壓。frame DATA STOP至hold DATA START間依API邊界存在343～364µs盲區，不補造其中edge。

HXF12個目標中8個有效離散LOW、2／10／50／150000µs四點missed／NA，最後有效點相對anchor273630～273643µs；不是連續LOW證明，也不是精確250ms停止。觀測已改變下框時序，不能據此唯一區分硬體／取位／供電根因。

### HX、FIFO與同步DUMP的時間界線

| 主機完整行時間，台北 | 已觀察資料 | 證據界線 |
|---|---|---|
| 首QC末17:26:58.762 FIFO1→本窗首17:27:21.945 FIFO2 | +1僅在窗間缺口發現 | 時刻／原因未知，早於DUMP |
| 17:27:24.954 FIFO2→17:27:26.051 FIFO3 | 本窗DUMP前新增，兩筆ERR0 | 中間唯讀HX_DIAG STATUS不構成因果證明 |
| 17:30:23.691 ERR0→17:30:24.694 ERR1、HX LINK_ERR | 首次快照看到本次LINK | 主機接收界線不同於逐框boot CPU時間 |
| 行407，17:30:38.784 BEGIN→行1514，17:30:52.082 END | DUMP同步匯出13.298s | 來源明示阻塞polling，不能稱背景無擾動 |
| 17:30:52.224 HX STALE／ADXL MISSING、AGE13462ms、FIFO3→17:30:53.228 HX／ADXL OK、FIFO4 | DUMP立即事後新增警示 | 與DUMP前新增分列，後續OK不解除動態完整性 |

host expected／observed session71F2BE52、run_conflict=null、overflow=false／failure=null，CLOSE／QUIT成功、exit0於17:30:57.270，支持本bundle窗釋放COM3。根因仍未知，完整診斷僅在UNCAL及保留原字／警示的範圍條件通過，不能轉成量測可用或CAL前提已成立。

## DUMP後兩個獨立零TX窗及新HX錯誤

兩窗皆沿用本版操作歷程而未新查VERSION／session，窗間有缺口；ring早已凍結，兩次新ERR沒有逐框原字／sidecar，不能把seq2376的frame或hold套用。沒有新增裝置TX／reset／TARE／CAL或清資料，當下載重仍未確認。

| 窗與公開檔 | capture START～END，台北／實長 | STATUS／不同RX及首末完整收行時間，台北 | RAW均值／範圍／最大相鄰差，counts | 狀態與保存 |
|---|---|---|---|---|
| 173405：[serial](20261028-hx711-postdump-passive-173405.serial.log)／[JSONL](20261028-hx711-postdump-passive-173405.lines.jsonl)／[host](20261028-hx711-postdump-passive-173405.host.json) | 17:32:34.282～17:34:04.919／90.637s | 90／90，RX3438→4270；17:32:34.570～17:34:03.931 | −164267.4333／−164369～−164166／173 | IDLE／UNCAL／HX OK、ERR2／AERR0／FIFO4持平；29,646B／180行＋32B尾片 |
| 173608：[serial](20261028-hx711-postdump-passive-173608.serial.log)／[JSONL](20261028-hx711-postdump-passive-173608.lines.jsonl)／[host](20261028-hx711-postdump-passive-173608.host.json) | 17:34:37.247～17:36:08.119／90.872s | 90／90，RX4589→5424；17:34:38.052～17:36:07.404 | −164266.3000／−164343～−164168／114 | IDLE／UNCAL，最後HX LINK_ERR／ERR3；AERR0／FIFO4持平；29,617B／180行／0尾片 |

bundle末17:30:57.241 ERR1至第一後窗首17:32:34.570 ERR2，是未擷取缺口中發現+1，無法定位時刻、原字或原因。第二後窗末17:36:06.396 HX OK／ERR2／RX5416→17:36:07.404 HX LINK_ERR／ERR3／RX5424，首次觀察本窗新錯誤；快照RAW是最近accepted值，不是失敗原字，不能用其近正常範圍消除故障。

Planck分別核對兩窗原文／行號／UTF-8 offset／已解析fields／UTC及monotonic時序差異0，serial SHA及bytes／完整行／尾片與host相符。第一窗32-byte尾片保留原樣、不納統計；第一窗failure=null。第二窗helper在末端非OK守衛停止，host failure為 `Unexpected device state; passive recording stopped`，這是**實板觀察觸發的停止原因**，不代表主機遺失、檔案截斷或串列關閉失敗。兩窗CLOSE／QUIT成功、process exit0、port_released_by_process_exit=true、overflow=false；第二窗exit0於17:36:08.129，COM3已釋放，未知後續實板是否自行恢復，不為此額外reset。

| 新事件及後窗公開證據 | SHA-256 |
|---|---|
| continuous serial | `E3EE4D9A95FA8C3831B9B007644D00E507E7075BD5BA7931D547EAC4A1FF5416` |
| continuous JSONL | `2B982D73E2652ABAED5F1F81B53FD15FD02251137A5540F1FAB19616358681EE` |
| continuous host | `4AF068AF099900FDA87EEF0FB14E90CDE19FE77728E64FB432D3D7BAB50E7319` |
| analysis-v4 | `EAF043F68B74C29D10AD91506D17050FA09E7771BD21866F10CFF17ED570FBD7` |
| 173405 serial | `8B69287170EC95A12F3B526F1ED404610590494ABB9D4661CA3E339367D49770` |
| 173405 JSONL | `22A9FD536ED1F9A006035FBE83B1134624FD957829E47B448AB8766C7B253CA7` |
| 173405 host | `1B1ABCE2092A15B0D1530EC6771C80C399FD465765B478AABDF3DF7546DB79B7` |
| 173608 serial | `2BB452A9125158B986A8A0236BA4615DEF422D07469197919071FE32124C1D68` |
| 173608 JSONL | `F3B709E03C8B54B6A72DEE47547DE7409654FFCB1AD4F69C7561D5C16A309F86` |
| 173608 host | `E2D24D1C72C9F682F87601E48A5562A8159F9FEE0AECE98C5D5A2EFF782AD21E` |

截至這兩個後窗，DRAM超額與診斷部署問題已處理，真LINK及後續新錯誤證明HX資料品質問題仍存在，沒有TARE／CAL366成功結果。當時Gibbs的RMT工作僅離線可行性評估，沒有新RMT部署；後續候選試作另列下節，不以候選構想取代本版實板結果。Planck只更新本session，Git結果及後續策略由主代理整合。

## 保持 PCNT2m1 下的後續快照：缺口ERR4與RMT候選設計期間

後續兩窗仍由已部署PCNT2m1輸出，沒有新RMT實板資料。Gibbs正在專案外試作RMT候選；本輪沒有再燒錄或reset，不能把檔名的`rmt-design`解讀成RMT已執行。窗間沒有主機擷取，未新讀VERSION／session、當下載重未確認；RAW與時間只描述實際保存的快取快照。

| 已完成窗與公開證據 | capture START～END，台北／實長 | STATUS完整收行時間，台北／不同RX | RAW平均／範圍／最大相鄰差，counts | 保存及計數 |
|---|---|---|---|---|
| 174028：[serial](20261028-hx711-postdump-passive-174028.serial.log)／[JSONL](20261028-hx711-postdump-passive-174028.lines.jsonl)／[host](20261028-hx711-postdump-passive-174028.host.json)／[主代理摘要](20261028-hx711-postdump-passive-174028.passive-analysis.json) | 17:38:58.059～17:40:28.362／90.303s | 17:38:59.058～17:40:27.369；89／89，RX7029→7854 | −164259.8202／−164376～−164121／217 | 29,399B／179完整行／64B尾片；ERR4／AERR0／FIFO4持平 |
| 174822：[serial](20261028-hx711-rmt-design-passive-174822.serial.log)／[JSONL](20261028-hx711-rmt-design-passive-174822.lines.jsonl)／[host](20261028-hx711-rmt-design-passive-174822.host.json)／[主代理摘要](20261028-hx711-rmt-design-passive-174822.passive-analysis.json) | 17:43:21.431～17:48:22.144／300.713s | 17:43:22.018～17:48:21.146；299／299，RX9489→12287 | −164256.9298／−164385～−164154／170 | 98,775B／598完整行／160B尾片；ERR4／AERR0／FIFO4持平 |

173608末17:36:07.404為ERR3／LINK_ERR，新174028首17:38:59.058已ERR4／HX OK；只知這個未擷取缺口發現+1，不能定位精確時刻、失敗原字、恢復經過或原因。兩個新窗全STATUS為IDLE／UNCAL／HX及ADXL OK，沒有窗內ERR／AERR／FIFO增量或≥1000 counts快照跳值，並不表示每框皆正常；ring凍結後沒有這次新錯誤的原字或sidecar，不能以先前seq2376事件填補。

Planck獨立重掃兩個serial及JSONL：原文／line_no／UTF-8 offset／已解析fields／UTC與monotonic順序差異0，bytes／完整行／尾片與host相符、SHA與host及主代理摘要相符；STATUS數、不同RX、首末時間、RAW平均／min／max與各自passive-analysis完全一致。64及160-byte不完整尾片原樣保留、不補行、不納統計。兩窗裝置TX皆空，host overflow=false／failure=null，CLOSE／QUIT成功、process exit0、port_released_by_process_exit=true，支持**各已完成窗**終點COM3已釋放；之後主代理另開下一五分鐘窗並獨占COM3，該新窗尚未封存，本節不填結果或聲稱當下埠空閒。

| 新窗公開證據 | SHA-256 |
|---|---|
| 174028 serial | `FE6B47486297E316D323B3F3040220CD6F1065680E9C81DBB2E16FBC1F71672C` |
| 174028 JSONL | `7ED61A1D14228B36DEE1413866B6551C6203A7ABA1385C70A949BA7D20E53CF8` |
| 174028 host | `44B77544DFFFE937A70834AB1DE82C8128D4F3C29BFB0C24D2FFE059A892D37F` |
| 174028 passive-analysis | `816CDD8513672B6BA1BDAD12CC1795ED3E16EA0C2AC91014F7D0F65A5DC98197` |
| 174822 serial | `7C141E2BA67D5C60A4863517B18559ECC82B95B9450513F8E4118E0959FF2989` |
| 174822 JSONL | `F7D3400BBB5B60B18E5749960358E6CC85780BEF5AABCF230B3A17431AA82555` |
| 174822 host | `F03EA3B7314CAD7855C88A427EEBD1154D63171545CC3B3F6856EA4DA5706DDD` |
| 174822 passive-analysis | `99F841A66D1F16D7DEDCFFB72B4492211E74B5A6AC1E80371DC0842FF5B77BAF` |

本次Planck仍僅修改本session，沒有開埠、修改韌體、讀秘密或建置產物，沒有將候選RMT、CRC通過或短窗ERR持平當作HX根因已解。量測品質與366g校正仍未完成。
