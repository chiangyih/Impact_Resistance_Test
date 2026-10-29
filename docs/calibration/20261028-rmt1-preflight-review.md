# RMT1 候選獨立 preflight review（Ampere）

狀態：**具體來源 preflight 與限定主機測試已核，兩項實質阻擋已修；尚待最終來源／host驗證固定、原FQBN正式編譯與目標部署，未實板 QC。** 本檔只屬儲存庫外候選；不能取代已部署 PCNT2M1 的 QC。主代理統一 COM3 操作，Gibbs 唯一實作韌體／測試／工具；Ampere 只審核此候選公開來源與證據，不讀 WiFi 秘密或含憑證建置產物。

使用者已授權持續自動診斷、必要修改後編譯／燒錄及立即新版 QC；本次 RMT 評估仍不代表 HX 根因已確診、允許盲改原 RAW 或已完成 CAL366。

## 本機官方 SDK 契約與可聲稱範圍

已讀 Arduino ESP32 core 3.3.12 對應 `esp32-libs/3.3.12/include/esp_driver_rmt/include/driver/rmt_rx.h`、`rmt_types.h`、`rmt_common.h`、`hal/include/hal/rmt_types.h` 及 ESP32 `soc_caps.h`：

- `rmt_receive()` 非阻塞；完成結果由 `on_recv_done` 提供，callback 在 ISR 環境。
- `edata` 的生命只在 callback 內；若 loop 使用必須複製必要 metadata／symbols，不能保存 edata 指標到之後使用。
- RX event 有 `received_symbols`／`num_symbols`／`flags.is_last`，沒有獨立 overflow／全部輸入未丟失的證明欄位。
- `rmt_disable()` 的公開 header 沒有承諾返回後所有晚 callback 都已排空。沒有可核對的 driver 同步證明時，timeout 後重用同buffer／ctx再以「當前token」接受 callback 會有跨框錯綁風險。
- 每個 symbol 原始32-bit包含兩個15-bit duration與兩個level。10MHz nominal tick為100ns；它是MCU時鐘解析度，不是經外部校正的脈寬精度或所有窄脈衝可見保證。
- ESP32共8個RMT通道、每通道64word是資源上限；板載RGB初始化已使用RMT TX，不能僅憑總數保證兩RX一定可配置。須逐API檢查並實板核對資源／GPIO。

RMT只能觀察MCU數位接收端的有限level-duration序列，不能代替HX711端電壓、地、供電、有效clock電平／setup-hold或ADC真值。雙RX啟動非同步，CPU呼叫括弧不等於硬體閘門共同起點；不得把兩個symbol流直接以100ns精度互相對齊，亦不得按RMT重建字覆寫原RAW。

## 必須解除的部署阻擋條件

| ID | 必要契約 | 主機／來源可驗證項 | 實板仍必驗 |
|---|---|---|---|
| RMT-PF01 | GPIO25保持原SCK輸出、GPIO33保持原DATA輸入；RX driver造成的direction／pull變更必須復原並讀回 | BASE／DRIVER／READY／EVENT保存function／route／OE／OECTRL／drive／pull／IE白名單；配置或復原失敗不得盲繼續clock | 本版四階段原始讀回，PCNT原正對照；不以ACK冒充腳位安全 |
| RMT-PF02 | 兩RX、兩live buffer及有限事件slots全有或明確降級；RAM固定有界 | 每一步配置／enable／callback註冊／第二unit失敗、釋放或留存策略；無loop malloc／無無限重試；RAW與PCNT原資料仍保留 | setup前後及WiFi後INTERNAL8BIT free／largest／minimum，requested／allocated／state／attempts |
| RMT-PF03 | callback不能把舊job當新seq；buffer生命覆蓋所有可能ISR使用 | 使用獨立buffer identity／不可變generation；timeout／API不明後terminal-disable RMT直到新boot是可接受保守方案，保留buffer／ctx不free。測晚callback、重複callback、seal後callback、錯buffer／錯unit | 正常完成／callback有證據；極端路徑無故障注入則未測 |
| RMT-PF04 | 禁止把partial／timeout／滿容量／無callback當完整波形 | 檢查pointer、0／64／>64 symbols、is_last0、奇異symbol／內部零duration／非法level、最大寬度終止與API成功分開；overflow未知明列 | 實際num_symbols／flags／終止狀態讀回；64容量邊界不能宣稱完整 |
| RMT-PF05 | 每個job確實屬一個原HX完成attempt，不能混入下一train／hold／callback覆寫 | prepare／read_start／read_end／receive_done／finish各CPU括弧；原SCK loop／RAW接受不變；下一train到來但舊RX未完時明確終止／失效，不將混框流綁新seq | 與RAW／PCNT同seq／版本／session、原read窗／新開銷，兩RX完成順序 |
| RMT-PF06 | 首LINK與首DATA模型差兩事件不互相搶占，固定後不回寫 | 事件slot各自政策、同框重疊去重、先差後LINK／先LINK後差／同框雙事件／總期限／不重新武裝；loop可持續RAW | 真事件或如實未觸發；既有PCNT2M1事件不能代本版 |
| RMT-PF07 | symbol只支持已觀察有限內容，不能聲稱未丟邊緣／精準跨通道同步 | 保存原symbol32-bit及ticks；valid／complete／timeout／truncated／overflow_unknown分欄；起末idle及首transition anchor未知保留 | 本版原脈衝數／level-duration、CPU與PCNT包絡及差異如實列出 |
| RMT-PF08 | 匯出資料不可變且完整性可獨立重算 | 新外層schema／profile，不讓舊parser靜默忽略；每事件metadata＋symbols canonical CRC、NA與0分開；外層END、session／seq一致；主機正常＋各失敗實際exporter fixture | 原文／JSONL／主機時間與保存bytes／SHA、row／block／外層完整、同步DUMP影響另列 |

初稿時PF01～PF08均待實作；下方具體審查記錄列出目前已核部分與剩餘條件。只有通過必要主機測試、原FQBN正式連結成功、正確目標寫入／校驗成功後才進實板QC。主機guard／模擬可證程式分支，不證實體波形或根因。

## 建議的明確失敗分類

至少分列 `NOT_CONFIGURED`、`ALLOC_FAILED`、`SETUP_API_FAILED`、`GPIO_RESTORE_FAILED`、`START_FAILED`、`BUSY_SKIPPED`、`CALLBACK_PENDING`、`CALLBACK_MISMATCH`、`TIMEOUT`、`PARTIAL`、`CAPACITY_AMBIGUOUS`、`CLOSED_NO_EVENT`、`SEALED_COMPLETE`；名稱可依實作調整，語意不可併成0或成功。RMT不可用不等於HX RAW為0；RMT trace完整也不等於HX資料正確。

若timeout或不可確認停止造成terminal-disable，後續可保持原RAW／PCNT診斷；但GPIO復原／SCK輸出安全無法確認時必須維持原sticky clock guard，不為持續RX偽造恢復。封存資料與之後runtime晚callback計數分開，不能在匯出期間改變已標完整payload／CRC。

## 本次審查界線

初稿時只有公開候選骨架；後續已進行下方具體審查，尚未將主機證據擴張為實板通過。未修改主儲存庫來源、既有PCNT2M1 QC、校正、COM或供電。由主代理完成剩餘正式建置／目標核對與部署；每次真正燒錄仍須立即獨立新版QC。

## 具體來源審查與兩項已修阻擋

已讀候選 `hx_rmt_diag.h`、`hx_rmt_adapter.h`、主ino新增hook、schema5／RMT parser、actual adapter native案例及兩個actual-exporter產生器。候選目前使用三個固定事件：首LINK、首DATA條件模型差、首accepted／PCNT一致參考；後者可保留shape／capacity不確定的前綴，不宣稱正常波形。

1. **UNBOUND造成永久pending（已修）**：原service在 `!read_bound` 時永遠return。原read開始跨600秒／20秒邊界而未實際append至RAW時，主hook不bind，導致close之後仍無法封存整個bundle。修正加入 `discardHxRmtBinding()`／`HXR_NOT_BOUND`；close或明確discard時可在callback／timeout後有界結束，不發明RAW／事件。native涵蓋done後close無bind、close前pending再timeout、discard後done；actual adapter＋實際dump也確認未綁定及晚callback不改封存資料。
2. **EVENT GPIO故障未進當框reason（已修）**：原碼只設global terminal／clock guard，當框仍可能reason0而被summary稱bounded consistent。現於EVENT readback失敗／route改變時，將GPIO原因綁當前work或該NOT_ARMED事件，保留舊reference不可變；parser亦反向核對EVENT stage，不能以READY階段代替。native新增arm後改route再bind及EVENT讀回失敗兩條路徑。

另兩個編輯中問題已修：null RMT heap＋真HXF LINK不再被schema5 parser要求虛構首LINK trace；新增mode5的actual exporter產生器曾有字串SyntaxError，完成修正後已由Ampere重新生成／編譯／解析通過。這些中途失敗不能當作最終測試通過，但也沒有部署到實板。

## Ampere實際主機核對

- Actual adapter native已獨立以MSVC `/W4` 編譯並執行，當時27案例全部通過，`HxrState=4016`／`HxrRuntime=4080`（此為x64主機尺寸，目標靜態斷言／runtime另驗）。出現5條既有PCNT未用函式C4505；沒有將此run記成0 warnings，也不視為已定位韌體缺陷。Gibbs之後新增案例須以最終host清單另記。
- 由實際ino函式重新抽取的整體exporter（函式hash `124C4A0D289DF103922348B88AC038925CF154B9D862ACF42C7BB16828FEEDD9`）已獨立生成並編譯：正常schema5、sidecar FIRST／SECOND／reserve失敗2／3／4之schema3、以及RMT heap null但保留真LINK之schema5全部可由候選parser解析。每槽RAW256；null RMT為setup2／三event未選，沒有虛構symbols。
- Actual adapter＋實際 `dumpHxRmt`（函式hash `35B489EF0993779C01F2B50E4E049AEAFC238AB23EBF9B2475093AAB0D2BEC63`）五模式：timeout／null／unbound／unbound-timeout後晚callback／wrong-buffer，全部輸出可解析；每模式第二次dump與第一次byte-exact相同，保留晚ISR buffer生命且不重arm。
- 原PCNT2M1主儲存庫與候選的 `delayMicroseconds(2)`→25pulse→原postcheck區塊，正規化LF後逐字相同，SHA256 `08DFA86D6B694564C4227F9CCE3ABEB51607CC9763BCA848B3BCAF9052BA8E32`。RMT/API/ISR及hook在外側仍改變排程，不以區塊相同聲稱整體取樣零擾動。

以上是指定來源時點的主機證據；若最終函式hash改變，actual fixtures須重新生成並重跑。Ampere沒有讀取含憑證建置，也沒有將credential-free SDK link probe代替本機含私密header但不公開內容的正式目標編譯。

## 官方實作補核與保留限制

ESP-IDF5.5.5非DMA RX_DONE處理在callback前停止RX、清done事件並複製buffer；HW RX_ERROR可被driver清除，沒有在event內附帶錯誤旗標。因此正常buffer重用還依賴這個固定SDK路徑，token並非硬體回傳generation認證，不能宣稱能抵抗任意跨兩代同地址偽造callback。timeout／不明路徑不再arm、保留ctx／buffer終身，是另外的保護。[官方rmt_rx.c](https://raw.githubusercontent.com/espressif/esp-idf/v5.5.5/components/esp_driver_rmt/src/rmt_rx.c)

官方測試在RX先arm後送單HIGH脈衝，回傳流可由HIGH開始；不要求初始LOW或末尾正duration的idle。因此已取消硬性51正run且首末LOW的假設；0duration marker不換算成1ms。保留原始symbols及所有正HIGH encoded段，即使整trace不一致也標censored／format資訊，不能抹除前綴陽性內容。[官方test_rmt_rx.c](https://raw.githubusercontent.com/espressif/esp-idf/v5.5.5/components/esp_driver_rmt/test_apps/rmt/main/test_rmt_rx.c)

手動extern的兩個driver helper簽名與官方 `rmt_common.c` 定義吻合：channel-id輸出int、resolution輸出uint32；它們不在公開common header，也不在官方private header宣告。必須固定SDK5.5.5並通過真正連結；讀回是driver計算分頻結果，不是物理頻率校驗。[官方rmt_common.c](https://raw.githubusercontent.com/espressif/esp-idf/v5.5.5/components/esp_driver_rmt/src/rmt_common.c)

目前的terminal表示不再開始新RX；timeout後不呼叫非同步停止／釋放，舊硬體job仍可能晚完成。`pending=0`／`closed=1`只表示應用層診斷已封存，不證兩硬體RX已停止。安全完成unit的symbols可複製；未完成／錯ptr／錯token／partial／重複callback的unit不讀buffer，固定零只作canonical佔位、不是量測0。

## 本時點PF結論與剩餘門檻

PF01～PF08的來源守衛與上述正常／降級／timeout／晚事件主機路徑已具體核對，**目前未見尚存的已確認來源安全阻擋**。兩項實質缺陷已有修正及主機覆蓋；最終Node mutation負測、完整host驗證及來源清單仍由Gibbs固定後交付。之後主代理須用原FQBN正式編譯／正確MAC寫入校驗，並立即新版本QC；不能因本preflight直接宣稱已部署或HX根因解決。

實板必查：自然boot／完整新版本與session、INTERNAL8BIT heap及WiFi後餘量、實際兩channel／driver-resolution、五階段GPIO、PCNT正對照、同seq有限RMT symbols與所有NA原因、完整schema5 CRC／END／封存不回寫，以及新增FIFO／同步DUMP干擾。真API失敗／timeout／容量／晚callback未實板注入者仍標未測，目視、HX端電氣／供電與366g校正前提仍待驗。
