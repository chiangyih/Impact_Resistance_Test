# HX711逐次讀取診斷：離線候選設計（2026-10-28）

狀態與範圍
本文件只有設計，沒有實作、編譯、燒錄、開COM、讀秘密或修改既有韌體。
目前保留供電與UNCAL；現版仍沒有下述逐框介面。不得把設計中的欄位或命令當成現在可讀。
本設計若以新韌體燒錄方式實作，必須重置並建立新執行epoch，不能保留目前的執行中RAM狀態；目前明確不部署、不燒錄、不重置。
是否部署須遵守使用者目前保持供電及狀態的要求；歷史係數另存不能當成跨重啟可安全套回的證明。
此處「讀取框」指pollHx一次通過DATA低檢查後的25個軟體脈衝嘗試，不等於獨立ADC轉換。

1. 已知公開程式契約與目前資訊上限
- 目前24個digitalRead位元累積成bits；decodeHx只遮24位及作二補數解碼，RAW未套去皮或比例。
- 第25脈衝後只檢查DATA是否HIGH；失敗只增ERR，不寫RAW/RX/hx_last_ms。失敗的bits及嘗試時間沒有輸出。
- STATUS為快取；自然約1Hz，查詢另印。沒有attempt序號、GPIO邊緣紀錄、SCK高低時間、HX專用poll gap或ADC硬體時間。
- 同一主迴圈serviceSerial可處理多個STATUS，中間不pollHx；實際同RX的RAW理應一致，AGE可增長。
- 現有DUMP只接受DONE/FAULT。不得捏造ARM/STOP試驗讀參數；那也會改背景負載及WiFi條件。
- VERSION/ADXL_DIAG能留識別與快取，但不能補回HX失敗框。HEX可由RAW & 0xFFFFFF導出，沒有獨立新增資訊。
- 雙STATUS冗餘可查快取/輸出一致性；若未重現異常，不能當已修復。即使兩次同壞RAW，仍未分出ADC輸入、GPIO捕捉、記憶體或相關通道異常。

2. 現有介面可做的最後一層分辨
在新鮮IDLE/UNCAL/HX OK時送一組STATUS換行STATUS換行，完整保留兩條回應及自然輸出。
每組14輸入bytes，位於32-byte單圈服務預算內；傳輸分段仍可能使兩條分圈，須以實際RX驗證。
- 同RX、同RAW：與相同快取重印相容，不是兩個獨立ADC樣本。
- 同RX、異RAW：不能只用真實載荷/ADC輸入改變解釋，須查公開程式契約不一致、記憶體、格式/串列/主機路徑；不可平均修正。
- 同一壞RX重印壞RAW：支持壞字已在快取/格式化前存在的假說；不能完全排除相關傳輸損壞或重放。
- 不同RX：資料已前進，無法用它回查上一壞字的內部來源。
可低頻加VERSION作固定字串參照；固定輸出異常也可能是身份/通道/記憶體問題，正常固定字串不保證動態RAW正確。
所有TX都是額外負載；不補發積欠命令，異常後只保留已送回應與有界被動恢復資料，不清佇列。

3. 未來候選記錄欄位（目前不存在）
以固定大小RAM記錄，開始時分開定義poll與transfer attempt。
- transfer_attempt_seq：每次實際進入位元讀取遞增，包括最後失敗的嘗試。
- accepted_rx_before/after、err_before/after、accepted、SAT、phase、cal_mode等。
- 讀取開始/結束CPU時間、相鄰attempt gap、距最近接受框的間隔；等待DATA高返回的poll次數可先作累計。
- bits24：原始累積字，成功/失敗都保存；軟體發出脈衝數、起始DATA、最後資料樣本及第25脈衝後DATA。
- 可選max_sw_high_us、max_sw_low_gap_us；進階模式另存25個HIGH及25個LOW區間或50個軟體邊緣時間。
- schema/version、公開來源雜湊、建置設定、boot/session ID、診斷模式及有效旗標。
上述時間均為CPU觀察，不是實際管腳波形或ADC轉換時間。25次軟體寫入不證明HX收到25個有效邊緣。
必要時只導出校正的valid旗標與double位元/17位有效數字；不可傾印全RAM/flash以免包含WiFi秘密。
係數另存不等於重啟後可安全恢復，更不能把不同零點epoch視為同一校正。

4. RAM與事件保存
低開銷摘要目標約64 bytes/attempt，256筆約16 KiB；實作前必須用sizeof及可用heap重新核算。
完整50個uint32時間約200 bytes，加約56 bytes標頭，目標約256 bytes/attempt；128筆約32 KiB。
可分固定前/後事件區，遇ERR、SAT或候選大跳先保留前資料，再收固定筆數/時間後凍結。只凍結診斷ring，不改量測phase或校正。
若attempt恰為10/80次每秒，256筆約25.6/3.2秒；故障重試可能快於ADC率，不能保證這些秒數，須用實際時間戳回算。
若未凍結可覆寫最老診斷筆；凍結後不得默默覆蓋，溢位、丟紀錄與不完整事件必須明示。
主機保留原文、去重表、事件前後窗、雜湊及無法判定項。位移比對先驗同RX一致，再對不同RX序列比對k=1..23。
現版沒有ring下載命令；未來即使提供，也應是唯讀下載，不自動CLEAR/TARE/CAL或變更背景設定。

5. 干擾預算與不能承諾的精度
不能在SCK迴圈、critical section或每個bit直接printf/Serial輸出。
新增50次左右的micros呼叫就會改變讀取；若單次額外成本假設1/5us，約增加50/250us每attempt，這是算例而非實板量測。
在80 attempts/s，該算例約增加0.4%/2% CPU時間；若成本落在HIGH期間，也會直接改HIGH寬度，可能讓原現象改變或消失。
不得因診斷而把整個傳輸鎖成長critical section，或順手改delay、RATE、I2C、WiFi/OLED、接腳或校正門檻。
可先只摘要開始/結束與錯誤，將逐邊緣量測列為第二模式；每種模式都是不同觀測條件。
115200/8N1理論上限約11520 bytes/s。100 bytes/attempt在80 attempts/s已約8000 bytes/s，尚未含自然STATUS及格式成本。
256 bytes/attempt在80 attempts/s約20480 bytes/s，超出此線速；每次全量即時輸出不可行。
因此應先存RAM，再於明確凍結後另行匯出；匯出期間資料不代表原來無輸出的排程條件。
實際heap、時間函式成本、UART阻塞及診斷能否不溢位，均為未測，不能用編譯通過代替。

6. 協定與故障分層
未來可在記錄固定後加入record_seq及CRC，主機檢查漏記/重排/輸出後資料損壞。
有效CRC僅支持該記錄形成之後的完整性；不能證明ADC字、GPIO捕捉或CRC計算前的RAM正確。
無效CRC也不能單憑一項判UART硬體壞，還需區分格式、截尾與主機保存問題。
既有RAW右移特徵只是事後候選：FFFDAE=-594、FFFF64=-156；不能自動左移還原或濾除。
完整讀取時序若違反規格，仍須分開列CPU觀察及真實管腳波形的差別，不能從AGE代替。
若時序看似正常且字仍異常，純軟體記錄仍不能排除供電、ADC本身或類比輸入瞬變。

7. 純主機驗證與實板驗證分開
可用既有純主機模型檢查24-bit往返、兩個位移候選、同RX衝突、重複RX造成三列掃描漏判、時間回繞及截尾/CRC狀態。
最小字向量：000000、7FFFFF、800000、FFFFFF、FDAEBA、FDAEEB、FFFDAE、FD9276、FD926E、FFFF64。
主機模型通過不代表GPIO時序通過，更不代表故障根因或366g精度驗收。
目前已定位的是資料品質異常及介面觀察限制；未定位可確認的軟體根因。不得只因一窗未見異常恢復校正或修改門檻。
在不新增觀測點的約束下，現有介面可能無法完全區分供電/ADC/GPIO時序；應清楚報告資訊上限，避免無限重複相同窗口。

8. 已保存現有證據（皆為主機收到完整行的時間，非ADC或實際故障瞬間）
- 20261027-hx711-366g-repeat-223853.serial.log：22:34:37.182/38.188/39.192，RAW -151878/-594/-151829，RX141351/141358/141367，AGE163/252/151ms，ERR均9。中值與雙鄰右移8位相容，非已證實根因。
  SHA256 DD0E26A43DFE3B45188136837617122EF5501FFE92F057776E3F1F08D45F005A。
- 20261028-hx711-366g-fresh-empty-precheck-071224.serial.log：07:12:17.517/18.520/19.523，RAW -159114/-156/-159122，RX425983/425990/426000，AGE165/250/149ms，ERR仍31；到20.524/21.529才31→32。該窗零TX、IDLE/UNCAL/HX OK；中值與雙鄰右移10位相容。
  SHA256 D6FE9376D4219C882FA9D007AC77572880E96E585C05C0EF59A6BFDD4EA29CEE。
- 20261028-hx711-uncal-fault-continuous-074914.serial.log：零TX；07:41:17.977/18.981，ERR32→33、RX441952→441961、RAW -160360→-160419、AGE205→199ms；無同窗大跳。故ERR與可見尖峰不呈已證實的一對一關係。
  SHA256 D63A4F5F62EBA47AF3FC8B9DC51E48EFE379BD9759AC900DF8C50FB7EAC53346。
- 20261028-hx711-uncal-dense-status-aba-075717：B 1110條STATUS，1348行/1284不同RX，RX跨度2149，約59.75%只指已接受RX的快照覆蓋。64組同RX重印均RAW一致；ERR33/AERR2/FIFO0不增。無再現不表示故障消失。
- 20261028-hx711-uncal-paired-status-aba-082438：19批write、38邏輯STATUS；277行/253不同RX，19個重複RX群RAW一致（最多3次）。B開始後約9.77秒觀察ERR36→37後停止排新查詢。08:21:48.382/.410同RX464181、RAW -161282、ERR37，AGE85/108ms。
  無request ID，不能認領上述哪一行是具體query回覆；後一重印只可稱與已送pair/在途輸出相容。先前零TX窗已出現ERR增加，不能將此ERR歸因查詢。
  SHA256 52BA01F81BDEDB5CB213EFA750C4E0A755EAFDA5224475EB999209969C0B6E6F。
上述檔案位於專案docs/calibration/，原始證據與JSONL/host分開保留；無法由這些欄位自證實際SCK管腳波形、供電或真ADC輸入。

參考（公開來源）
- 本機05-test/05-test.ino：decodeHx、pollHx、printStatus、processCommand、serviceSerial、loop。
- AVIA HX711原廠資料表（SparkFun公開託管）：https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
- 已完成的純主機模型：irt-hx711-bit-model-20261028-gibbs.cjs及其.result.json；不操作裝置。
