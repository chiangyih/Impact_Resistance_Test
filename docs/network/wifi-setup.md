# WiFi設定與驗證界線

最新 `v1-layout2-fifo-diag1-wifi1-d220-right-optical1` 已編譯、自動燒錄並由唯讀VERSION讀回，新增[獨立手動光學計時](../experiments/manual-speed/manual-optical-guide.md)。啟動SPEED前STA連線有IP；進入後暫停radio，直到SPEED CLEAR才解除暫停，手動畫面沒有WiFi圖示屬預期。後續首筆已DONE，1153.289ms／0.1908m/s與原文換算相符；20:46主機COM3已關閉，未STOP／CLEAR或啟新輪，結果保留時WiFi仍暫停。完整QC、光學準確度與重複性仍待補，見[首筆補充](../../handoff.md#manual-optical-first-result-20261026)。

前版-right的WiFi右上圖示（x115..123／y4..9）曾由使用者回覆「圖示與文字均正常」，詳見[前版部署與討論](../../handoff.md#wifi-icon-right-and-team-review-20261026)及[22秒續讀證據](../quality/2026-10-26-wifi1-d220-right-qc-resume-serial.log)。此為前版目視，不代替本版手動畫面、RGB或所有顯示情況驗證。供電、感測時序與量測精度尚未驗收。

## 檔案與憑證

- 連線邏輯由[network_wifi.h](../../05-test/network_wifi.h)提供，與第五版感測流程整合；本文件不改接線或控制落體釋放。
- 使用者提供的私人憑證已存入Git忽略的 `05-test/wifi_secrets.h`，不在文件、範例、序列輸出或公開紀錄填入真實SSID／密碼。未提供本機私密檔的checkout預設停用WiFi。
- 可公開的[wifi_secrets.example.h](../../05-test/wifi_secrets.example.h)只使用 `YOUR_2G4_SSID`、`YOUR_WIFI_PASSWORD` 占位文字；另行建置時先複製為同目錄的私密檔再於本機設定，不列印秘密。
- 私人憑證原始檔、可能展開憑證的生成來源，以及BIN／ELF等建置產物不得公開、附入問題回報或提交Git；忽略憑證檔不表示生成產物不含秘密。
- `.gitignore`已加入精確私密檔規則；本輪建置產物放在專屬系統暫存目錄，未放入儲存庫。忽略與未追蹤狀態另行檢核。

## 接入條件

ESP32以STA身分連到既有AP；AP須提供相容的2.4GHz網路。實際AP頻段、驗證方式、路由器設定或手機熱點的2.4GHz設定均待使用者確認，不由雙頻名稱或既有網路可見性推定相容。

本輪僅提供連線／IP狀態，沒有網頁、HTTP服務、OTA或遠端命令；不能透過WiFi開始量測、校正或釋放落體，也不代表新增了電磁鐵控制功能。

## 連線策略與量測互斥

STA採狀態機輪詢，不加入等待連線的while／delay迴圈。首次啟動前關閉持久化，將憑證留在RAM，並關閉核心自動重連。待機每250ms至多輪詢一次，連線逾15秒或掉線先停radio，再隔30秒重試；重試次數未設總上限，可用WIFI OFF停止。計時以32位無號差值處理溢位。

WiFi核心初始化與radio停止API仍可能同步等待，不能稱為零延遲或完全無阻塞。程式在任何啟動嘗試前先保守標記radio可能開啟，部分啟動失敗亦須成功停用才可進入校正／記錄；停止失敗時拒絕開始。待機的自動停止重試相隔至少30秒。

| 裝置階段 | 網路服務規則 |
|---|---|
| IDLE且未收集校正資料、未處於SPEED模式 | 才可服務網路、處理下列WiFi命令；輪詢不得加入阻塞等待連線的迴圈 |
| TARE／CAL資料收集 | 進入前先停止radio，收集期間不服務網路、不接受WiFi命令 |
| ARMED／WAIT_DN／CAPTURE | ARM進入前先停止radio；活動期間不輪詢、不重連、不服務網路 |
| 原ARM DONE／FAULT | radio及網路服務保持暫停；CLEAR回到IDLE後才允許依待機策略恢復服務 |
| SPEED WAIT_UP／WAIT_DN／DONE／FAULT | START前radio須停止成功；整個手動模式不服務／重連網路，保留結果期間亦維持暫停；保存SPEED DUMP後SPEED CLEAR才解除此暫停 |

以下為已實作命令介面；失聯、停止失敗等情境的實板QC仍待補，操作範圍依使用者授權：

| 命令 | 用途／限制 |
|---|---|
| WIFI STATUS | 僅IDLE且非校正收集時查連線／IP狀態；不是感測器或校正驗收 |
| WIFI ON | 僅IDLE且非校正收集時提出連線要求，由待機輪詢處理 |
| WIFI OFF | 僅IDLE且非校正收集時要求停止radio／連線並取消自動重試；停止失敗會回報FAILED，不能當成已關閉 |

保留既有感測讀取、中斷、RAM記錄、校正與釋放邊界。STATUS的radio欄為軟體的保守標記，並非射頻量測。DUMP新增WIFI_POLICY註記，CSV欄位不變。活動期間停止網路不能反推待機期間完全無擾動，也不能以ERR=0或無新增FIFO警示宣稱零漏樣。

## 編譯與驗證紀錄

2026-10-26初次wifi1本機檢查及獲准後的部署如下；該次證據見[序列紀錄](2026-10-26-wifi1-serial.log)與[handoff部署節](../../handoff.md#wifi1-deployment-20261026)。其後220mm修正部署以本文件開頭及所連第六十七節為準。

| 檢查項目 | 結果／證據／日期 |
|---|---|
| 來源版本、編譯環境與編譯結果 | v1-layout2-fifo-diag1-wifi1；ESP32 core 3.3.12；FQBN esp32:esp32:nodemcu-32s:UploadSpeed=115200,FlashFreq=40；編譯通過，程式959741／1310720 bytes、全域RAM 93000／327680 bytes；COM3燒錄及Hash校驗成功 |
| 主機邏輯檢查、重試／timeout及命令狀態限制 | [WiFi測試](../../tests/05-wifi.test.cjs)38項與[OLED測試](../../tests/05-wifi-oled.test.cjs)41項通過；既有韌體18、FIFO診斷19及離線工具56項亦通過，均不驗證實體時序 |
| 私人憑證與生成產物未公開／未納入Git的檢核 | 精確忽略規則及未追蹤狀態通過，公開文字檔憑證掃描通過；建置產物僅在系統暫存專屬目錄 |
| 實機STA連線、IP取得、掉線及限速重試 | 約10秒擷取含12行CONNECTED及非零IP；未實測掉線、重試、長期穩定或網際網路可達性 |
| OLED連線圖示（最新right版） | 快取為CONNECTED、有有效IP且radio可能啟用時，於右上x115..123／y4..9畫九乘六像素圖示；第一行文字自x4起。使用者已確認本次圖示與文字正常；斷線／停止後清除圖示僅有主機測試，未作實板切換驗證。早先wifi1／d220版左上角位置保留在歷史紀錄 |
| 校正／ARM前radio停止、活動期與DONE／FAULT互斥 | 待確認 |
| WiFi開／關的供電、基線、時序與感測品質對照 | 待確認 |

WiFi會改變系統供電與執行時間條件；獲准恢復後須重新核對供電餘裕、空載RAW／回零、HX與ADXL資料品質、FIFO／中斷時序及適用校正，不能直接沿用無WiFi時的驗證結論。API回報停止成功不證明基線立即穩定；本候選未增加停機穩定門檻或排除跨切換形成的首筆HX轉換。驗證時應保留切換與新資料的時間關係；實際允許誤差、穩定時間與比較門檻仍待確認。

前次wifi1的11行STATUS為IDLE／UNCAL／HX=OK／ADXL=MISSING，初始化診斷ACK53=0及ACK1D=0；本次wifi1-d220待機窗口已回報ADXL=OK、FIFO_FULL=1，舊紀錄原樣保留。未調查缺失轉為可讀的原因，短期待機OK與零交易錯誤不代表量測驗收；極性、校正與FIFO警示仍待驗證，目前不可直接進入撞擊比較。

現行接線依[README接線圖與接線表](../../README.md#wiring)，量測界線見[CSV與資料限制](../../README.md#data-limits)。本次授權僅涵蓋燒錄與待機連線，ESP32留在IDLE與WiFi連線；感測排查、校正與試驗尚待另行安排。
