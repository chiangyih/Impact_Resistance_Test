# 手動位移速度測試紀錄（optical1首筆成功已保存）

更新日期：2026-10-26（Asia/Taipei）。本頁分列ARM試跑、燒錄重啟、19:19等待歷史與20:45首筆成功讀回；操作見[手動光學計時說明](manual-optical-guide.md)。

## 範圍與最新決定

使用者先選擇沿用ARM，隨後明確更正為獨立手動光學計時模式，以最後指示為準。只用同一反射標記依序上→下通過，不進行自由落下、荷重校正或材料驗收。原ARM整合記錄模式保留不變。

| 項目 | 已知證據／狀態 |
|---|---|
| 有效觸發間距 | 使用者回報220mm；幾何量測方法與不確定度待確認 |
| 未觸發 | 使用者OLED確認U0 D0 |
| 僅上方觸發 | 使用者OLED確認U1 D0 |
| 僅下方觸發 | 使用者OLED確認U0 D1；上方移開後回0 |
| 本輪極性 | 依上述目視確認進入電位為1／1；重接後已設定並讀回POL 1 1 |
| 本輪距離設定 | 重接後已設定並讀回DIST 220；設定讀回不代替幾何驗證 |
| 首筆DeltaTime／速度 | id1 DONE：1153289µs＝1153.289ms；串列0.190759m/s，OLED回報0.1908m/s；單次流程與換算有限通過，精度未驗 |

## 連線與操作歷程（已發生）

1. 18:19被動讀取COM3失敗，.NET回報裝置失去作用；[初次log](20261026-181947-untriggered.log)只有主機錯誤，不是電位驗證。
2. PnP為OK仍不等於可開埠；18:27以CLI、115200、DTR=off／RTS=off嘗試仍報Invalid serial port，見[CLI log](20261026-182723-cli-monitor.log)。上述兩次未送POL／ARM。
3. 使用者確認IDLE且無需保存資料後重接USB；CLI以DTR=off／RTS=off恢復通訊，見[重接後原始log](20261026-183044-cli-monitor-after-usb.log)。此後已讀回DIST220、POL1/1，並執行兩次ARM。
4. 兩次均為FAULT／UPPER_TIMEOUT。使用者說第一次未看到ARM，第二次尚未動作；這些回報不證明感測器漏觸發，也不補造通過時間。
5. 燒錄前的CLI工作階段98955已由主代理Ctrl+C關閉；其後已另行燒錄重啟，現況見下節。本頁作者未開COM3，不將舊trial當作重啟後仍保留的RAM資料。

| 歷史ARM紀錄 | 已保存原檔與離線報告 |
|---|---|
| trial 1：FAULT／UPPER_TIMEOUT；A=947、H=110 | [DUMP](20261026-trial-001-upper-timeout.dump.csv)／[JSON](20261026-trial-001-upper-timeout.dump.csv.json) |
| trial 2：FAULT／UPPER_TIMEOUT；A=948、H=109 | [DUMP](20261026-trial-002-upper-timeout.dump.csv)／[JSON](20261026-trial-002-upper-timeout.dump.csv.json) |

兩份JSON皆為`source_kind=new`、`integrity.complete=true`，只表示該FAULT試驗的DUMP結構與預期筆數完整；不表示速度有效、ADC零漏樣或量測合格。A／H是ACC／HX記錄列數，不是手動通過次數或材料試片數；兩份均無有效速度。

## optical1部署與19:19等待歷史

主代理後來完成新版編譯、燒錄校驗及RTS重啟；前兩次ARM FAULT與原檔保留為燒錄前歷史，沒有因此變成有效速度，也不沿用其RAM狀態或校正。

| 項目 | 本版證據／結論 |
|---|---|
| 主機檢查 | 主代理回報215項通過：新模式43項＋既有172項；不是實板量測驗收 |
| 編譯 | core 3.3.12；FQBN `esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40`；程式963545 bytes、RAM 93056 bytes |
| 燒錄 | [upload log](../../quality/2026-10-26-optical1-upload.log)：完整Hash校驗成功、exit=0並由RTS重啟；不等於完整QC通過 |
| 實板版本 | [新serial log](../../quality/2026-10-26-optical1-serial.log)的唯讀`VERSION`回應為`v1-layout2-fifo-diag1-wifi1-d220-right-optical1`；不是自然boot banner |
| 啟用前讀回 | normal phase=IDLE、CAL=UNCAL、HX／ADXL=OK、ERR／AERR=0、FIFO_FULL=1；DIST220、POL1/1，未將FIFO警示結案 |
| 19:19讀回（歷史） | 已`SPEED START`；當時最後讀回仍id=1、state=WAIT_UP、reason=NONE、A=0／H=0 |
| 當時尚無通過數值 | WAIT_UP原文為`upper_boot_us=NA lower_boot_us=NA dt_us=NA speed_mps=NA`；原文保留，後來DONE數值見下節 |
| 等待觀察 | [主機時間與觀察證據](../../quality/2026-10-26-optical1-wait-observation.md)：19:16:56／19:17:23兩次查詢仍同id1 WAIT_UP，約27秒間隔支持超過10秒上方等待；不是GPIO精確時間或速度驗證 |
| 19:19關埠（歷史） | 19:19:36.9589316+08:00讀主機時鐘後查SPEED STATUS仍WAIT_UP；隨後Ctrl+C關CLI41132，19:19:38.6346547+08:00再記時。當時COM3釋放、無常駐monitor；工具exit=1為主動中止監控 |
| 當時MCU與保存 | 未送STOP／CLEAR、未重置，保留獨立WAIT_UP；主機關埠後MCU仍可接收標記及計時，後來已重新讀回並存檔，見下節 |
| QC | [本版報告](../../quality/reports/2026-10-26-optical1-qc.md)由Ampere持續更新；完整QC待補，不以首筆成功代替精度／重複性／防護驗證 |

## 首筆成功與最新關埠（20:45讀回）

| 項目 | 已保存證據／本輪結論 |
|---|---|
| 來源 | [讀回log](20261026-optical1-first-transit-readback.log)／[SPEED原文](20261026-optical1-001.speed.txt)／[觀察與重算JSON](20261026-optical1-001.speed.txt.json)；JSON不是原ADC DUMP報告 |
| 識別／狀態 | 紀錄ID `20261026-optical1-001`；唯讀VERSION仍optical1全字串；裝置id1、DONE／NONE、DIST220、POL1/1、A0／H0、ignored_upper_edges=0 |
| 裝置時間 | `clock=boot_micros`；上1265178878µs、下1266332167µs，差1153289µs＝1153.289ms |
| 原值與重算 | 串列`speed_mps=0.190759`；依220×1000／1153289重算0.1907587777m/s；僅兩點平均速度，不是獨立時間校正 |
| READY／動作目視 | 使用者原文「已看到 READY，先準備」及「是，同一標記先上後下」；非本頁作者親見 |
| OLED結果目視 | 使用者原文「1153.289，0.1908」；單位依先前提問對應ms／m/s，與串列數值的顯示取位一致；實際現場觀察精確時間待確認 |
| 主機觀察時間 | 查詢前20:45:07.1952325+08:00、回應後20:45:08.8249183+08:00；不是GPIO邊緣或物理通過時刻 |
| 最新關埠／RAM | 主代理Ctrl+C關CLI67270、COM3已釋放；20:46:03.3504259+08:00為關埠後記時。未CLEAR／STOP／START／reset／flash；裝置DONE結果保留，未啟新輪 |
| 有限判定 | 同標記單次流程、時間差與換算／取位一致性有限通過；精度、重複性、實板失敗防護及完整QC仍待補，沒有獨立校正合格結論 |

## 獨立模式規則（已部署，完整QC待補）

| 項目 | 最新需求 |
|---|---|
| 指令 | `SPEED START`／`SPEED STATUS`／`SPEED DUMP`／`SPEED CLEAR`及`STOP` |
| 啟用前提 | IDLE、無校正收集、已驗POL、距離>0、兩頭均未觸發、radio停止成功 |
| 準備 | 開始顯示READY，進入WAIT_UP無準備倒數；可從容準備同一標記 |
| 計時 | 等上方進入後，須於30秒內到下方；小於200µs作防錯，不是精度門檻 |
| 感測隔離 | 不用HX／ADXL校正或資料；等待／計時不刷新OLED、不輪詢ADC、不占原記錄buffer |
| 完成／停止 | 完成保留DT／V；STOP保留手動FAULT與原因，不清校正或原ARM trial |
| 匯出／重試 | 先SPEED DUMP存檔，再SPEED CLEAR重試；radio保持暫停到SPEED CLEAR |
| 自動釋放 | 無自動釋放；不是撞擊試驗模式 |

距離220mm時，`v = 0.220 / Δt_s = 220 / Δt_ms` m/s，只是兩點平均速度，不能當撞擊瞬間速度。30秒及200µs是流程邊界，不是已驗證準確度或材料驗收標準。

## 待補與下一步

| 項目 | 狀態 |
|---|---|
| 新模式部署／版本 | 已燒錄成功並取得唯讀VERSION；自然boot banner未在本輪serial log保留，不混稱啟動擷取 |
| 完整部署後QC | 待補；依本版報告持續補證，不沿用舊版結論 |
| READY、DT／V目視與首筆SPEED匯出 | 已有使用者回報及原文／JSON；未另取得照片或現場精確觀察時間 |
| 首筆時間／速度及單次流程 | 已保存1153.289ms、串列0.190759m/s；有限通過，不等於準確度驗收 |
| 重複性／實板失敗防護 | 尚未驗證；未把單次成功外推為所有分支通過 |
| 光學延遲／抖動、有效距離與不確定度 | 待確認；不因獨立模式而視為已校正 |

最新讀回為id1 DONE，原文與JSON已保存，主機已關埠、裝置結果未清、未啟新輪。需重試時依既有授權先確認檔案已保存，再SPEED CLEAR；本次只更新文件，未執行清除、開埠或Git。後續先由Ampere更新本輪QC，再依需求安排重複性、獨立時間／距離及防護驗證；兩次ARM FAULT與19:19 WAIT_UP歷史保持原樣。
