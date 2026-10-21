# 2026-10-21 HX711 校正快照封存

- [逐筆 STATUS／RAW CSV](2026-10-21-hx711-status.csv)：200 筆觀察、35 欄，UTF-8 BOM、CRLF；不刪除重複回報。
- [原始文字輸出](2026-10-21-hx711-source.log)：本次對話中 10 段未截斷的 COM3 命令輸出，包含 TARE／CAL 成功回應及主機開關埠訊息。
- [完整操作背景及結論](../../handoff.md#hx711-calibration-20261021)。

這是從已保存的歷史輸出補存，不是重新讀取 COM3；資料來自當時已燒錄的 `v1-layout2`，不是 `v1-layout2-fifo-diag1`。後者在封存之後才另行燒錄，見[部署紀錄](../../handoff.md#fifo-diag1-deployment-20261021)；本 CSV 未混入後續燒錄前後的新觀察。日期為 Asia/Taipei 的 2026-10-21。

## 階段與筆數

| `stage_id` | 條件 | STATUS 筆數 | 同階段重複 RX |
|---|---|---:|---:|
| `initial_status` | 首次查詢，外加荷重未確認 | 13 | 0 |
| `tare_pre` | 使用者確認空托盤，TARE 前 | 4 | 1 |
| `tare_post` | TARE 成功，仍為 UNCAL | 4 | 1 |
| `cal400_pre` | 400g 已放置，CAL 前 | 3 | 0 |
| `cal400_post` | CAL 400 成功，400g 保留 | 5 | 1 |
| `unload1` | 第一次卸載，保留托盤 | 13 | 0 |
| `unload1_settle` | 第一次卸載後追加觀察 | 28 | 0 |
| `repeat400` | 同一批 400g 重放 | 16 | 0 |
| `unload2` | 第二次卸載，保留托盤 | 16 | 0 |
| `unloaded_drift` | 排查期間空載觀察 | 46 | 1 |
| `version_check` | 空載版本／設定查詢 | 6 | 0 |
| `clearance_followup` | 使用者確認紙張可穿過、間隙 5mm 後 | 46 | 0 |
| 合計 | 10 段原始輸出，TARE／CAL 各拆前後 | 200 | 4 |

## 欄位與換算

- `observation_id`：全檔觀察順序；`stage_order`：原始輸出段落順序；`stage_observation`：同階段觀察順序。不是 ADC 逐框序號。
- `raw_counts`、`rx_count`、`hx_error_count`、`hx_status`、`cal_state`、`age_ms` 及 ADXL／FIFO／接腳等欄位，均逐筆解析實際 STATUS；保留原始計數，不強制歸零。
- `raw_status_line`：包含原有 `HOST_SAMPLE` 前綴（若有）的完整原行；`source_tool_id` 與 `source_line_number` 對應 `.log` 中同一段原始輸出的 1-based 行號，不是整份合併 `.log` 的全檔行號。
- `expected_added_mass_g`：依當時使用者確認，空托盤為 0、20g × 20 個為 400；首次未確認時留空。此欄不是獨立秤量或可追溯重量認證。
- `tare_offset_counts`：只在 TARE 成功後填入 `-162747.625`。`counts_per_g`：只在 CAL 成功後填入 `9.977421875`。
- `derived_mass_g`：只在已 CAL_USER 的 176 筆，以 `(raw_counts - tare_offset_counts) / counts_per_g` 在主機換算；不是裝置直接輸出的克數。UNCAL 的 24 筆留空，不能當成 0g。數字保留雙精度有效位，顯示的小數位數不代表量測精度。
- `duplicate_rx_in_stage=1`：同階段前面已有相同 RX 的快取回報，原始 RAW 已核對相同。共 4 筆，仍完整保存。若統計要排除重複，可篩選 0；200 筆觀察中只有 196 個不同 RX。
- `host_elapsed_ms`：僅兩段 45 秒觀察有原始主機經過時間，共 92 筆；其餘留空。它是各段內的經過時間，不能跨段串接為連續時軸。`age_ms` 是裝置回報的讀值年齡，不能當作採樣時刻。
- 沒有逐筆絕對時間戳；`.log` 的 `turn_started_utc` 僅為對話回合開始時間，**不是**該段開埠時間或各筆觀察時間，不能推算未記錄的時刻。

這些是約每秒的 STATUS 快照，並非所有 HX711 轉換框。TARE／CAL 各使用的 32 個新有效框沒有逐框列印，無法事後還原；不得把本檔當成那 64 框的原始清單。原 handoff 中 CAL 後 5 筆平均保留重複回報，而第一段 45 秒統計採 45 個不同 RX；重算歷史數值時須沿用相同口徑。

校正後回零與重放仍有偏差，這 200 筆封存的累計 ERR 最後為 2，不能因快照為 HX=OK 就宣告所有中間框正常。封存本身不修改裝置 RAM 係數，也不代表校正合格。後續燒錄前另觀察到 ERR=7；燒錄後 RAM 係數清除、裝置為 UNCAL，這些另存於部署紀錄，不追寫歷史 CSV 或把重置後 ERR=0 當成修復證明。
