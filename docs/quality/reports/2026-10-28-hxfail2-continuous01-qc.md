# hxfail2 授權持續有界續錄第01輪獨立觀察（2026-10-28）

本輪依使用者最新授權，保持USB插接，持續以同版hxfail2有界重啟／記錄，目標取得HX失敗後DATA，釐清原因後才完成366g。**本輪沒有新韌體修改、編譯或燒錄，不是新燒錄QC。** 歷史部署見[hxfail2 QC](2026-10-28-hxfail2-qc.md)，前兩輪分別見[restart1](2026-10-28-hxfail2-restart1-qc.md)／[restart2](2026-10-28-hxfail2-restart2-qc.md)，不能用舊QC代替新session。

主代理唯一操作COM3與reset；Ampere只讀公開證據並撰寫此報告，未開埠、重置、讀秘密或私有build。**結論：單次reset、自然boot／版本／session `7E5D072F`、雙槽到期封存及完整bundle匯出子項通過；無LINK，DATA實際取樣仍未觸發。** 符合保存後按既有授權續下一輪有界診斷的控制前提，下一次reset前仍需新鮮guard；這不是量測或校正通過。計數重新開始或有限窗口無觸發不代表原異常已修，保持UNCAL與校正暫停。

## Boot、控制與完整性

[serial](../../calibration/20261028-hxdual-continuous-01-boot.serial.log)、[host](../../calibration/20261028-hxdual-continuous-01-boot.host.json)、[逐行JSONL](../../calibration/20261028-hxdual-continuous-01-boot.lines.jsonl)。先前session的公開bundle已保存；本輪新boot不覆寫舊原文、不沿用RAM校正或跨session合計計數。

| 控制／資料 | 獨立核對結果 |
|---|---|
| OPEN前設定 | COM3／115200／8N1／Handshake None、DTR=false／RTS=false；本輪時間均為台北UTC+08，實際OPEN為12:24:41.9746800。 |
| reset前guard | 12:24:45.5226039通過，先收到4筆IDLE／UNCAL／HX與ADXL OK、RX14564→14590／ERR0／AERR0／FIFO2；舊epoch依既有保存及操作追溯，不將這4筆STATUS稱為新查session。 |
| 單次RTS | assert12:24:45.5239822、release12:24:45.7387854，各一次；DTR均false。單調間隔214.8165ms是主機控制界線，非實測管腳脈寬。 |
| 收尾 | CAPTURE_END12:25:00.7461046、CLOSE12:25:00.7535125，host port_closed=true／failure=null。此boot capture已關，主代理另持COM等待雙槽。 |
| 原始完整性 | 7722 bytes與host相符，SHA256 `84306A4AE5D2BB8C519E686662F9087F973A724558BA27566F9C45DB5F4C48BC`。50個RX_CHUNK從offset0連續涵蓋全部位元組；59完整行、無尾片。 |
| JSONL映射 | Ampere核對原文、行號、byte offset及每行LF最後位元組所屬chunk的完整UTC／單調時間，差異0；chunk順序一致。時間是主機接收映射，非ADC、GPIO或boot事件的精確UTC。 |

host記錄中沒有裝置文字命令TX；RTS切換為本次已授權reset，與一般唯讀窗不切控制線分開。主代理操作條件為USB未拔、保持供電；未獨立量測供電波形，不能由ROM reset原因推斷外部USB斷電。沒有TARE／CAL／UNCAL／ARM／STOP／CLEAR／WiFi切換或新flash。

## 本輪自然boot與新epoch

- JSONL11於12:24:45.7704729收到`rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)`，原樣記錄，不推電氣原因。
- JSONL24於12:24:46.6704360收到自然版本`v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1-hxfail2`及UNCAL／SCK25 DATA33，runtime仍是既有版本。
- JSONL28於12:24:46.8260943收到HX_DIAG_READY schema2／session `7E5D072F`／slots2／window600000000／record_bytes56／rings_bytes28672／no_auto_cal1。session配合reset、banner與RX重新開始識別新epoch，不當永久唯一ID；READY接收時間不當裝置窗口起點。
- OLED啟動ACK0x3C、ADXL READY／IDENTIFIED、RATE0x0A／FIFO0x9F；INIT PRE_N32／SRC83→POST_N0／SRC81。PRE是在重設定前取得，不能視為本MCU epoch新OVR；ACK／地址與OFF_REQ不代表OLED／RGB實際目視通過。
- 新boot首STATUS為HX OK／RX1／RAW−163371／AGE131，後續至RX126均HX／ADXL OK、ERR0／AERR0／FIFO1。WiFi由WAIT／CONNECTING到CONNECTED／radio1／IP192.168.52.71；不推外網或射頻時序。
- HXF初態WAITING／captured0／next0／valid0／missed0，尚無實際DATA點。沒有資料不填LOW／連續LOW或ADC ready；新boot前的RAM係數不沿用。

## 本輪待核與可信度界線

主代理交付收集器新增「session不符、額外boot、phase／CAL意外或STATUS消失即停止後續reset」守衛，真LINK出現後仍等待封存。此為操作計畫交付，實際守衛執行情形須由本輪完成host／原文核對，不以這份boot檔預先填通過。

## 完整bundle、session守衛及關埠

[serial](../../calibration/20261028-hx711-hxdual-continuous-observe-123455.serial.log)、[逐行JSONL](../../calibration/20261028-hx711-hxdual-continuous-observe-123455.lines.jsonl)、[host](../../calibration/20261028-hx711-hxdual-continuous-observe-123455.host.json)、[control](../../calibration/20261028-hx711-hxdual-continuous-observe-123455.control.log)、[分析v2](../../calibration/20261028-hx711-hxdual-continuous-observe-123455.analysis-v2.json)。

capture為12:25:55.055～12:34:55.397，實際540.342秒；planned620000ms是上限，主機開監控時間不是裝置600秒窗口起點。208838 bytes、1373完整行＝537 STATUS＋537 WiFi＋254 HXRD＋12 HXFP＋33其他行，無尾片。SHA256 `A6BA166C6365C3294325A59BCBCE408E17AD144DB16D50FB02C61BB6504F215B`與host一致；Ampere獨立核對原始位元組、完整行offset、行號、全部已解析欄位及UTC／單調順序，差異0。

僅HX_DIAG STATUS（12:25:58.556）及HX_DIAG DUMP（12:34:47.379）兩條唯讀TX；先於12:34:46.702收到schema2／slots2／all_frozen1通知。外層BEGIN_HX_CAPTURE為12:34:47.423，完整END_HX_CAPTURE為12:34:50.008，跨度2.585秒；兩槽及唯一HXF順序正確，最外END complete1／retained1／read_only1齊全，不以單槽結束冒充完整匯出。

host expected_session／observed_session均`7E5D072F`、run_conflict=null；Ampere另掃本capture所有session欄均一致，沒有額外boot／READY／自然banner、所有STATUS phase=IDLE／CAL=UNCAL，無RX倒退。完整STATUS最大主機間隔2752.2986ms，出現在同步DUMP，不能解讀成ADC週期；本窗沒有觀察到所列狀態衝突或STATUS長期消失。這是守衛在正常本窗的追溯，不代表已注入故障驗證每個拒絕分支。

OPEN前115200／8N1、DTR／RTS off與實際control DESCRIBE吻合，OPEN／CLOSE／QUIT OK、process exit0、stderr0、overflow=false／failure=null，主代理已關閉並釋放本bundle capture。工具ACK不是ESP32 ACK。

## 雙槽、CRC及未分類筆數

| 檢核 | 本輪結果與限制 |
|---|---|
| 身分／政策 | 同版hxfail2／session `7E5D072F`，ANY mask7、LINK mask1，與本輪自然boot一致。 |
| 期限 | 兩槽start_us516288、window600000000，deadline600516288；freeze_us600544419，reason3 WINDOW／export_sealed1，CPU跨度600.028131秒、晚28.131ms，非硬即時或GPIO凍結瞬間。 |
| 固定收支 | 各scope=frozen attempts5586=pre_overwritten5458+records127+omitted1，overflow0／seq_wrapped0，trigger_flags0／pre0／post0／post_complete0；無效trigger零欄不造事件。 |
| 保留原字 | 各seq5459～5585共127筆，accepted1／後檢1／flags0／ERR0，RX5458→5585；首start586909477、末end600463691µs，保留跨度13.554214秒。 |
| 獨立完整性 | 兩槽共254列19-word CRC、補數解碼、RX／ERR增量、seq及gap連續性獨立核對差異0；127個重疊seq逐word相同，254列只有127個不同attempt，不是兩次獨立ADC量測。 |
| CPU括弧 | read187～212µs，126個跨列開始gap15625～215058µs；首列前一attempt不在保留窗。CPU值不驗SCK高寬／有效邊緣、ADC時刻或漏樣量。 |
| HXF／捕獲可信度 | CLOSED_NO_LINK／mode4／stop_reason3／captured0／resume_valid0，metadata及resume為NA，12點valid0／missed0、DATA與實測時間為NA。103-word CRC獨立重算`0x7E8831FE`吻合；分類為「無DATA」，本輪沒有LOW hold，不判連續LOW或ADC ready。 |

依已部署header的順序，先記完成seq，再Tick(record.start)，若到期即omitted++return，flags分類在其後。本次末保存seq5585的end600463691早於deadline52.597ms；封存5586含1筆越窗，freeze/start600544419已在deadline後28.131ms。依程式路徑與收支，seq5586未進LINK／SAT／JUMP分類，其原字及flags未保留，不補造正常或異常。

因此未觸發規則的範圍最多是seq1～5585（5584次相鄰accepted比較），不能說5586全部分類過。未保留的5458個字也無法離線逐字驗真，不涵蓋未ready poll、未完成attempt或全部ADC轉換。v2工具從本窗serial重跑exit0，整份JSON物件與保存分析一致；CRC／解析通過不代表原ADC取得字正確，舊近零／巨大字及漂移仍未解。

## 快取重印及DUMP警示

537 STATUS只有536不同RX646→5638、無倒退；唯一重印RX5592，12:34:47.395／12:34:50.147的RAW同為−163394，ERR0／AERR0／FIFO1相同，AGE15→2766ms。符合DUMP前後未新輪詢的快取重印，不重複計新ADC框，也不把STALE改成資料更新通過。STATUS RAW範圍−163485～−163236為約1Hz快照統計，不是全部原字或靜態精度。

最外END後JSONL1362於12:34:50.147為HX STALE／ADXL MISSING／FIFO1；JSONL1364於12:34:51.149恢復HX／ADXL OK、FIFO變2，其後至末筆維持2。同步DUMP阻塞為已知軟體行為，這些觀察與匯出擾動相容，不能回因或改寫較早封存資料。沒有本輪新ADXL_DIAG定位第二來源，不推已知漏樣量或與HX根因混同。

| ID／狀態 | 後續獨立核對 |
|---|---|
| HXF2-C01-Q01／P2／部分通過 | 雙槽到期封存／CRC／127個重疊唯一attempt及完整bundle通過；5586含1筆越窗未分類，只有前5585受flags判斷。 |
| HXF2-C01-Q02／P2／無DATA | HXF CRC及NA呈現通過，但本輪captured0／12NA，無LINK拒絕綁定、實際LOW點位或resume可驗；真DATA與分離事件路徑未觸發，不代表舊異常已修。 |
| HXF2-C01-Q03／P2／警示保留 | 初始化FIFO1，DUMP後STALE／MISSING、FIFO1→2；與DATA介入及HX根因分開，不清警示或推已知漏樣量。 |
| HXF2-C01-Q04／P1／量測用途未驗收 | 舊近零／巨大接受字及漂移未解，UNCAL；不得因無觸發、CRC通過或捕獲部分DATA直接CAL366、沿舊比例或修RAW。 |

## 續輪前提及收尾

本輪原文／時間／CRC、身分與狀態追溯及關埠核對通過，沒有觸發所列停止續輪的衝突；可依使用者已有授權，在下一次新鮮IDLE／UNCAL guard通過並確認舊證據已保存後，繼續一次有界重啟診斷。不代表量測可用或366g校正可開始；下一輪如session不符、額外boot、phase／CAL意外或缺狀態，應保存並停止後續reset，由主代理判讀。

本輪boot及bundle兩段capture均已關；新輪須另記session，不跨輪合計無觸發次數。Ampere未持有COM、僅更新本觀察報告，未改工程檔／原始資料，未commit／push。
