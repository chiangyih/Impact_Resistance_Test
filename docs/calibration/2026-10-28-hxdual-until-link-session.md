# 2026-10-28 HX711雙槽有界重啟續錄session

日期／時區：2026-10-28，Asia/Taipei（UTC+08:00）。主代理獨占COM3操作；Planck只核對公開資料並整理此文件。

**第02輪`A1C20320`已可信捕獲HX拒絕後9個有效LOW離散點，另3個目標明確missed；本輪診斷捕獲目標已達，主代理已停止循環重啟並釋放COM3。** ANY先接受異常字−320，之後LINK拒絕−160並ERR0→1；兩槽各256列、512個不同attempt及完整CRC匯出通過。實際hold273.810ms改變後續時序，9點不證連續LOW或根因；量測資料品質仍未通過、UNCAL及366g校正暫停。保持USB供電，裝置仍為既有hxfail2，未新flash、commit／push；PCNT方案測試中，尚未燒錄或取得實板邊緣結果。

## 最新授權與每輪邊界

本輪新指示優先於[前一重啟session](2026-10-28-hxdual-restart-session.md)中「兩次有界觀察完成後不再自行重啟」的歷史界線；不改寫前兩輪已發生的操作或資料。既有session `99C7AFD6`的[完整bundle](20261028-hx711-hxdual-restart-observe-120811.serial.log)及[分析](20261028-hx711-hxdual-restart-observe-120811.analysis-v2.json)已保存，包含末筆越窗未分類的限制，不以新輪覆蓋。

以下為本次續錄的執行規則，尚未發生的後續輪次不列完成：

- 每輪主機擷取有界，裝置雙槽沿用600秒目標；完整保存boot、版本／session、原文／JSONL／host、封存計數及bundle後才續下一輪。
- 同一時間只有主代理操作COM；每輪先核對IDLE／UNCAL及RX前進守衛，再依授權作一次RTS assert／release，DTR false、USB不拔，不清警示或改RAW。
- 每次boot另立epoch，不合併為不重置的長時間穩態，不跨boot合計RX／ERR或沿用RAM校正。保持USB供電不代表RAM跨重啟保留。
- 遇不一致或真LINK，先保留原始資料與失敗後DATA並分析，不立即重啟抹去現況。核對原拒絕綁定、實際採點／valid／missed／resume及完整CRC；不把NA當LOW、無事件或後檢HIGH當資料正確。
- 取得DATA是診斷進展，並非根因／修復或量測驗收。待原因與資料品質前提處理後才近時TARE／CAL及366g載卸驗證；目前UNCAL、尚未校正。

每輪session識別需連同boot原文及主機檔案追溯；8位識別不作永久唯一ID。主機收行UTC、CPU括弧與GPIO／ADC時刻分開，DUMP同步阻塞條件另外記錄。

## 12:23前檢：上一epoch已封存

- [前檢serial](20261028-hx711-before-continuous-restart-122346.serial.log)
- [完整行JSONL](20261028-hx711-before-continuous-restart-122346.lines.jsonl)
- [host設定、唯讀命令及收尾](20261028-hx711-before-continuous-restart-122346.host.json)

serial-monitor v0.15.0，OPEN前115200／8N1、DTR／RTS off；12:23:35.616 OPEN ACK後只送12:23:39.127 STATUS、12:23:40.229 HX_DIAG STATUS兩條唯讀命令，capture於12:23:46.341結束。12筆STATUS收行12:23:35.965～12:23:46.046，RX13958→14049，均IDLE／UNCAL／HX及ADXL OK、ERR0／AERR0／FIFO2。

兩槽讀回hxfail2／session `99C7AFD6`、FROZEN／reason3、records127／export_sealed1；HXF為CLOSED_NO_LINK／captured0。這是既有封存狀態，不是新取得的失敗後DATA。本窗CLOSE／QUIT成功、process exit0、failure=null，該連線已關閉。

## 續錄第01輪：7E5D072F自然boot已取得

- [第01輪boot serial](20261028-hxdual-continuous-01-boot.serial.log)
- [逐行接收JSONL](20261028-hxdual-continuous-01-boot.lines.jsonl)
- [SerialPort、守衛及RTS事件host](20261028-hxdual-continuous-01-boot.host.json)

同一已部署hxfail2，沒有新flash。主代理以.NET SerialPort、115200／8N1、handshake none開COM3，開埠前DTR／RTS false；USB未拔。守衛實際保存4筆舊epoch STATUS，IDLE／UNCAL且RX14564→14590，12:24:45.5226039 GUARD_PASS後才重啟。

| 已發生事件 | 主機時間（台北）及限制 |
|---|---|
| SerialPort OPEN | 12:24:41.9746800 |
| 單次RTS assert／DTR false | 12:24:45.5239822 |
| RTS release／DTR false | 12:24:45.7387854；目標200ms，主機單調呼叫間隔214.8165ms，不是獨立量到的管腳電壓脈衝 |
| 自然ROM原文 | 12:24:45.7704729收到`rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)`；不能由此判USB曾斷電 |
| 自然應用banner | 12:24:46.6704360讀回`v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1-hxfail2`、UNCAL |
| HX_DIAG_READY | 12:24:46.8260943，schema2／session `7E5D072F`／slots2／window_us600000000 |
| 初始HXF | 12:24:46.8566314，WAITING／captured0／next0／valid0／missed0；尚無失敗後DATA |
| boot擷取結束／SerialPort CLOSE | 12:25:00.7461046／12:25:00.7535125，port_closed=true／failure=null |

boot檔包含重啟前4筆與新boot後15筆STATUS，不能當成一條RX連續序列。新boot短窗為IDLE／UNCAL／HX及ADXL OK、ERR0／AERR0／FIFO1，僅描述已保存狀態，不表示目前觀察窗或全部讀框皆正確。自然banner不代替OLED／RGB現場目視，未使用先前OLED數值。

## 原始完整性與待補

Planck逐列核對原始serial與JSONL的完整行、行號及UTF-8 byte offset，兩份差異均0；前檢5739 bytes／33完整行，boot7722 bytes／59完整行，均無未完成尾片，serial SHA與host保存值一致。boot JSONL沒有預解析fields，判讀依原文，不宣稱核對不存在的欄位。

| 公開檔 | SHA256 |
|---|---|
| before-continuous-restart serial.log | `2F220EDC643D42F35A56330DF0CCC334D33494FEF69693C308ACBCC71F6B4A25` |
| before-continuous-restart lines.jsonl | `E0258D971D0F54E2186748A9886B4BBB731F9495781B83A6871FACC5721D2E24` |
| before-continuous-restart host.json | `ACC7609317ED587A721E2404F8CD2A58536DD2EB42641BCA61B6E3A4F2385F93` |
| continuous-01-boot serial.log | `84306A4AE5D2BB8C519E686662F9087F973A724558BA27566F9C45DB5F4C48BC` |
| continuous-01-boot lines.jsonl | `B7E2E3A4FE981FF82F000E55829645EAA697C8A06F894A52B1E25E41597925C3` |
| continuous-01-boot host.json | `66B0FDF76426B3090E3A63236CB6090D27D885412428CBB424F588E86DBCFCD0` |

前檢及boot擷取各已關閉；其後第01輪bundle觀察結果已取得，另列於下節。後續新增輪次僅在實際保存證據後追加，不預造次數或時間。保持供電／UNCAL、未TARE／CAL、未新flash或commit／push；Planck僅整理此文件，未開COM、讀秘密或改handoff／其他人的文件。

## 續錄第01輪完整bundle：WINDOW、未取得失敗後DATA

- [bundle serial原文](20261028-hx711-hxdual-continuous-observe-123455.serial.log)
- [完整行JSONL](20261028-hx711-hxdual-continuous-observe-123455.lines.jsonl)
- [host設定、session守衛與收尾](20261028-hx711-hxdual-continuous-observe-123455.host.json)
- [完整analysis-v2](20261028-hx711-hxdual-continuous-observe-123455.analysis-v2.json)

本窗OPEN ACK為12:25:55.055，capture結束12:34:55.397；只送12:25:58.556 HX_DIAG STATUS、12:34:47.379 HX_DIAG DUMP兩條唯讀命令。host的expected_session與observed_session均為`7E5D072F`、failure=null，與本輪boot及完整bundle一致。12:34:46.702已先收到兩槽all_frozen1／reason3通知，DUMP未強制凍結；外層BEGIN／END_HX_CAPTURE收行12:34:47.423～12:34:50.008，完整跨度2.585秒。

| 項目 | 核對結果及界線 |
|---|---|
| 共同期限 | start_us516288、目標600000000µs、freeze_us600544419，實際600.028131秒，比目標晚28.131ms；為CPU服務期限，不是ADC或GPIO波形。 |
| ANY／LINK | 同hxfail2／`7E5D072F`，均WINDOW／reason3、trigger_flags0；各127列seq5459～5585，accepted127／rejected0、overflow0。 |
| 精確封存收支 | **5586＝5458 overwritten＋127 retained＋1 omitted**；第5586次越窗，不能稱5586次全部已受觸發判斷。 |
| 重疊 | 兩槽254個輸出列，canonical逐word完全重疊，只有127個不同attempt，不重計為254次量測。 |
| 保留跨度 | 首start586909477～末end600463691 boot µs；只是末段前史，未保留整個600秒全部原字。 |
| HXF | CLOSED_NO_LINK／captured0、12點valid0／missed0、時間及dout NA，metadata／resume NA，103-word CRC `0x7E8831FE`通過；不是12個LOW或逾點。 |
| 實際介入 | hold／changed_next_attempt_timing均false，未取得真LINK或實際失敗後DATA。 |

沿用已部署契約，完成seq先記入attempts，`Tick(record.start_us)`若使槽到期則omitted++並在flags分類之前返回。因此**僅seq1～5585可支持未觸發既定LINK／SAT／相鄰accepted差≥1000規則**；第5586次原字／flags未保留，不補造為正常。JUMP首個accepted沒有前值比較；上述涵蓋不等於5585個字經離線驗真，也不是全部ADC conversion，5458筆已覆寫的原字不可還原。

Planck從原serial重跑v2.0.0解析器exit0，全部JSON與保存analysis-v2一致，兩槽CRC、重疊及完整bundle契約通過。這只支持形成後的匯出完整性，不能以本輪無觸發解除舊−80／−317／極端字、漂移或量測限制；ANY先封存後LINK的事件分離與實際DATA採點仍未測。

## 第01輪快照、DUMP干擾與完整性

537 STATUS／536不同RX646→5638，收行12:25:55.374～12:34:55.164；RX5592有一次同RAW−163394的重印，不算新ADC讀值。全STATUS均值−163346.9590、範圍−163485～−163236 counts，均值含該次重印；約1Hz快照不是全部已接受字、ADC時脈或靜態精度證據。

全窗IDLE／UNCAL、ERR0／AERR0。同步DUMP後12:34:50.147印HX STALE／AGE2766ms、ADXL MISSING／FIFO1，12:34:51.149近期OK並FIFO升2，之後至末筆持平。匯出會阻塞輪詢，這是封存後的介入條件；未另取ADXL_DIAG定位警示來源，不推實際漏樣量或與HX同因，不回因至封存前資料。

serial208838 bytes、1373完整行、無尾片；Planck核對原文／行號／UTF-8 byte offset／STATUS已解析欄位差異均0，serial SHA與host一致。缺值仍為NA、不補0，跨boot不拼接為一條RX序列；主機完整行UTC不是原讀框／ADC時刻。

| 第01輪bundle公開檔 | SHA256 |
|---|---|
| serial.log | `A6BA166C6365C3294325A59BCBCE408E17AD144DB16D50FB02C61BB6504F215B` |
| lines.jsonl | `2B7D4A76BFCC97899B5AA21AB1C6714BD857B5341C8B436D705754763EC0A5F2` |
| host.json | `E92929A334283B524B7D27600AEA96C12610D01AEB263A9483A3FA4BB96CD5FD` |
| analysis-v2.json | `055A01A3ACD93D51088CA55C61EBA1AC8D974889DED4E12EF7ACDD084DC3D233` |

第01輪monitor CLOSE／QUIT成功、process exit0、overflow=false、failure=null，主機確認該連線COM3已釋放。[Ampere第01輪完整QC](../quality/reports/2026-10-28-hxfail2-continuous01-qc.md)已完成：reset／自然boot身分、到期封存及完整匯出有限子項通過，實際DATA未觸發、警示保留、量測用途未驗收；符合依原授權在新鮮guard後續一輪的控制前提，並非校正可開始。其後實際第02輪boot如下，不將未取得DATA表述為永久停止或診斷完成。

## 續錄第02輪：A1C20320自然boot

- [第02輪boot serial](20261028-hxdual-continuous-02-boot.serial.log)
- [完整行接收JSONL](20261028-hxdual-continuous-02-boot.lines.jsonl)
- [SerialPort設定、guard與RTS事件host](20261028-hxdual-continuous-02-boot.host.json)

同一已部署hxfail2，USB未拔、沒有新flash。開COM3前115200／8N1、handshake none、DTR／RTS false；舊第01輪證據與QC已保存。守衛實際取得4筆IDLE／UNCAL且RX6930→6959前進的舊epoch狀態，之後才作一次RTS assert／release，沒有將舊新RX串成連續資料。

| 已發生事件 | 主機時間（台北）與界線 |
|---|---|
| SerialPort OPEN | 12:37:16.5660971 |
| PRE_RESET_GUARD_PASS | 12:37:20.0931559，status_count4、last_rx6959 |
| 單次RTS assert／DTR false | 12:37:20.0939968 |
| RTS release／DTR false | 12:37:20.3036600；目標200ms，主機單調呼叫間隔209.6824ms，不是獨立電壓脈衝量測 |
| 自然ROM | 12:37:20.3489783印`rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)`；不據此判USB斷電 |
| 自然應用banner | 12:37:21.2161736讀回`v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1-hxfail2`、UNCAL |
| HX_DIAG_READY／初始HXF | 12:37:21.3999428讀回schema2、session `A1C20320`、slots2、window600000000µs；12:37:21.4301159 HXF WAITING／captured0／valid0／missed0 |
| boot擷取結束／CLOSE | 12:37:35.3320348／12:37:35.3345118，port_closed=true、failure=null |

boot檔19筆STATUS包含舊epoch4筆及新boot15筆。新boot RX1→126、IDLE／UNCAL／HX及ADXL OK、ERR0／AERR0／FIFO1，僅是本段已保存短窗，不證目前全部讀字正確、DATA路徑已通過或原異常已修。`A1C20320`與`7E5D072F`的RAM／計數分開，不跨boot合計無觸發次數。

serial7718 bytes／59完整行、無尾片；Planck逐列核對原文／行號／UTF-8 byte offset與JSONL差異0，serial SHA與host一致。此JSONL沒有預解析fields，判讀依原文；主機UTC不當成ADC／GPIO時刻。

| 第02輪boot公開檔 | SHA256 |
|---|---|
| serial.log | `3DC210A82E0F5BFB2864501521EF43CCD4688504C8B4737D13A3EA30853A62B8` |
| lines.jsonl | `17DD97F2E2F15BAFC0A9FEDC11E23FAEB2C41AACFE703F93F2E3F1C59127F659` |
| host.json | `8FF4FDFE8E115D5140DFB0AFFF13721EB9432FE878F246AC9AF10826F866297D` |

第02輪boot擷取已關閉；主代理其後另開有界觀察，已取得下節完整bundle及失敗後DATA。boot時WAITING與其後捕獲結果分開記錄，不回填為boot當時已完成。

## 第02輪雙槽事件：ANY先接受−320，之後LINK拒絕−160

- [第02輪bundle serial](20261028-hx711-hxdual-continuous-observe-124442.serial.log)
- [完整行JSONL及收行UTC](20261028-hx711-hxdual-continuous-observe-124442.lines.jsonl)
- [host命令、session guard及收尾](20261028-hx711-hxdual-continuous-observe-124442.host.json)
- [完整analysis-v2](20261028-hx711-hxdual-continuous-observe-124442.analysis-v2.json)
- [Ampere第02輪完整QC](../quality/reports/2026-10-28-hxfail2-continuous02-qc.md)

以下時間皆為2026-10-28台北24小時制（UTC+08:00）。本窗OPEN ACK為12:37:52.914，capture結束12:44:42.297；host expected_session與observed_session均為`A1C20320`、failure=null。只送12:37:56.418 HX_DIAG STATUS及12:44:32.280 HX_DIAG DUMP；12:44:32.047已先收到兩槽all_frozen1／reason1通知，DUMP沒有強制凍結。完整外層BEGIN／END_HX_CAPTURE收行12:44:32.326～12:44:37.218，跨度4.892秒。

| 槽／保留範圍 | 事件及完整性 |
|---|---|
| ANY mask7，seq3150～3405 | 256列＝127前＋trigger3277＋128後，accepted256／rejected0，JUMP flags4，post_complete1／overflow0。封存attempts3405＝pre_overwritten3149＋256 retained。 |
| LINK mask1，seq3677～3932 | 256列＝127前＋trigger3804＋128後，accepted255／rejected1，LINK flags1，post_complete1／overflow0。封存attempts3932＝pre_overwritten3676＋256 retained。 |
| 共用epoch與重疊 | 同一hxfail2／`A1C20320`，共同window_start516311 boot µs；兩槽沒有重疊，512列為512個不同attempt，不是兩段獨立ADC真值量測。封存計數各自固定，不相加成總轉換數。 |

| seq／槽 | bits24／RAW | 原接受與計數 |
|---|---|---|
| 3276／ANY前鄰 | `0xFD8199`／−163431 | accepted1、RX3275→3276、ERR0 |
| **3277／ANY觸發** | **`0xFFFEC0`／−320** | **accepted1、postcheck HIGH，RX3276→3277、ERR0，JUMP4** |
| 3278／ANY後鄰 | `0xFD81D8`／−163368 | accepted1、RX3277→3278、ERR0，JUMP4 |
| 3803／LINK前鄰 | `0xFD818C`／−163444 | accepted1、RX3802→3803、ERR0 |
| **3804／LINK觸發** | **`0xFFFF60`／−160** | **accepted0、postcheck LOW，RX3803→3803、ERR0→1，LINK1** |
| 3805／resume | `0xFD81DD`／−163363 | accepted1、RX3803→3804、ERR1，已受hold介入 |

ANY觸發read_end358539689µs至LINK read_end417250681µs相隔58.710992秒，為同boot CPU時間差，不貼成主機精確UTC。ANY兩側數值算術右移9位均得−320，只是事後數值相容候選，不證丟9位、不修RAW，也不確定與舊−80／−317同因。ANY先凍結後LINK仍捕獲的分離事件路徑已有本版實板證據；接受−320／ERR0同時明確顯示量測品質問題仍在。

## LINK後DATA：9個有效LOW、3個missed及實際時序

HXF為COMPLETE／captured1並綁定seq3804、`0xFFFF60`／−160、RX3803、ERR0→1，保留原拒絕、phase_before0／cal_before0。anchor是原read_end **417250681 boot µs**；probe_start417250706、probe_end417524491，anchor至probe_end為**273810µs（273.810ms）**。resume有效，下一次attempt為seq3805／start417524518，比probe_end晚27µs。

下表的實際括弧均為相對anchor的CPU時間（µs），不是電壓波形或ADC時刻。missed點明確NA，不補成LOW；最後250000µs只是目標，實際點晚到。

| 目標µs | 實際begin～endµs | 結果 |
|---|---|---|
| 2 | NA | missed |
| 10 | NA | missed |
| 50 | 51～56 | valid、DATA LOW |
| 200 | 201～202 | valid、DATA LOW |
| 1000 | 1001～1003 | valid、DATA LOW |
| 2000 | 2001～2002 | valid、DATA LOW |
| 5000 | 17735～17748 | valid、DATA LOW，非精確5ms |
| 20000 | 22731～22744 | valid、DATA LOW |
| 50000 | 50731～50744 | valid、DATA LOW |
| 100000 | 102733～102746 | valid、DATA LOW |
| 150000 | NA | missed |
| 250000 | 273780～273810 | valid、DATA LOW，晚於目標 |

9個有效讀取皆LOW可信，另3個目標missed，沒有未處理點；**COMPLETE不代表完整12點覆蓋**。有效點之間電位未知，不證連續LOW、ADC ready、SCK有效邊緣、實際少幾個clock或根因。軟體profile延後下一HX attempt，`low_hold_intervention_applied=true`、`changed_next_attempt_timing=true`；不能把resume回近鄰RAW當成已修復或證明無介入時會如何。

兩槽逐列CRC、12點及103-word profile CRC `0x6DFD4AEA`、LINK／profile綁定與完整外層END通過。Planck從原serial重跑v2.0.0 exit0，全部JSON與保存analysis-v2全物件一致；另核對512個HXRD列／seq均不重複。CRC支持形成後匯出完整性，沒有獨立ADC真值；主機12:44匯出收行UTC不是原事件或各GPIO讀取的精確UTC。

## 第02輪快照、DUMP及後90秒窗

bundle窗403 STATUS／403不同RX307→3971，收行12:37:53.797～12:44:41.372；STATUS RAW平均−163389.1092、範圍−163507～−163248 counts，約1Hz沒有印到已接受−320，不能用它驗真全部原字。全窗IDLE／UNCAL、AERR0，ERR0→1並曾印LINK_ERR，與逐讀拒絕分層記錄。

雙槽512列同步DUMP後12:44:37.359印HX STALE／AGE5047ms、ADXL MISSING／FIFO1；12:44:38.361近期OK並FIFO升2，其後持平。這是資料凍結後的同步輸出介入，不回因至ANY異常、LINK拒絕或DATA採點；沒有新ADXL_DIAG定位第二警示來源，不推實際漏樣量或與HX同因。

- [匯出後90秒serial](20261028-hx711-postdump-passive-124855.serial.log)
- [完整行JSONL](20261028-hx711-postdump-passive-124855.lines.jsonl)
- [host與關埠](20261028-hx711-postdump-passive-124855.host.json)

後窗零裝置TX，capture12:47:24.445～12:48:55.262、90.817秒；90 STATUS收行12:47:24.995～12:48:54.319，90不同RX5461→6276。IDLE／UNCAL／HX及ADXL OK、ERR1／AERR0／FIFO2持平；RAW平均−163418.6111、範圍−163533～−163291 counts。與前窗有未擷取間隔、未再查版本／session且ring已凍結，依操作追溯承接本boot；約1Hz快照不抹除異常或證明精度／修復。

## 原始完整性、QC與停止循環後狀態

bundle serial191685 bytes／1363完整行，後90秒29700 bytes／180完整行，兩者均無尾片。Planck逐列核對原文／行號／UTF-8 byte offset／STATUS已解析欄位差異0，各serial SHA與host一致；第02輪boot serial hash亦與先前保存一致，沒有改寫原始證據。缺值保留NA，所有主機時間皆按UTC+08轉為24小時制，沒有補造ADC時刻。

| 第02輪bundle／後窗 | SHA256 |
|---|---|
| bundle serial.log | `A97AC6F84344A30A0680597B49C6E7352BFD9BCBB841F8AF12E74F9D7958488F` |
| bundle lines.jsonl | `A138D53042E1A6D8D3730AC6D8100C1E7DA7A4935654D7D29455F5E2136BB957` |
| bundle host.json | `F6C9A5645F8BFD3E2E4E6508EC3F1D92E8429F4A0DBFDF86B0AB261DDDFEA15D` |
| bundle analysis-v2.json | `DCABE9AD79892C4A4CE54B20D4D364B3C49188093B832613046163A0AD62B1C0` |
| 後90秒serial.log | `FFB45F8875090EA5919FACC95E6D810CEE9C362DDD5F9476E199695468A7C0FC` |
| 後90秒lines.jsonl | `CC978D64E8FB82DEFB8C88436A87F6498B3BA9CAEB42A39D8B04A5D1AAD6ADE3` |
| 後90秒host.json | `DE25B41B32BE36C7BD06CEB97A88DD2BA270CEA2491A421A5BC5DE53ACE01145` |

[Ampere第02輪QC](../quality/reports/2026-10-28-hxfail2-continuous02-qc.md)判真LINK後9個離散DATA捕獲可信、三missed如實保留，雙槽分離事件／CRC及匯出有限路徑通過；Q04量測資料品質未通過，根因未定、CAL仍暫停。不是完整波形或材料驗收。

捕獲目標已達，主代理已停止後續循環RTS重啟並保留`A1C20320`。第02輪boot、bundle及後90秒擷取均已正常關閉；兩monitor窗CLOSE／QUIT成功、process exit0、overflow=false／failure=null，COM3已釋放，USB未拔／供電保持。尚在離線評估免外接儀器的邊緣觀察方案，**尚未部署或執行**，不填新波形／事件結果。維持UNCAL，原因與366g校正未解；未TARE／CAL、未修改韌體／新flash、commit／push。Planck只更新本文件，未開COM、讀秘密或改handoff／其他文件。

## 下一步設計中：不改線的ESP32 PCNT邊緣診斷

本輪真DATA與原始證據已保存，循環reset已停止。9個有效LOW點之間仍有盲區，其中相對anchor的102746～273780µs相隔171.034ms，期間電位未知，不能補成連續LOW。Gibbs與Kierkegaard正研擬不改線、使用ESP32 PCNT的邊緣診斷，作為釐清離散觀察缺口的候選方案。

**本設計初記時僅在研擬，當時尚未實作、編譯、燒錄或為此重啟，也沒有PCNT實測結果。** 後續測試進度另記於下節，不改寫當時的未執行狀態。PCNT方案即使日後取得資料，也僅觀察MCU端數位邊緣，不能當HX端類比波形、電氣幅度或供電量測；不能直接定位根因、還原RAW或推定校正通過。實際可觀察範圍及診斷介入限制仍待設計與驗證。

本設計初記時供電保持、UNCAL、COM3已關閉／釋放，當時沒有因本設計而TARE／CAL、改線或改韌體。366g校正仍待原因與資料品質前提釐清，不把候選方案描述為已執行或已驗證。

## PCNT測試期間：5分鐘零TX被動窗

- [serial原文](20261028-hx711-pcnt-design-passive-132156.serial.log)
- [完整行JSONL](20261028-hx711-pcnt-design-passive-132156.lines.jsonl)
- [host設定、收尾及計數](20261028-hx711-pcnt-design-passive-132156.host.json)
- [control原文](20261028-hx711-pcnt-design-passive-132156.control.log)

主代理後續回報PCNT仍在測試、**尚未燒錄**；本窗不是PCNT實板結果，未新增重啟或DATA／邊緣採點。保持既有供電及UNCAL，原事件與根因未解。

| 項目 | 已保存結果（台北24小時制） |
|---|---|
| PASSIVE_CAPTURE_START／END | 13:16:56.350～13:21:56.751，實際300.401秒 |
| STATUS收行 | 13:16:56.621～13:21:55.754，299 STATUS／299不同RX，21773→24579 |
| RAW平均／最小～最大 | −163555.9732／−163713～−163424 counts |
| phase／CAL／近期感測器狀態 | IDLE／UNCAL／HX及ADXL OK |
| ERR／AERR／FIFO_FULL | 1／0／2，全窗未觀察到增加 |
| 裝置TX／overflow／failure | 0／false／null |

serial最終保存98799 bytes、598完整行及**129-byte未完成尾片**；尾片原樣保留，不補造行、不納統計。Planck逐列核對原文／行號／UTF-8 byte offset／STATUS已解析欄位與JSONL差異0，serial SHA與host最終保存值一致。另PASSIVE_CAPTURE_END事件快照total_bytes=98798，比最終保存少1 byte；兩數分列，不將事件快照冒充最終檔長或自行補造該差值原因。

| 公開檔 | SHA256 |
|---|---|
| serial.log | `94F7D0BD970EB9EB3BDD66BA9899DE9BE0422FB45D1CBC1715B31B45A6C583D5` |
| lines.jsonl | `E23F066D750B2619D8EBFA0D5E775A7759BB87AF7F9D1716517EABDEA0788C36` |
| host.json | `4B5A52759DC8038BD398EDFE4EE4D5CAD12A21318A354B8B081A003231065438` |
| control.log | `20C89E60163C5035CC067814700B842114C0904728EDC0C73935360346B806A5` |

此窗約1Hz快照、非逐框，未再查版本／session；只按操作追溯既有狀態，不能解除已保存−320／LINK拒絕與量測品質問題，也不代表PCNT方案有效或366g校正可開始。CLOSE／QUIT成功、process exit0、主機確認COM3已釋放；供電保持、UNCAL，未TARE／CAL、新flash、commit／push。Planck此次僅更新本文件，未開COM或改其他檔。
