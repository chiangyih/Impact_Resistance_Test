# 2026-10-28 RMT 數位持續時間診斷與隔離取證

> **RMT1已部署、功能QC未通過**：`hxfail5-pcnt2m1-rmt1`正式編譯、正確MAC四區寫入校驗及首次boot成功，新session `8FE7B3DC`。PCNT正對照8次7good／1bad／ready0，seq3 DATA17對條件期望15；保留失敗、停止正常QC餘下查詢，不能沿用PCNT2m1條件通過。隔離schema5完整匯出及CRC／seq通過，共256唯一框，LINK2827拒收−1285並ERR0→1；有限RMT數位流含DATA內部LOW1.6µs，但缺bad／good參照、PCNT全NO_PROOF，不能確診根因。量測品質未通過、366g未校正。RMT2僅專案外保留政策候選，未READY／未正式編譯或燒錄。主代理正唯一持有COM3做18:45工作階段單次最長20分鐘被動記錄（exec1218及相應stop-marker流程），該新窗尚未封存；舊窗close不代表當下埠空閒。USB保持供電，沒有清警示／TARE／CAL。詳下方及 [Ampere RMT1 QC](../quality/reports/2026-10-28-rmt1-qc.md)。

> 已結束的長窗：主代理18:10開始原定最多20分鐘的零TX被動擷取，之後以主機controlled marker在18:22:27.838正常結束，實際727.401s；公開原文／JSONL／host已封存為`rmt-design-long-182227`。CLOSE／QUIT成功、process exit0於18:22:27.845，該窗COM3已釋放；原exec session `90477`與停止marker流程已完成，不是仍在工作的持埠程序。長窗ERR4→5的主機首次觀察界線為18:14:51.133～52.141，沒有逐框資料可定位原因。詳下方追加結果。

部署前狀態（歷史）：RMT候選完成原FQBN正式編譯時，最終host工具回歸仍待交付，尚未燒錄；當時實板為PCNT2m1。其後host release、正式寫入、首次RMT1啟動與本版QC結果已另列下節，不能將較早候選狀態或PCNT2m1結果套到新版。主代理唯一操作COM3，各窗關埠紀錄只描述該窗終點。

使用者已授權修正逐框 DRAM、正式編譯後自動燒錄及 QC，保持供電繼續記錄 RAW／時間、排查 HX，原因釐清後完成 366 g 校正，不再詢問操作權限。RMT 為此持續診斷的後續候選；不授權自由落下、材料驗收、猜測當下載重或清除歷史異常。當下載重未重新確認；校正仍受根因及資料品質前提限制。

## 已有證據與識別目標

[PCNT2m1 session](2026-10-28-pcnt2-memory-session.md)及[獨立 QC](../quality/reports/2026-10-28-pcnt2m1-qc.md)已記錄：seq2376 的拒收 `FFFD7E/-642` 有效 SCK50／DATA8，條件模型預期6；另三個正常 accepted 框亦 DATA+2。隨後 hold 有效 SCK0／DATA2。完整雙槽及逐框 CRC／seq 通過，只256個唯一框。

這些結果不能確定單一 clock 高低寬、DATA 短脈衝時間、HX 端電氣／供電或根因。RMT 候選目標是在既有25脈衝迴圈之外，保存 MCU GPIO25／33 的數位持續時間資料，以檢查高電位過長及額外短變化的假說。既有取樣迴圈、RAW 接受條件、警示、RAW／trial 容量及40KiB PCNT sidecar不變，不以新診斷修正 RAW。

物理專家核對的 [AVIA HX711 V2.0 原廠資料](https://cdn.sparkfun.com/assets/b/f/5/a/e/hx711F_EN.pdf)列 PD_SCK 高電位0.2–50µs、超過60µs進入 power down；名義2µs delay、整框CPU耗時或50個MCU邊緣均不等同實際逐脈衝寬度。RMT亦僅為MCU端數位觀察，不能直接驗證HX端電壓、有效clock或供電。

## 候選與檢查界線（部署前歷程）

設計初期由Gibbs在專案外 `C:/Users/tseng/Documents/Codex/rmt1-wip-20261028` 唯一修改公開候選，當時repo保留PCNT2m1來源；之後主代理已依公開來源清單移入RMT候選並正式編譯，下節另列最終來源識別。repo來源已更換不表示實板已燒錄。Kierkegaard與Ampere獨立審查，主代理負責整合、正式編譯、目標與燒錄；子代理不讀秘密或含憑證的建置產物、不開COM3。

- 直接使用本機 IDF5.5.5 RMT RX API，不用Arduino `rmtInit`，不用不存在的公開解析度查詢或無效GPIO -1規避driver設定。
- 暫擬每通道64 symbols、APB來源／10MHz、無DMA、有限自然idle，小型固定工作緩衝及first LINK／first DATA模型差事件快照。全部大狀態在setup一次配置內部8bit heap；實際靜態／heap、driver量與WiFi後餘量仍待正式及實板驗證。
- 驗BASE／DRIVER／READY GPIO，還原及讀回SCK OE、output route、drive、pull等；不讓RX初始化改變原SCK輸出。無法確認腳位安全即保留clock guard，不能假造更新。
- 原clock迴圈內不加入指令；RX busy不延後合法HX讀框來湊完整波形，重疊／不足覆蓋必須標記。每次API前後CPU括弧、原讀框、PCNT與RMT token／seq分開記錄。
- callback為ISR，event資料只在callback有效。不得存其暫時指標供後續讀取；只做有界完成處理，沒有Serial或動態配置。
- classic ESP32 callback不提供可靠RX overflow證明；`is_last`及未滿64不能單獨證明完整。滿容量、prefix、timeout、缺callback、晚callback、token不符均有明確無效／缺資料狀態。
- timeout後不假設disable能drain晚callback，不盲重arm／重用／釋放同一buffer；可terminal-disable RMT至新boot、保留ctx與buffer，原RAW照走。
- 兩RX依序啟動沒有共同精確零點，不能直接驗證跨線0.1µs相位。CPU API括弧不是實體閘門時間；不完整trace不作無異常／無脈衝結論。
- 新來源須先完成有意義的host/native／實際adapter與匯出解析檢查，原FQBN正式編譯、正確目標四區校驗後才部署，並立即進新版獨立QC。PCNT2m1報告不能關閉新版本缺陷。

以上設計階段當時尚未有候選實板符號、波形、資源、有效性或根因結論；後續實際成功與失敗另列下節。沒有新的TARE／CAL、WiFi切換或物理操作；Git結果由主代理另記。

## 原廠及 SDK 語意的離線審查更新

以下是Kierkegaard的候選審查結論，Planck另開原廠來源核對引用與關鍵段落；不是新實板資料或READY結論。HX711原廠時序數值保留前節引用，名義delay與MCU觀察仍不等於HX端電氣證據。

- [ESP32 TRM Version5.9](https://documentation.espressif.com/esp32_technical_reference_manual_en.pdf)，§30.2.5、印刷頁730／PDF index729：無edge超過idle門檻時，RX用zero-period項結束。候選不得把該zero轉成實測1ms或已知電位；由marker推終端電位及時長的條件尚須另外證明。
- [exact IDF5.5.5官方RX test](https://raw.githubusercontent.com/espressif/esp-idf/v5.5.5/components/esp_driver_rmt/test_apps/rmt/main/test_rmt_rx.c)，L349–383：RX先arm，filter測試拒絕2µs短pulse後，接收10µs HIGH，斷言首symbol為HIGH且duration約10µs。這支持修正先前候選一律預期「firstLOW／lastLOW／51正duration runs」的假設；不能把該固定形狀當本板正常trace必然格式。目前新有效格式仍在審查。
- [exact IDF5.5.5 RX driver](https://raw.githubusercontent.com/espressif/esp-idf/v5.5.5/components/esp_driver_rmt/src/rmt_rx.c)，L522–620：copy及硬體RX_ERROR處理之後，callback交付symbols指標、數量及is_last，沒有獨立overflow證明。Kierkegaard據此要求保留prefix／截斷／未知狀態；is_last或未滿64不能單獨保證完整。

審查另要求保留raw ticks；整體trace不完整或prefix不能把其中可信的正duration長HIGH候選刪掉。任何lower bound必須另列成立的邊界／終止條件，不能把有條件MCU數位時長直接寫成HX端powerdown確診。以上修正仍需候選實作、host／adapter測試、正式編譯及新版實板QC；未滿足前不宣稱可燒錄。

## 候選開發期間仍為 PCNT2m1 的被動窗

下列是RMT候選在專案外開發時，**既有PCNT2m1實板**輸出的快取STATUS；檔名`rmt-design`只記錄工作階段，不能表示已部署RMT、已取得RMT symbols，或候選已READY。兩窗間未擷取，沒有新VERSION／session讀回，只依既有操作歷程承接PCNT2m1；當下載重未重新確認，不能把RAW均值當空載、重量或新TARE基準。

| 窗與公開證據 | capture START～END，台北／實長 | STATUS完整行收行時間，台北／不同RX | RAW平均／範圍／最大相鄰差，counts | 保存與計數 |
|---|---|---|---|---|
| 175443：[serial](20261028-hx711-rmt-design-passive-175443.serial.log)／[JSONL](20261028-hx711-rmt-design-passive-175443.lines.jsonl)／[host](20261028-hx711-rmt-design-passive-175443.host.json)／[主代理摘要](20261028-hx711-rmt-design-passive-175443.passive-analysis.json) | 17:49:43.268～17:54:43.614／300.346s | 17:49:43.465～17:54:43.614；300／300，RX13061→15875 | −164256.9967／−164369～−164121／164 | 98,894B／599完整行／27B尾片；ERR4／AERR0／FIFO4持平 |
| 180106：[serial](20261028-hx711-rmt-design-passive-180106.serial.log)／[JSONL](20261028-hx711-rmt-design-passive-180106.lines.jsonl)／[host](20261028-hx711-rmt-design-passive-180106.host.json)／[主代理摘要](20261028-hx711-rmt-design-passive-180106.passive-analysis.json) | 17:56:05.524～18:01:06.021／300.497s | 17:56:05.907～18:01:05.025；299／299，RX16646→19448 | −164259.3612／−164392～−164147／212 | 98,767B／598完整行／96B尾片；ERR4／AERR0／FIFO4持平 |

兩窗全STATUS均IDLE／UNCAL／HX及ADXL OK，裝置TX為空，沒有本窗ERR／AERR／FIFO增量或≥1000 counts快照跳值；這是約1Hz快取觀察，不能代表全部HX轉換、逐框正常或已修復。PCNT2m1 ring早已凍結，既有ERR3→4只在17:36:07.404～17:38:59.058未擷取缺口被發現，後續ERR4持平不會補出該錯誤的原字或sidecar，也不解除此前accepted異常、LINK或漂移。

Planck獨立重掃兩個原始serial、JSONL及host：原文／line_no／UTF-8 byte_offset／已解析fields／UTC及monotonic時間順序差異0；SHA、bytes／完整行／尾片均與host相符，STATUS數、不同RX、首末時間及RAW平均／min／max與各自passive-analysis一致。27及96-byte不完整尾片原樣保存，不補行、不納統計。兩窗host均overflow=false／failure=null，CLOSE／QUIT成功、process exit0、port_released_by_process_exit=true；175443與180106的exit時間分別17:54:43.618及18:01:06.026，支持**各窗終點**COM3已釋放，不將此推成其後埠空閒或候選可部署。

| 公開證據 | SHA-256 |
|---|---|
| 175443 serial | `71B72DA197AA0F3808C9FC0B80B36EC5F0EEC102A9322417E1D2B2ED8AFF0454` |
| 175443 JSONL | `33819753051D08275779D33826019B74CA3D1DCD235F478648C563DCF90896EF` |
| 175443 host | `0C6D1C8A3BCE8B3809F17912DF7435ECE5063E396D50825787FFE20B981CD577` |
| 175443 passive-analysis | `293B13A4AB9F2633806105D2CCEBEBB2796FD13AC5F71BDF2F2095979576A13C` |
| 180106 serial | `92BE7E4E2E7D916546B799E387BA67B25721D118A5E701AB2FBE987C5BDCEA26` |
| 180106 JSONL | `C39B9151E555273F6EFA2CFBF9B4C4DE84EBC7C874DE99D727297C4EEC30FD69` |
| 180106 host | `59196A509D15ECB01E940D6A4559497EF0EBD55BCF089E6EC82B119D378F3F79` |
| 180106 passive-analysis | `D061722489C74092DC602D27949B40D96C856351950195ADC39098BF9407D70D` |

以上175443／180106兩窗初次整理時，Planck僅修改本session，未開COM、讀秘密／建置產物或修改候選，當時沒有新的正式編譯、燒錄或實板QC結果可填；後續編譯結果另列下節。366g校正仍未完成，原因與量測品質前提沒有因被動窗持平而成立。

## 後續 PCNT2m1 窗及已結束的長窗

這兩窗同樣由舊PCNT2m1實板輸出，沒有RMT symbols，沒有新版本／session讀回、TARE／CAL或物件現場操作。當下載重仍未確認；窗間未擷取，不能將前一窗至長窗首尾合成無缺口序列。

| 已完成窗與公開證據 | capture START～END，台北／實長 | STATUS完整收行時間，台北／不同RX | RAW平均／範圍／最大相鄰差，counts | 保存及計數 |
|---|---|---|---|---|
| 180856：[serial](20261028-hx711-rmt-design-passive-180856.serial.log)／[JSONL](20261028-hx711-rmt-design-passive-180856.lines.jsonl)／[host](20261028-hx711-rmt-design-passive-180856.host.json)／[主代理摘要](20261028-hx711-rmt-design-passive-180856.passive-analysis.json) | 18:03:56.752～18:08:56.778／300.026s | 18:03:57.694～18:08:55.794；298／298，RX21062→23849 | −164254.6309／−164370～−164155／196 | 98,372B／596完整行／32B尾片；ERR4／AERR0／FIFO4持平 |
| long182227：[serial](20261028-hx711-rmt-design-long-182227.serial.log)／[JSONL](20261028-hx711-rmt-design-long-182227.lines.jsonl)／[host](20261028-hx711-rmt-design-long-182227.host.json)／[主代理摘要](20261028-hx711-rmt-design-long-182227.passive-analysis.json) | 18:10:20.437～18:22:27.838／727.401s | 18:10:21.097～18:22:26.843；724／724，RX24640→31429 | −164320.1298／−164478～−164173／188 | 239,049B／1,448完整行／128B尾片；ERR4→5、AERR0／FIFO4持平 |

兩窗STATUS全為IDLE／UNCAL／HX及ADXL OK，設備TX皆空；長窗仍捕獲ERR4→5，首次觀察界線18:14:51.133（RX27168／ERR4）～18:14:52.141（RX27177／ERR5）。主機時間界線不是ADC故障精確時刻，HX OK只表示當時最近資料狀態，不能抹除累計新錯誤。PCNT2m1 ring早已凍結，無本次新事件原字／sidecar；沒有≥1000 counts快照跳值也不能判讀框正常或原因已解。

長窗原計畫1200000ms上限，主代理為下一工作階段建立controlled marker後，host於18:22:27.838記錄`PASSIVE_CAPTURE_OPERATOR_END`／`controlled end before next workflow step`並正常保存。不是直接終止程序、串列錯誤或滿20分鐘的完成；實際727.401s與上限分開記錄。host overflow=false／failure=null，CLOSE／QUIT成功、exit0於18:22:27.845，支持該窗已關埠；180856亦CLOSE／QUIT成功、exit0於18:08:56.783。

Planck獨核兩窗原文／line_no／UTF-8 offset／fields／UTC及monotonic時序差異0，bytes／完整行／尾片及SHA與host吻合，STATUS數、不同RX、首末時間、RAW平均／min／max與主代理passive-analysis一致。32及128-byte尾片原樣保留、不補行、不納統計，未讀私有live檔。已完成窗的關閉只描述其終點，不代表後續目標核對或燒錄階段尚未開埠。

| 公開證據 | SHA-256 |
|---|---|
| 180856 serial | `1EB7A89F22CB6C6E513F0C8F51C9EBA74B1DA52AC24BE18CBA3DFFC834E3DCF7` |
| 180856 JSONL | `EFA8F544A954500EBDB03281834D2811098C46F7D8EC834B293C626B03F1C222` |
| 180856 host | `ED047D3270DBEBBAA0831A928D1EEC53195F4DAC1A4FB7DD24FF318873420F29` |
| 180856 passive-analysis | `44C875D56FBF12E01F54E630B219ABDBF26CFE8AFAE21C223707C0427048F95A` |
| long182227 serial | `18C9AB8B974DCE5C5AB835457CD1C80444541EDD44E59A4232F34B16366FB8D1` |
| long182227 JSONL | `28C748B0B157CFF850F3FEE95595FAAC7FE58E8D48A27F897863B50CAFA24D36` |
| long182227 host | `953FC92EFDAD498B745018C2933C755567A7F33C933F98589D2989A68438C301` |
| long182227 passive-analysis | `CF3032AE663872F6903FB7044E5E1839F2AF80EB3FF0C50CDC0AC112723A798F` |

## RMT1 候選正式編譯（部署前歷程）

- [公開來源manifest](20261028-rmt1-source.json)
- [正式compile原文](20261028-rmt1-compile.log)
- [compile主機時間、參數及結果](20261028-rmt1-compile.host.json)

主代理以本機既有設定於18:17:03.045～18:20:30.617台北完成正式編譯，exit0。沿用core3.3.12／原FQBN `esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40`；程式1,026,845／1,310,720 bytes，全域124,080／327,680 bytes、編譯摘要剩203,600 bytes。這是正式連結成功，不是新實板版本、實際runtime heap／WiFi餘裕或RMT接收品質驗證。

Planck獨立核對來源manifest列出的12項公開檔，當前SHA全部相符；其中`05-test.ino`版本為 `v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1-hxfail5-pcnt2m1-rmt1`。compile host的FQBN及專案外build目錄與來源manifest一致，compile log SHA與host指定值相符。秘密header未列入公開manifest；Planck未讀秘密、BIN／ELF／生成來源。公開來源／主機紀錄能支持交付追溯，不能代替私有映像內容的獨立稽核。

| 公開候選來源／編譯證據 | SHA-256 |
|---|---|
| `05-test/05-test.ino` | `5D19D27CF53980FE9DF6C7508EBDDF027CCC0A290A6F7A167B7576321C6FEEF1` |
| `05-test/hx_rmt_adapter.h` | `B1D76BC009C9EF79784DF063C8FA831B2191E3CA1338545AAE1B8121AE747974` |
| `05-test/hx_rmt_diag.h` | `5B72BEBCBB53B4B9F04AB9004121C1608AE10D19CEE1E8EBFC78C0DB3F88DEA2` |
| source.json | `FE338A6011AEA8D9E9F94D1615D3681C0942BA0DC40F508C549778F7A0A1FC9E` |
| compile.log | `DB9308493668F1FEDDF6415439CC35BCA24A98DF08B5A1EEAA66D350135F0EFA` |
| compile.host.json | `D204BB90E117A7771649A03FE5271CB467803AB23DDBFEA5F5B52AFE7B5B8773` |

正式編譯交接當時，最終host工具回歸仍待Gibbs交付，這個階段尚無新目標核對、寫入、首次RMT啟動、symbol／CRC或本版QC。後續完成結果另列下節；候選編譯成功本身不解除HX根因、ERR新增或366g校正前提。

## RMT1 最終主機交付與正式部署

- [host release](20261028-rmt1-host-release.json)／[host manifest](20261028-rmt1-host-manifest.json)
- [Node測試原文](20261028-rmt1-node-tests.log)／[native測試原文](20261028-rmt1-native-tests.log)／[preflight](20261028-rmt1-preflight-review.md)
- [目標identity](20261028-rmt1-identity.log)／[identity host](20261028-rmt1-identity.host.json)／[target](20261028-rmt1-target.json)
- [upload原文](20261028-rmt1-upload.log)／[upload host](20261028-rmt1-upload.host.json)／[部署摘要](20261028-rmt1-deployment.json)

Gibbs已交付Node192／192、native138／138（含actual RMT adapter27）；Planck核對原始測試log與摘要一致，未獨立重跑全部主機測試。native命令有`/wd4505`，不把受抑制的未用static函式warnings稱成完全不存在。主機實際exporter及合成失敗／晚callback案例是部署前證據，不等於實板symbols或根因驗證。

Planck獨立核對source manifest12項公開韌體與host manifest23項tool／test的SHA及檔長全部相符，host release所列公開檔hash亦相符，source／deployment來源對應一致。原pulse範圍的兩個hash差異已由Gibbs釐清：LF正規化489字元、不含postcheck為`B00C0E5BBE74C37CA753FF248A1D03DBA79A0C01D6E21F547A0864412EB55BB5`；同起點含完整postcheck與LF、554字元為`08DFA86D6B694564C4227F9CCE3ABEB51607CC9763BCA848B3BCAF9052BA8E32`。各範圍在PCNT2m1與RMT1均各自相同，不能把不同抽取範圍當來源變動；RMT/API/ISR仍會改變排程。

18:28:19.839～18:29:22.112台北，COM3、ESP32-D0WD-V3 revision3.1、MAC `e0:8c:fe:a8:42:bc`正確。esptool5.3.1於0x1000／0x8000／0xE000／0x10000四區寫入，原文4次Hash verified、exit0，upload log SHA與host指定值相符，沒有NVS／全片清除指令。before／after no-reset留在bootloader，再由主代理先開capture、單次RTS首次啟動；寫入與app執行分開記錄。

| 公開部署證據 | SHA-256 |
|---|---|
| host-release.json | `EBA4DB75BA206FBB2EC282108C5954D30978FB481B541975EB69B563A56AB8BF` |
| host-manifest.json | `DD194C7012612006FE3EAED7739E8EF848C52E2CDE759FD6728C4D0B7CFF74F8` |
| identity.log | `EE7376FAF08E2E11490B2095C61BDDDA852CE1FCEEBEE4BB7A385A3BD9E1A7E0` |
| upload.log | `0EA9248CD69F65E52332DF11969CCB0A7A81708C5035BE45F5FDDD0407FB0DD3` |
| upload.host.json | `BDE95921C4002907D361624A804CB67EDD429DF48AE8AC9DF1BC9CAFD9CF19A1` |
| deployment.json | `913BA1A9B8979A9F51A2191D5DBF103DE49164C82D0A5546AEFE365D5B11A954` |

## RMT1 首次啟動與正常 QC 護欄失敗

[first-boot serial](20261028-rmt1-first-boot.serial.log)／[JSONL](20261028-rmt1-first-boot.lines.jsonl)／[host](20261028-rmt1-first-boot.host.json)保存18:29:36.3388416 assert RTS、18:29:36.5483487 release、DTR false及零設備TX，為本次寫入後單次首次app boot。完整版本`hxfail5-pcnt2m1-rmt1`及schema5／session `8FE7B3DC`已捕獲；ROM `POWERON_RESET`不獨證USB斷電，也不允許沿用舊RAM校正。

實板配置sidecar40960B及RMT4032B成功，RMT setup1、channel0／1、兩driver-computed resolution10MHz，這是driver參數而非實際時鐘頻率量測。新boot初始HX WAIT／RAW NA／RX0，之後正常accepted更新；OLED初無ACK，後18:29:40.5441427 STATUS讀回0x3C。這只是通訊ACK恢復，不是畫面目視通過。FIFO1→2首次出現在18:29:40.5441427～41.5588245，不將啟動現象或ACK變化猜成已知因果。

Planck核對boot9,211B／64完整行／0尾片，raw_line／line_no／UTF-8 offset／UTC與monotonic時序及SHA相符；STATUS fields全符。非STATUS第32行OLED SAFE，JSONL只解析`x4..123`，原文完整`x4..123,y4..59;`保留，與既有parser逗號截token限制一致。host CAPTURE_END18:29:56.3287091、CLOSE18:29:56.3359810、port_closed=true／failure=null，支持該boot窗已釋放COM3。

[183009 QC serial](20261028-hx711-rmt1-postflash-qc-183009.serial.log)／[JSONL](20261028-hx711-rmt1-postflash-qc-183009.lines.jsonl)／[host](20261028-hx711-rmt1-postflash-qc-183009.host.json)僅執行STATUS與HX_DIAG STATUS，讀回proof_count8／good7／bad1／ready0；主代理按guard停止剩餘查詢。**本版功能QC未通過**，不沿用PCNT2m1的條件通過、不重湊正對照或改驗收。

QC檔5,127B／26完整行／0尾片，Planck核對raw／offset／fields／時間／SHA差異0；7 STATUS／7不同RX236→281，IDLE／UNCAL／HX及ADXL OK、ERR0／AERR0，FIFO2→3在18:30:07.647～08.649。WiFi current internal free58332／largest55284／min54708屬有限快照。ADXL_DIAG、HELP、WIFI STATUS及末端診斷皆未送，沒有首末ACC_RX差值或完整WiFi檢核可填；OLED／RGB目視未測。

host failure為`STATUS guard failed; remaining queries withheld`，CLOSE／QUIT成功、process exit0於18:30:08.929、overflow=false。正常清理關埠不代表QC通過。後續同boot隔離取證用於保存失敗，不放寬正常QC或量測前提。

## 同 boot 隔離 schema5 證據與有限 RMT 流

- [183516隔離serial](20261028-hx711-rmt1-failed-proof-evidence-183516.serial.log)／[JSONL](20261028-hx711-rmt1-failed-proof-evidence-183516.lines.jsonl)
- [host](20261028-hx711-rmt1-failed-proof-evidence-183516.host.json)／[analysis-v5](20261028-hx711-rmt1-failed-proof-evidence-183516.analysis-v5.json)
- [Ampere RMT1完整QC](../quality/reports/2026-10-28-rmt1-qc.md)

host明示failed_proof_diagnostic_quarantine=true、measurement_accepted=false，expected／observed session皆8FE7B3DC、run_conflict=null；只送HX_DIAG STATUS及封存後DUMP，沒有reset／TARE／CAL／清資料或WiFi切換。Planck核對176,846B／1,466完整行／0尾片，raw／line_no／offset／fields／UTC及monotonic順序差異0、SHA與host一致；從原文重跑v5 parser，整個結果與封存analysis-v5 deepStrictEqual。

各槽seq2700～2955，共256唯一attempt，255accepted／1rejected，兩槽所有重疊canonical word相同；不是512獨立量測。frozen attempts2955＝2699覆寫＋256保存＋0omitted，overflow0／seq_wrapped0。Ampere另用獨立CRC算法核對所有RAW19word與sidecar20word row、sidecar block `0xF709C0B2`、HXF103word `0x9EA4A8F1`、PCNT405word `0xD7C92D54`，及RMT64＋3×190＝634word block `0x7EFE8AF4`全通。CRC與同框綁定只支持保存資料自洽，不是ADC真值。

### bad proof 與 NA 界線

seq3 accepted `FD7D56／−164522`，SCK50／DATA17，條件期望15；兩unit API／attempt／valid／overflow與GPIO配置符合，bad只由DATA條件差+2造成，未定位多出edge在train或lead-in／tail。seq4同原字DATA15相符，不抵銷seq3失敗。沒有seq3的RMT波形可定位差值，不能猜成毛刺、丟位或唯一根因。

proof8／7good／1bad／ready0保留；256唯一sidecar全部NO_PROOF、usable0／unusable256，原文count與時間均NA。analysis的proof_source_frames256只計政策請求標記，不代表額外執行256個正對照；不能把NA換成SCK0／DATA0或「全部吻合」。PCNT hold亦SKIPPED／NO_PROOF而未取樣，沒有失敗後有效PCNT edge總數。GPIO復原子項通過不等於正對照通過。

### LINK2827 及數位持續時間限制

seq2827 bits`0xFFFAFB`／RAW−1285、原postcheck LOW／rejected，RX2826不增、ERR0→1；前seq2826 accepted−164464，後seq2828 accepted−164437已在hold介入後，不能稱自然恢復。STATUS首次觀察ERR增加在18:34:46.771～47.776，後筆HX已OK仍不消除拒收；原read_start309815906／end309816130是boot CPU時間，不當精確主機UTC。

只有slot0 FIRST_LINK被選，seq／token2827、reason0、兩callback安全複製，driver resolution均10MHz；slot1首DATA模型差與slot2 accepted參考皆未選，因政策依賴usable PCNT但本boot proof_ready0。未選的全零canonical是NA，不是沒有DATA差或已有正常參考。

| RMT接收資料 | 正duration內容，按driver10MHz作nominal換算 | 可支持及不能支持 |
|---|---|---|
| SCK25 symbols／49正runs | 25HIGH2.7～10.5µs、24LOW3.6～8.2µs | 首／末censored，流內沒有≥60µs HIGH不排除截頭、窗外或HX端不同波形 |
| DATA5 symbols／9正runs | HIGH79.9、LOW1.6、HIGH43.9、LOW6.9、HIGH7.4、LOW7.9、HIGH37.0、LOW7.9、HIGH13.8µs | 內部LOW1.6µs是新MCU數位候選；79.9µs是DATA HIGH，不能套PD_SCK的powerdown門檻 |

reason0／bounded_symbols_consistent不證完整輸入波形、物理頻率、跨線相位、遠端clock或供電；zero terminator不轉成1ms。Kierkegaard指出，僅在兩流完整涵蓋同train且正常RX語意成立的受限條件下，DATA1.6µs LOW與「只在SCK rise更新並保持整bit」模型不符；仍不能直接稱雜訊、定位哪次取位miss或確診硬體／軟體根因，缺本版good／bad參考尤其保留。

HXF9個有效離散LOW、2／10／5000µs三點missed／NA，最後點相對anchor253325～253334µs，resume seq2828 start310069819。9LOW不代表連續LOW，較晚離散點與RMT可見前綴不能拼成完整全程波形；PCNT hold因NO_PROOF未提供計數。

### FIFO、同步輸出與隔離收尾

| 時間，台北 | 已觀察資料 | 界線 |
|---|---|---|
| 18:32:21.224 FIFO3→18:32:22.226 FIFO4 | DUMP前新增 | 不歸到尚未發生的DUMP，也不因附近STATUS查詢推因果 |
| 行340，18:35:01.352 BEGIN→行1454，18:35:11.515 END | schema5同步匯出10.163s | 來源明示blocks_polling，保留觀察條件 |
| 18:35:11.650 HX STALE／ADXL MISSING、AGE10317ms、FIFO4→18:35:12.653 OK／OK、FIFO5 | DUMP事後新增 | 與DUMP前3→4分列；恢復OK不等動態完整或故障消失 |

全窗169 STATUS／169不同RX1490→3004、IDLE／UNCAL、AERR0，RAW快照平均−164458.9467、範圍−164567～−164330 counts。CLOSE／QUIT成功、exit0於18:35:16.874、overflow=false／failure=null，支持該隔離窗已釋放COM3；部署及完整匯出通過不解除功能QC與量測品質未通過。

## RMT1 五分鐘後窗與外部 RMT2 候選

[184315 serial](20261028-hx711-rmt-design-passive-184315.serial.log)／[JSONL](20261028-hx711-rmt-design-passive-184315.lines.jsonl)／[host](20261028-hx711-rmt-design-passive-184315.host.json)／[主代理摘要](20261028-hx711-rmt-design-passive-184315.passive-analysis.json)為已部署RMT1的後窗，名稱沿用rmt-design工作流程；與較早PCNT2m1同檔名前綴的窗按時間及部署歷程分開。

capture18:38:15.423～18:43:15.686台北、300.263s；298 STATUS／298不同RX4636→7348，完整收行18:38:16.420～18:43:14.695，IDLE／UNCAL／HX及ADXL OK、ERR1／AERR0／FIFO5持平。RAW平均−164515.7785、範圍−164652～−164380 counts，當下載重未確認，不當空載／校正基準，也不以快照無增量解除bad proof或根因。

Planck核對最終98,373B／596完整行／33B尾片，raw／line_no／offset／fields／UTC及monotonic時序差異0，SHA及最終計數與host相符，RAW統計與主代理摘要一致。CAPTURE_END事件快照98,340B，關閉期間另收33B，最終stored98,373及33B尾片吻合；兩個計數代表不同時點，不當資料損失。尾片原樣保留不納統計，零設備TX、CLOSE／QUIT成功、exit0於18:43:15.699、overflow=false／failure=null。

主代理之後另開18:45工作階段單次最長20分鐘零TX記錄、exec1218及相應stop-marker流程，由root唯一持COM3，該新窗尚未封存；不以本後窗close推當下空閒。RMT2僅在專案外 `rmt2-wip-20261028` 試作保留FIRST_LINK／FIRST_BAD_PROOF／FIRST_GOOD_PROOF及診斷旗標政策，補本次seq3缺波形及無參考的證據缺口；strict proof／RAW／guard不放寬，尚未READY／正式編譯／燒錄。RMT1失敗不被候選構想或後窗暫無錯覆寫，TARE／CAL366仍未完成。

| RMT1新公開擷取 | SHA-256 |
|---|---|
| first-boot serial | `9A87C0171C2305E9130555BED40CE73AF123D3A2E8707BFA351C29AB20630B97` |
| first-boot JSONL | `1F9363D97FE294C8AFA204328BF0C526BC3059297C559CABA531EEC75AB07D9D` |
| first-boot host | `8BDC6A91630C41E4B07F52CAE8BC228BD23C0D8F6D385569D5C2BAE9DA5186BC` |
| 183009 QC serial | `88AD1BB2DEB9DCD8CE0D95DA499437C958F3C298CB3AD69E6C1922044E655D6B` |
| 183009 QC JSONL | `659135DE8AC5542BDD05FE0DA01EF472D2D82F6B03B679DD0ED92CA55E253FDF` |
| 183009 QC host | `1CEC13F1766D315C66D729B28908F5580C5FFF71CC5ECBA79265C18916376159` |
| 183516隔離serial | `EC412B34940CC841AE9693B148E30FCD57A1BF9163103B927D211BFC4366C762` |
| 183516隔離JSONL | `78C7E4BED254448A5725A850D5AC8997A6881282056E075E7D98E30B2A2CB068` |
| 183516隔離host | `D88EC42FCEAF4658AE4B88A065947ED79797567B712C189E68D7F7ED257BD751` |
| analysis-v5 | `A533F6E1380BFDADA0E6581FF46170DBC4E3922142E804B34002DEBCF2735AA7` |
| 184315 serial | `7555CEEC97665C3BD24D756FE3ABE6F722DEE9EE007F7573165C6000352B1800` |
| 184315 JSONL | `91A7A26ACB486B8C2F9624D9968DC2BF5F5393D75451D8622B073D7F8E4A96E0` |
| 184315 host | `56A393C21C46320A37844DBC5D882107D3C9735698D122F1F933BFBE723298B9` |

Planck本次僅更新本session，未開COM、讀秘密／build產物或修改任何韌體／QC／handoff。部署、功能QC、完整性及量測品質分層記錄，Git結果由主代理整合。
