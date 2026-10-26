# 初步量測系統：離線資料流程驗證

本輪先補[主機解析工具](../../tools/preliminary-measurement.cjs)，沿用第五版RAW／CSV，不新增GUI或硬體。工具讀取本機文字並產生離線JSON報告，不開埠、不控制裝置；硬體仍暫停。2026-10-26已完成下述離線程式檢查，不代表已取得新試驗資料或通過量測驗收。

## 範圍與驗收界線

- 現可先開發／驗證：DUMP解析、結構與筆數核對、缺值／品質保留、來源標記及可替換校正重算。
- 恢復後才驗：通訊、實際版本／RAM、空載基線、校正與獨立靜態驗證、時序／動態能力及低負荷實物流程。
- 不得據程式測試或完整報告宣稱硬體已正常、量測準確、真實瞬時力／衝量／吸能量合格，或正式材料排名成立。

## 離線CLI與輸入

以下從專案根目錄執行；尖括號為待替換的既有檔案路徑，不是已存在的試驗檔。來源必選synthetic（人工測試）、historical（歷史重放）或new（真實新資料）；標記new本身不驗證資料真實性或新鮮度。

```powershell
node tools/preliminary-measurement.cjs --help
node tools/preliminary-measurement.cjs --input "<人工測試DUMP文字檔>" --source-kind synthetic
Get-Content -LiteralPath "<歷史DUMP文字檔>" -Raw | node tools/preliminary-measurement.cjs --stdin --source-kind historical
node tools/preliminary-measurement.cjs --input "<DUMP文字檔>" --source-kind synthetic --calibration "<同來源profile.json>" --output "<尚不存在的報告.json>"
```

--input與--stdin必須二選一；--calibration、--output可省略，預設報告寫stdout。既存輸出檔拒絕覆寫。完整離線匯出退出0；不完整／資料錯誤退出2，仍輸出檢查報告；來源、profile或CLI配置錯誤退出1。

檔案輸入及profile必須為一般檔案；裝置名稱與裝置命名空間拒絕使用，不用本工具讀串列埠。

輸入是[第五版dumpTrial()](../../05-test/05-test.ino)的單一trial匯出，不是STATUS快照或校正CSV。可在BEGIN前保留同trial的「# TRIAL id STATE reason=... A=n H=n ...」作預期筆數；其後須有「# BEGIN trial=id state=... reason=... CAL=... ...」、下列完整header、ACC／HX資料列及「# END;...」。不要從歷史STATUS／OLED照片拼造DUMP。

```text
kind,t_read_us,x_raw,y_raw,z_raw,x_g,y_g,z_g,mag_g,hx_raw,hx_status,mass_g_static
```

ACC與HX分組、非對應欄位保留原NA；t_read_us是相對ARM的讀出時間，不是真實採樣時間。工具不自動找首次接觸、停止／分離窗口或真實力峰值。原檔與來源日期另存，不能將重放時間當量測日期。

## 報告如何判讀

| 欄位 | 使用規則 |
|---|---|
| schema_version／source_kind／source_sha256／trial | 報告格式版本為1；保存資料類型、這次輸入的SHA256及trial識別／狀態／原因。trial_id不是跨開機全域ID |
| integrity.structure_valid／issues | 缺BEGIN／END／header、重複trial、欄數／NA錯誤、NaN、負時間、同感測器時間倒序、RAW越界、ACC／HX欄錯配等列error；看code、severity、message定位問題 |
| integrity.expected／observed | 分別核對ACC／HX預期與實收列數；不符列error。沒有# TRIAL預期計數則warning，export_count_verified=false且complete=false |
| integrity.export_count_verified／complete | 結構與筆數通過只支持離線匯出完整；即使完整也可能是FAULT試驗，不等於物理事件完整、ADC零漏樣或指標有效 |
| quality | hx_errors／acc_errors／fifo_full／sat／hx_stale／acc_stale／ignored_edges保留原值，scope為frozen_trial_delta；不是事後STATUS累計值。SAT是HX框統計；缺值用null，不能補0 |
| recorded_metadata／records | 保留原始註記（含列印的校正參數、NEAR_RANGE等）及可解析的ACC／HX列與來源行號；原註記不自動成為有效校正。原輸入檔仍另存，錯誤列由issues定位 |
| measurements.acc | count與sampled_magnitude_peak_g／sampled_axis_abs_peak_g僅為已取樣整段資料摘要，不是主事件的有符號軸向比力峰值；缺資料用null |
| measurements.hx | 分列count、non_sat_count、RAW極值、recorded_mass_g_static_*原記錄值及recalculated_mass_g_*外部重算值；均非動態力認證 |
| validation | 固定stage=offline-dataflow-only；hardware_verified、measurement_accepted、material_comparison_accepted均為false，不能因退出0改稱驗收通過 |

## 可替換校正profile與來源綁定

預設calibration.mode=none，recalculated_mass為null；不從DUMP欄位或歷史資料自動套回係數。CAL_USER的原記錄重量僅留在recorded_mass欄，仍未獨立驗證。外部profile只產生另列重算值，模式為external_unvalidated，保留原始RAW與原記錄重量。

| JSON必填欄位 | 條件 |
|---|---|
| profile_id | 明確識別這份profile，不等於校正合格證明 |
| offset_counts／counts_per_g | 有限數值，比例不得為0；參數來源、日期、適用輪次與驗證狀態另記實驗紀錄 |
| source_kind | 必須與該次分析的synthetic／historical／new完全一致 |
| input_sha256 | 必須精確等於該次輸入的source_sha256；檔案／文字一旦改變須重新確認綁定 |

先對既有輸入不帶profile產生報告，取得source_sha256，再依明確來源建立JSON profile。缺欄、非有限值、零比例、來源或SHA不符均拒絕。管線可能改變文字編碼／換行，綁定應以工具實際接收輸入的報告SHA為準；有profile時優先用--input保持同一輸入。

重算公式為(RAW－offset_counts)／counts_per_g，只是靜態等效克數；SAT或ADC rail不重算，結果為null。歷史profile只能綁歷史輸入，不能把離線200g修正係數當新試驗校正；新資料須有適用的新校正來源。工具不替使用者證明參數有效，也不寫回裝置。

## 程式測試與狀態

2026-10-26以Node.js v24.11.0執行：新增工具56項測試全通過；既有第五版18項與FIFO診斷19項主機檢查通過。另以既存新版HX711靜態STATUS歷史log作反例，工具退出2且不判為完整試驗DUMP。可從專案根目錄重現：

```powershell
node --test tests/preliminary-measurement.test.cjs
node tests/05-test.test.cjs
node tests/05-fifo-diag.test.cjs
```

測試須涵蓋完整／截斷／錯誤schema、預期筆數缺失或不符、缺值／飽和、來源與SHA綁定，以及CLI退出碼與拒絕覆寫。成功只支持所涵蓋的離線程式行為，不支持實體時序或精度。

測試使用明示SYNTHETIC的內嵌資料，另有一般檔案／profile／報告的暫存整合測試；原輸入與既有輸出不覆寫。測試曾發現顯式傳入錯誤profile可能靜默退回none，現已改為拒絕，並保留回歸測試。現有靜態STATUS歷史資料不足以驗證完整動態擷取，沒有把它轉造成實際落下資料。

## 恢復前資料與後續順序

現在只需依記憶或既有紀錄補兩項歷程：①最後量測後是否斷USB、重啟或燒錄；②是否改線、移動感測器／托盤、調螺栓或改變線材牽引。未知保留待確認，不要求現在通電、量測或拍新圖；這些回覆不等於恢復操作授權。

獲准恢復後：確認現場／RAM與可靠讀取 → 保存並排查靜態空載RAW基線 → 建立當次校正及獨立升降載／回零／漂移驗證 → 完成必要時序／動態能力檢查與低負荷實物通過流程 → 初步小量測。允許誤差、穩定門檻、負荷與樣本數仍待確認；低負荷也不能略過量測能力及安全條件。

依[規格範本](trial-spec-template.md)、[紀錄表](trial-record-template.md)填三層驗收；可沿用[規格草案](drafts/2026-10-23-trial-spec-draft.md)與[紀錄草案](drafts/2026-10-23-trial-record-draft.md)，缺口見[待補資訊清單](drafts/2026-10-23-information-needed.md)。單項無效不刪整片，性能失敗與峰值無法定量分開記錄；正式力與材料效果主張須另有證據。
