# hxfail2 授權持續有界續錄第02輪獨立觀察（2026-10-28）

本輪接續[第01輪已保存觀察](2026-10-28-hxfail2-continuous01-qc.md)，依使用者授權保持USB插接，以同版hxfail2重啟及有界收集，目標取得HX失敗後DATA，釐清後才366g校正。**沒有新韌體修改、編譯或燒錄，不是新燒錄QC。** Ampere只讀掃敏後公開證據並撰寫本報告，未開COM、重置、讀秘密／私有live目錄或build。

**結論：可信捕獲LINK拒絕後9個有效離散DATA點，另3個目標明確missed；雙槽事件封存、原始完整性、CRC及LINK／profile綁定通過。** ANY先封存後LINK仍取得事件的本版路徑得到實板證據。這不是完整12點覆蓋或連續波形；同輪ANY仍接受異常RAW−320，量測資料品質未通過。捕獲目標已達，停止後續循環reset並保留session `A1C20320`；原因未釐清，保持UNCAL及366g校正暫停。

## 本輪boot與控制

[serial](../../calibration/20261028-hxdual-continuous-02-boot.serial.log)、[host](../../calibration/20261028-hxdual-continuous-02-boot.host.json)、[逐行映射JSONL](../../calibration/20261028-hxdual-continuous-02-boot.lines.jsonl)。以下均台北UTC+08。

| 控制／保存 | 獨立核對 |
|---|---|
| OPEN前 | COM3／115200／8N1／Handshake None，DTR=false／RTS=false；OPEN12:37:16.5660971。 |
| guard | 12:37:20.0931559通過，先收到4筆IDLE／UNCAL／HX及ADXL OK、RX6930→6959、ERR0／AERR0／FIFO2；舊epoch依前輪保存及操作追溯，沒有在這4筆STATUS另查session。 |
| 單次RTS | assert12:37:20.0939968、release12:37:20.3036600，各一次且DTR均false；實際單調間隔209.6824ms是主機呼叫界線，非實測管腳脈寬。 |
| 收尾 | CAPTURE_END12:37:35.3320348、CLOSE12:37:35.3345118；host port_closed=true／failure=null。 |
| 原文 | 7718 bytes與host相符，SHA256 `3DC210A82E0F5BFB2864501521EF43CCD4688504C8B4737D13A3EA30853A62B8`；52個RX_CHUNK連續涵蓋全部位元組，59完整行、無尾片。 |
| 時間映射 | Ampere核對原文、行號、byte offset、每行LF最後位元組所屬chunk的完整UTC與單調時間，差異0；chunk順序一致。主機接收時間不是ADC、GPIO或boot事件精確時刻。 |

RTS控制為本次已授權reset，與一般唯讀窗不切線分開；host沒有裝置文字命令TX。USB保持插接是主代理操作條件，無獨立供電波形；未TARE／CAL／UNCAL／ARM／STOP／CLEAR或切WiFi，沒有新flash。前輪公開原文／報告保留，不跨session沿用RAM校正或合計計數。

## 新自然啟動與狀態

- JSONL11於12:37:20.3489783收到`rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)`；如實列為裝置訊息，不推USB曾斷電或電氣原因。
- JSONL24於12:37:21.2161736收到hxfail2自然版本banner及UNCAL／SCK25 DATA33；JSONL28於12:37:21.3999428收到schema2 READY、session `A1C20320`、slots2／window600000000／record_bytes56／rings_bytes28672／no_auto_cal1。結合reset及計數重新開始識別新epoch，隨機session不是永久唯一ID；READY接收時間不是裝置窗口起點。
- OLED啟動ACK0x3C，ADXL READY／IDENTIFIED、RATE0x0A／FIFO0x9F，INIT PRE_N32／SRC83→POST_N0／SRC81。PRE是在重設定前取得，不能視為本MCU epoch新OVR；ACK及OFF_REQ不替代OLED／RGB現場目視。
- 新boot首STATUS為HX OK／RX1／RAW−163483／AGE131，後續至RX126維持HX／ADXL OK、ERR0／AERR0／FIFO1。WiFi從WAIT／CONNECTING到CONNECTED／radio1／IP192.168.52.71；不證外網或射頻時序。
- HXF初態WAITING／captured0／next0／valid0／missed0，尚無實際DATA點，不填LOW／連續LOW或ADC ready。保持UNCAL，不沿用先前係數。

## 完整bundle、守衛與原始完整性

主代理交付本輪新增私有live raw／chunk落盤防程序中斷遺失；Ampere沒有讀私有目錄，也未以機制存在預判資料完整。以下核對的是掃敏後公開原文、host接收／保存收支及逐行資料。

[serial](../../calibration/20261028-hx711-hxdual-continuous-observe-124442.serial.log)、[逐行JSONL](../../calibration/20261028-hx711-hxdual-continuous-observe-124442.lines.jsonl)、[host](../../calibration/20261028-hx711-hxdual-continuous-observe-124442.host.json)、[control](../../calibration/20261028-hx711-hxdual-continuous-observe-124442.control.log)、[完整分析v2](../../calibration/20261028-hx711-hxdual-continuous-observe-124442.analysis-v2.json)。

capture為12:37:52.914～12:44:42.297，實際409.383秒；planned620000ms為上限，不當本次擷取時長。191685 bytes與host received／stored一致，1363完整行＝403 STATUS＋403 WiFi＋512 HXRD＋12 HXFP＋33其他行，無尾片。SHA256 `A97AC6F84344A30A0680597B49C6E7352BFD9BCBB841F8AF12E74F9D7958488F`與host一致；Ampere獨立核對原始UTF-8完整行、offset、行號、全部已解析欄位及UTC／單調順序，差異0，public_redacted=false。

只有HX_DIAG STATUS（12:37:56.418）及HX_DIAG DUMP（12:44:32.280）兩條唯讀TX。先於12:44:32.047收到schema2／slots2／all_frozen1，再送DUMP；最外BEGIN_HX_CAPTURE為12:44:32.326、完整END_HX_CAPTURE為12:44:37.218，接收跨度4.892秒。兩槽及唯一HXF依序、最外END complete1／retained1／read_only1完整，未提前截於ANY或HXF。

host expected／observed_session均`A1C20320`、run_conflict=null；原文所有session一致、無額外boot／READY，所有STATUS為IDLE／UNCAL，沒有觀察到所列狀態衝突。正常本窗追溯不代表已注入故障測過每個拒絕分支。公開核心FW及v2工具來源hash本輪重新核對與已部署／既有v2一致；沒有新FW／燒錄。

## 雙槽分離事件與獨立CRC

| 核對 | ANY | LINK |
|---|---|---|
| slot政策／窗口 | mask7，start_us516311／window600000000 | mask1，相同起點／窗口 |
| 首觸發 | seq3277，JUMP4，end358539689µs | seq3804，LINK1，end417250681µs |
| 保留seq／數量 | 3150～3405／256筆 | 3677～3932／256筆 |
| 前後史／凍結 | 127前＋trigger＋128後，reason1／post_complete1，freeze372619617µs | 127前＋trigger＋128後，reason1／post_complete1，freeze431312680µs |
| 固定封存收支 | attempts3405=pre_overwritten3149+256+omitted0 | attempts3932=pre_overwritten3676+256+omitted0 |
| 接受／拒絕 | 256接受、0拒絕 | 255接受、1拒絕 |
| 保留／後史跨度 | 28.067192／14.079928秒 | 28.367876／14.061999秒 |
| CPU括弧／開始gap | read186～231µs；255個可跨列gap14952～249996µs | read187～232µs；255個gap15629～274035µs |

兩槽均FROZEN／export_sealed1／overflow0／seq_wrapped0。封存快照omitted均0，沒有把越窗未分類筆數當本次觸發判斷；ANY後續的live未收錄不回寫已封存計數。LINK失敗開始417250483µs，比ANY已凍結372619617µs晚44.630866秒；**ANY先凍結並未關閉LINK／HXF**，沒有為等待LINK提前DUMP。兩槽都在共同600秒總窗內完成。

Ampere以獨立CRC實作逐列重算512列19-word CRC、24-bit補數解碼、RX／ERR增量、seq／gap連續性及可重算flags，差異0；首列比較基準在窗外，不補造其前值。兩槽無重疊seq，512輸出列是512個不同attempt；本次沒有重疊列可比較，不能把分析器overlap_canonical_equal=true當本輪已實測重疊一致。seq3406～3676的271個中間attempt不在這兩個保留窗口，不稱全程連續原始字或ADC漏樣。

| 事件／鄰框 | bits24／raw | 原接受、計數及限制 |
|---|---|---|
| ANY3276 | `0xFD8199`／−163431 | accepted1、ERR0 |
| **ANY3277** | **`0xFFFEC0`／−320** | **accepted1／後檢1，RX3276→3277，ERR0不變，JUMP4** |
| ANY3278 | `0xFD81D8`／−163368 | accepted1、ERR0，回到近鄰值、JUMP4 |
| LINK3803 | `0xFD818C`／−163444 | accepted1、RX3802→3803，ERR0 |
| **LINK3804** | **`0xFFFF60`／−160** | **accepted0／後檢0，RX3803保持，ERR0→1，LINK1** |
| LINK3805 | `0xFD81DD`／−163363 | accepted1，RX3803→3804，ERR1不變，flags0 |

ANY−320是被接受的異常近零字，不因ERR0或CRC有效成為可信量測。其前後值算術右移9位相容只列數值候選，不證物理丟位或可修RAW，也不證與舊−80／−156同因。此事件早於後續LINK／LOW介入，不能回因至該次hold。

主機STATUS沒有印到−320；ERR變化相鄰收行為JSONL774／12:44:17.222 RX3797 ERR0，到JSONL776／12:44:18.225 RX3803 ERR1／HX LINK_ERR／RAW−163444。這是主機觀察界線，不是精確故障UTC；拒絕−160沒有覆寫最後有效RAW。

## 真DATA：9個可信離散點、3個missed

HXF schema1／SCK_LOW_250MS為COMPLETE（mode2、stop_reason1、captured1、next12、valid9／missed3）；原phase_before0／cal_before0表示本次在IDLE且無校正收集才hold。metadata與LINK3804的seq、bits、raw、原start／end、RX3803不動及ERR0→1逐欄精確對應。

Ampere獨立重建19個metadata＋12點各7個word共103個canonical LE uint32，CRC為`0x6DFD4AEA`、與原文一致；invalid點內部canonical零只供CRC，匯出仍是NA，不轉成LOW。全部點位及實際括弧相對原read_end anchor `417250681µs`如下：

| index／目標µs | 實際begin～end（anchor後µs） | DATA | valid／missed |
|---|---|---|---|
| 0／2 | NA | NA | 0／1 |
| 1／10 | NA | NA | 0／1 |
| 2／50 | 51～56 | LOW | 1／0 |
| 3／200 | 201～202 | LOW | 1／0 |
| 4／1000 | 1001～1003 | LOW | 1／0 |
| 5／2000 | 2001～2002 | LOW | 1／0 |
| 6／5000 | 17735～17748 | LOW | 1／0 |
| 7／20000 | 22731～22744 | LOW | 1／0 |
| 8／50000 | 50731～50744 | LOW | 1／0 |
| 9／100000 | 102733～102746 | LOW | 1／0 |
| 10／150000 | NA | NA | 0／1 |
| 11／250000 | 273780～273810 | LOW | 1／0 |

有效點DATA均0，前後CPU括弧1～30µs；非末點begin均未跨下一目標，跨過的2／10／150000µs點如實missed，不補採。最後點允許晚到，begin比250ms目標晚23.780ms、end晚23.810ms。COMPLETE表示目標索引處理至末點，**不表示12點皆取得或全程連續LOW**；中間長間隔及3個missed仍未知，不能定位何種排程／電氣原因造成晚到。

原3804 read括弧為417250483～417250681µs（198µs）；probe_start417250706，比anchor晚25µs，probe_end417524491。從原anchor到hold結束273810µs，probe_start到end為273785µs。下一真正attempt3805 start417524518，比probe_end晚27µs、比anchor晚273837µs；與LINK行的gap274035µs及metadata resume完全一致。下一read括弧210µs，接受−163363；後128框均accepted、ERR1，RAW−163494～−163304。

公開來源顯示此profile不加SCK脈衝、只新增離散DATA讀取並延後下一HX attempt；軟體發25脈衝不證器件收到25個有效邊緣。實際LOW等待改變後續讀取時序，不能把下一字回近鄰值當修復、恢復正常或證明無介入時會如何。這是**可信的部分有效離散DATA捕獲（9／12）**，不是獨立電氣波形、連續LOW、ADC ready或根因已確診。

v2分析器從原始serial重跑exit0，全部JSON物件與保存analysis-v2完全一致；FW與工具未修改。CRC／bits／raw同源一致只證形成後的匯出完整性，不能證明原取得字正確或還原正確RAW。

## 匯出排程及後續被動窗

本capture有403 STATUS／403不同RX307→3971，無額外boot、phase／CAL意外；ERR由0變1、AERR0。DUMP的雙256列使最外層接收跨度4.892秒；相鄰STATUS最大主機間隔5071.6367ms跨此同步匯出，不當ADC週期或非預期韌體reset證據。

最外END後JSONL1354於12:44:37.359為HX STALE／AGE5047ms、ADXL MISSING／FIFO1；JSONL1356於12:44:38.361恢復HX／ADXL OK，FIFO變2，其後持平。此為已知同步DUMP不輪詢的觀測條件，發生在資料封存後，不能回因至ANY−320、LINK拒絕或DATA各點。本輪沒有新ADXL_DIAG定位第二警示來源，不推實際漏樣量或與HX同因。

另[90秒serial](../../calibration/20261028-hx711-postdump-passive-124855.serial.log)、[JSONL](../../calibration/20261028-hx711-postdump-passive-124855.lines.jsonl)、[host](../../calibration/20261028-hx711-postdump-passive-124855.host.json)、[control](../../calibration/20261028-hx711-postdump-passive-124855.control.log)為零TX獨立窗：capture12:47:24.445～12:48:55.262（90.817秒），STATUS12:47:24.995～12:48:54.319。29700 bytes／180完整行／無尾，SHA256 `FFB45F8875090EA5919FACC95E6D810CEE9C362DDD5F9476E199695468A7C0FC`與host一致；原文／offset／全部欄位／時間差異0。

90個不同RX5461→6276，全窗IDLE／UNCAL／HX及ADXL OK、ERR1／AERR0／FIFO2不增；RAW均值−163418.6111、範圍−163533～−163291 counts，AGE125～215ms。此窗沒有新版本／session查詢，按本輪操作追溯；與前窗有未擷取間隔、ring已凍結而無新增逐框。約1Hz快照不抹除已捕獲事件，不證量測精度或修復。

bundle及被動窗OPEN前115200／8N1、DTR／RTS off與實際control DESCRIBE相符，CLOSE／QUIT OK、process exit0、stderr0、overflow=false／failure=null，主代理已釋放COM3。monitor ACK不是ESP32 ACK；沒有STOP／CLEAR／TARE／CAL或新flash。

## 結論、缺口及後續責任

| ID／狀態 | 條件 |
|---|---|
| HXF2-C02-Q01／P2／限定路徑通過 | ANY先JUMP封存、LINK後拒絕捕獲、雙槽完整匯出／CRC及各127前＋128後通過；兩窗無重疊，無本輪重疊列可比。其他busy／truncated／overflow／回繞／重複DUMP等未測。 |
| HXF2-C02-Q02／P2／可信部分DATA、覆蓋缺口保留 | 真LINK與HXF綁定／103-word CRC通過，9有效LOW／3missed、late及resume如實核對；完整12點與連續電氣波形未取得。Ampere交Kierkegaard／Gibbs作物理與軟體候選分析，不藉追加reset追到看似正常。 |
| HXF2-C02-Q03／P2／警示保留 | 初始化FIFO1；DUMP後STALE／MISSING、FIFO1→2，與DATA介入／HX原因分層，不清警示或推已知漏樣量。 |
| HXF2-C02-Q04／P1／量測資料品質未通過 | 同輪ANY仍接受−320、ERR0，另真LINK拒絕；根因未定位，UNCAL。Gibbs／Kierkegaard／主代理須先釐清及驗證必要修復，再建近時TARE／CAL366與回零／重放／另一點；不能因捕獲DATA或CRC通過沿舊比例、修RAW或直接校正。 |

本輪捕獲目標已達，主代理已停止後續循環reset、保留session與凍結資料。boot、bundle及後90秒三段capture均已關；Ampere未持有埠，只更新本獨立觀察報告，未改工程檔／原始資料，未commit／push。可信捕獲不是根因結案，366g校正持續暫停；本報告交Gibbs、Kierkegaard、Planck與主代理追蹤。
