# 2026-10-28 PCNT1 同版續錄02獨立觀察QC

本輪是既有 `v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1-hxfail3-pcnt1` 的一次已授權有界重啟觀察，**沒有新編譯或燒錄**。前session EC44F494已封存保存，新session為 **DF497E3D**，分開處理RX／ERR／FIFO／RAM狀態；不以新boot覆蓋舊證據或恢復舊RAM校正。本版部署、來源雜湊與首輪功能驗證見 [PCNT1燒錄後QC](2026-10-28-pcnt1-qc.md)。Ampere只讀公開證據，主代理為唯一COM3操作者。

目前結論：**已可信擷取首LINK後PCNT硬體回報：有效包絡窗內SCK計數0／DATA計數4；四筆本boot正對照、GPIO復原、原始512讀框／HXF103word／PCNT405word CRC與完整匯出通過。** HXF另有8個有效離散LOW／4個missed。此為MCU側數位計數證據，不是HX端類比、供電或實際clock波形量測；根因仍未定位。ANY新近接受−641／ERR0，資料品質失敗確實重現，366g校正維持暫停。主代理已停止循環reset、保存供電／UNCAL，後續被動觀察與已關閉capture分列。

新增14:45–14:50同版被動窗已獨立核對：STATUS有效快取再現RAW−1282／HX OK／ERR3，兩鄰值>>7數值相容僅為候選；既有ring封存後沒有該新異常的逐框／PCNT資料。這是後續新增窗，不改寫前述事件或把快照當硬體邊緣根因證據，詳見新增附錄。

另已核對15:04–15:09零TX被動窗，ERR於相鄰快照中首次觀察3→4→5；即使該窗沒有大幅RAW跳值、HX全為OK，也不能判錯誤已消失。新增累計、同期RAW快取與主機收行界線另列第二附錄，不冒充錯框原字或新逐框PCNT。

## 舊session fresh precheck

證據：[serial](../../calibration/20261028-hx711-hxpcnt-next-precheck-135734.serial.log)、[JSONL](../../calibration/20261028-hx711-hxpcnt-next-precheck-135734.lines.jsonl)、[host](../../calibration/20261028-hx711-hxpcnt-next-precheck-135734.host.json)、[control](../../calibration/20261028-hx711-hxpcnt-next-precheck-135734.control.log)、[stderr](../../calibration/20261028-hx711-hxpcnt-next-precheck-135734.stderr.log)。下列UTC加08:00為2026-10-28台北時間。

- Capture UTC05:57:25.365至05:57:34.089，8.724秒。4958 bytes／28完整行／0尾，SHA256 **`E12FD450F13C2F81296CF88338869FE577755DFBBAB3F46ABEC6A475ABB6C437`**；host received／stored一致。Ampere從原始bytes重掃LF，原文／offset／line_no／所有fields／UTC及monotonic排序均0差異。
- 9個不同RX的STATUS，RX7265→7330；皆IDLE／UNCAL、HX／ADXL OK、ERR0／AERR0／FIFO2不增。這是重啟前短窗快照，不是全框或精度證據。
- 本版完整版本／session EC44F494，兩槽FROZEN／reason3／export_sealed1，PCNT setup1／clock_blocked0、proof4／good4／bad0／ready1、NO_LINK／sealed1，與先前封存相符。此次`scope=live attempts7301／omitted1803`是凍結後累計，7301=5371覆寫+127保留+1803封存後未分類；不能改寫先前`scope=frozen attempts5498／omitted0`或宣稱7301均無事件。
- 唯一兩個DEVICE_TX為UTC05:57:28.870 `STATUS`、05:57:29.975 `HX_DIAG STATUS`；沒有設備狀態變更命令。Control原文核對OPEN前115200／8N1／DTR off／RTS off、OPEN／CLOSE／QUIT均OK；CLOSE05:57:34.089、close及quit ACK05:57:34.093、process exit0於05:57:34.099，stderr0、overflow=false／failure=null、埠已釋放。監控工具ACK與ESP32回應分列。

## 一次重啟與自然boot

證據：[serial](../../calibration/20261028-hxpcnt-continuous-02-boot.serial.log)、[逐行JSONL](../../calibration/20261028-hxpcnt-continuous-02-boot.lines.jsonl)、[host/chunk與控制](../../calibration/20261028-hxpcnt-continuous-02-boot.host.json)。主代理使用.NET SerialPort，非serial-monitor；本段沒有額外ASCII設備TX，只有一次已授權RTS assert／release。

- 原文7376 bytes／57完整行／0尾，SHA256 **`B426223EB8B965F1D5D4256FCED65EAB592DE506902C6C54D0293653F668A10E`**，與host bytes／hash一致，raw_bytes_preserved=true、public_redacted=false。48個RX_CHUNK連續offset完整覆蓋7376 bytes；Ampere逐行比原文／offset／line_no、UTC／monotonic排序，以及每個LF末byte所屬chunk的UTC／monotonic映射，全部0差異。這是主機收行時刻，不是字／edge的發生時刻。
- UTC05:58:06.5040767已在OPEN前配置COM3／115200／8N1／Handshake None／DTR=false／RTS=false；OPEN05:58:06.5160920。主代理刻意保持USB接插及供電意圖是操作紀錄，沒有獨立供電波形量測。
- 重啟前3個STATUS為IDLE／UNCAL、HX／ADXL OK、RX7644→7653→7662、ERR0／AERR0／FIFO2。guard於UTC05:58:10.0560258通過，僅一次RTS assert05:58:10.0569082、release05:58:10.2725575，兩次DTR皆false；monotonic差 **215.6626ms**。這是主機API控制間隔，不是實測EN脈寬。
- 行9 UTC05:58:10.3034272為`rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)`；如實保留裝置訊息，不能由名稱推斷USB曾拔除或外部供電中斷。
- 行22 UTC05:58:10.7686689自然banner為完整PCNT1版本、UNCAL、SCK25／DATA33、RGB32 OFF requested；行26 UTC05:58:10.8321927為`HX_DIAG_READY schema=3 session=DF497E3D slots=2 pcnt_schema=1 window_us=600000000 record_bytes=56 rings_bytes=28672 pcnt_bytes=1620 pcnt_setup=1 clock_blocked=0`。這證明本新boot自然識別與前版報告相符，不能回填EC44F494未捕獲的啟動。
- 行27 HXF WAITING／captured0；行28 PCNT setup1／clock_blocked0、proof_count0／good0／bad0／ready0、linked0／sealed0。這是在首有效框前的初態，**不能沿用EC44F494的四個proof**或當作本新session正對照已通過。後續guard／完整bundle須取得本session proof與GPIO／CRC。
- 行24初始HX WAIT／RAW=NA／RX0／AGE0，並非有效RAW0；之後RX7→121、HX OK、ERR0／AERR0，持續IDLE／UNCAL。boot窗口只見快照，不是所有attempt。
- capture end UTC05:58:25.2878847、CLOSE05:58:25.2952263，host port_closed=true／failure=null。此段已關閉；主代理另開同session觀察窗時由其單獨持COM，不能由本段close聲稱後續當下COM無人使用。

## 啟動異常及時序界線

| 實際證據 | 可支持結論 | 不可推論 |
|---|---|---|
| 行18 UTC05:58:10.7383406 `OLED MISSING: no ACK at 0x3C/0x3D`，前三個STATUS OLED0x00；行35 UTC05:58:14.1417983 `OLED ACK=0x3C`，之後STATUS OLED0x3C | 本輪啟動缺ACK後再次取得ACK；此路徑確實曾失敗，不能以後來ACK抹去 | ACK不是畫面可讀或RGB熄燈目視；不以相鄰時序唯一指定線路／供電原因 |
| 行36 UTC05:58:14.2646665 RX28、AGE624ms／FIFO1；行38 UTC05:58:15.2586260 RX37／FIFO2 | 額外FIFO累計增加發現於兩個相鄰主機快照間，發生在本輪任何DUMP之前 | 不能套用前輪同步DUMP原因，也不能將AGE624當異常取位或確切漏樣量；實際FIFO事件來源與時刻待診斷 |
| FIFO_INIT PRE_N32／SRC83→POST_N0／SRC81，PRE／POST有效 | 原碼初始化前後非同步快取已保留 | PRE在ADXL begin／resetFifo前，不能稱已證明本MCU epoch新OVR；不認定HX與ADXL同因 |
| 新boot短窗ERR0／AERR0、HX後續更新 | 此窗沒有觀察到累計新增HX錯誤 | 不代表舊RAW−320／−80／極端字已修，不能恢復366g校正或材料試驗 |

## 真LINK、PCNT數位計數與完整bundle

本輪完整證據為 [serial](../../calibration/20261028-hx711-hxpcnt-continuous-observe-140454.serial.log)、[JSONL](../../calibration/20261028-hx711-hxpcnt-continuous-observe-140454.lines.jsonl)、[host](../../calibration/20261028-hx711-hxpcnt-continuous-observe-140454.host.json)、[control](../../calibration/20261028-hx711-hxpcnt-continuous-observe-140454.control.log)、[stderr](../../calibration/20261028-hx711-hxpcnt-continuous-observe-140454.stderr.log)。保留 [原v3.0.0分析](../../calibration/20261028-hx711-hxpcnt-continuous-observe-140454.analysis-v3.json)，另存 [v3.0.1修正分析](../../calibration/20261028-hx711-hxpcnt-continuous-observe-140454.analysis-v3.0.1.json)，修正原因見後節。主代理於真事件保存後停止後續reset。

### 原文、控制及session

- Capture start UTC05:58:53.511至end06:04:54.219，360.708秒。179320 bytes與host received／stored一致，1309完整行、0尾，SHA256 **`3AB0F921F4732C305120F03ACCCA6E93D220143A3F9EBBA2B485DA29B8E9383E`**。Ampere由bytes重新掃LF，原文／offset／line_no／所有fields及UTC／monotonic排序均0差異；未改原文。
- 期待／觀察session均DF497E3D、run_conflict=null，所有block版本／session一致；沒有額外banner／READY／reset或意外phase／CAL變化。boot捕獲至本窗之間有未擷取間隔，不能宣稱主機全程連續；本次兩槽逐框suffix由同一boot的RAM保留。
- 本窗僅2個DEVICE_TX：UTC05:58:57.026 `HX_DIAG STATUS`、06:04:43.701 `HX_DIAG DUMP`。每次前最新兩個STATUS均IDLE／UNCAL／HX OK；DUMP前ERR1是已保留的目標事件，沒有清除或重啟。UTC06:04:42.758已見`schema3 slots2 all_frozen1 pcnt_sealed1 any_reason1 link_reason1`。
- 外層BEGIN行710 UTC06:04:43.754、END行1297 UTC06:04:48.923，5.169秒（monotonic5168.7787ms）。兩read schema2／HXF1／唯一PCNT1依序封存，外schema3／complete1／read_only1完整；未在首槽或HXF尾行提早關閉。
- Control原文核對OPEN前115200／8N1／DTR off／RTS off、OPEN／CLOSE／QUIT皆OK，沒有控制錯誤。CLOSE UTC06:04:54.219、close及quit ACK06:04:54.226、process exit0於06:04:54.231；stderr0 bytes、overflow=false／failure=null，該完整bundle窗已釋放COM3。之後主代理另開90秒被動窗，不能用本段close宣稱後窗當下無人持COM。

### ANY與LINK逐框資料

Ampere從原始CSV獨立重算512列各19個LE uint32 CRC、24-bit符號解碼、seq／attempt gap、RX與ERR增量、accepted／DOUT／原25個軟體pulse及可重算flags；0問題。兩槽沒有重疊，共512個不同attempt；工具`overlap_canonical_equal=true`在零重疊情況不構成額外逐字比對。兩槽共同start89839、window600000000µs；均在期限內以POST_FULL／reason1封存、127pre＋trigger＋128post，omitted0／overflow0／seq_wrapped0。

| 槽 | 保留seq／接受狀態 | 觸發及封存CPUµs | frozen計數 | 範圍／限制 |
|---|---|---|---|---|
| ANY mask7 | 2303–2558，256accepted | trigger2430 end266302824／freeze280205700 | 2558=2302覆寫+256保留+0omit | read CPU187–251µs，gap15828–274036µs；RAW−163837至−641 |
| LINK mask1 | 3322–3577，255accepted＋1rejected | trigger3449 end377976872／freeze392060699 | 3577=3321覆寫+256保留+0omit | read CPU186–255µs，gap15907–252044µs；RAW−529408至−163632（含拒絕字） |

ANY先封存，LINK開始失敗讀取377976640比ANY freeze晚97.770940秒，仍取得獨立LINK/HXF/PCNT；這證明本版ANY早封存不切斷後者。seq2559–3321共763個完成attempt的原字未保留於兩段suffix，不是據此認定漏ADC樣本；不以兩槽512列當全部boot資料。

- **accepted異常**：seq2429 FD8074／−163724、RX2428→2429／ERR0；seq2430 **FFFD7F／−641 accepted1、DOUT1、RX2429→2430／ERR0、flags4**，start266302600／end266302824（224µs）；seq2431 FD8003／−163837、flags4，之後恢復鄰近數值。STATUS行455 UTC06:02:37.251亦回RAW−641／RX2430／AGE250／HX OK／ERR0，所以不只是離線解析。兩個鄰值算術右移8位皆為−640，**不**是原規則的精確右移−641候選；不能勉強套先前−594或反推正確字。
- **首LINK**：seq3448 FD7FF4／−163852 accepted，RX3447→3448／ERR0；seq3449 **F7EC00／−529408 rejected0、DOUT0、RX3448不增／ERR0→1、flags1**，start377976640／end377976872（232µs），gap183129µs。原拒絕字未覆蓋有效快取。
- 下一實際seq3450 FD8058／−163752 accepted、RX3448→3449／ERR1，start378228684／end378228906（222µs）；與失敗start間252044µs，後128個皆accepted、ERR1、RAW−163845至−163639。這是診斷hold改動後的取得結果，不是已修復或ADC真值證據。
- 主機首次在相鄰STATUS觀察ERR0→1的界限為UTC06:04:28.651 RX3448／RAW−163852／ERR0到06:04:29.653 RX3455／ERR1。原始逐框提供同bootCPU括弧，但不能將CPU時間直接換成精確UTC／物理發生時刻。

### HXF離散點及時序介入

HXF原字、seq3449、RX3448、ERR0→1與LINK拒絕列逐欄吻合；原anchor為read_end **377976872**，probe_start377976925（+53µs）、probe_end378228522（+251650µs）。COMPLETE／captured1／next12、valid8／missed4；獨立103word CRC為 **`0x19E8B0B0`**，與原文一致。missed目標2／10／50／5000µs均為NA，不能補LOW；COMPLETE不表示12點都有效。

| 目標µs | 實際begin–end相對原anchor（µs） | DATA |
|---|---|---|
| 200 | 378–383 | LOW |
| 1000 | 1001–1002 | LOW |
| 2000 | 2001–2002 | LOW |
| 20000 | 20665–20670 | LOW |
| 50000 | 50645–50650 | LOW |
| 100000 | 100655–100660 | LOW |
| 150000 | 227695–227708 | LOW |
| 250000 | 251645–251650 | LOW |

150000µs目標實際begin晚77695µs但仍早於下一target；最後目標begin／end晚1645／1650µs，不能稱準時或硬上限250ms。最大兩個有效離散括弧間隙為100660→227695，**127035µs**。8個LOW並不證這些間隙全程LOW，PCNT也沒有提供edge落在何時。

### PCNT GPIO與本boot正對照

PCNT COMPLETE／mode2／reason0、linked1／seq3449／anchor377976872、setup1／clock_blocked0、proof4／good4／bad0／ready1、stage_mask15／sealed1。Ampere直接重建19個metadata＋32個GPIO＋8×40個proof＋34個HOLD，共405個LE uint32，獨立CRC **`0x4E0D8C75`** 與原文相同。未使用proof4–7原文NA、CRC內部固定零；沒有憑空湊八筆正對照。

SCK四階段function2／signal256／drive2，flags為 **BASE28→DRIVER29→READY28→EVENT28**，DATA四階段皆2／256／2／5。driver暫開PU後已復原，SCK原OE／OECTRL、route／drive與pull保留、DATA無output；BASE本已有IE1。EVENT讀回在此真LINK上實際取得，與本版首輪no-event的EVENT=NA不同。

| 本boot proof seq／bits（RAW） | SCK／DATA count＝預期 | SCK START／STOP CPU括弧 | DATA START／STOP CPU括弧 | 包絡前／後餘量 |
|---|---|---|---|---|
| 1／FD8027（−163801） | 50／7＝7 | 442566–442604／442957–442960 | 442617–442619／442874–442896 | 13／61µs |
| 2／FD8042（−163774） | 50／9＝9 | 535538–535576／535810–535813 | 535581–535583／535778–535784 | 5／26µs |
| 3／FD805D（−163747） | 50／9＝9 | 628538–628576／628810–628813 | 628581–628583／628778–628784 | 5／26µs |
| 4／FD8076（−163722） | 50／9＝9 | 719538–719576／719810–719813 | 719581–719583／719778–719784 | 5／26µs |

四筆accepted／postcheck均1，SCK起末LOW／DATA起LOW末HIGH；八個unit API15／attempt7／overflow0／valid1、各CPU API及level括弧順序／小於1秒、DATA預期轉變數及4good／0bad分類已獨立核算。這些是本DF497E3D資料，不沿用EC44F494。seq1–4不在後段兩槽suffix，不能再與不存在的同期HXRD列逐欄交叉比對；CRC及本地正對照只支持匯出完整性與counter路徑，不驗ADC字真值、最小脈寬或外部電壓。

### 首LINK有效計數窗

| unit | START API CPUµs | STOP API CPUµs | READ API CPUµs | count／API／attempt／overflow／valid | 起／末level離散讀取 |
|---|---|---|---|---|---|
| SCK25 | 377977168–377977206 | 378228637–378228639 | 378228639–378228642 | **0／15／7／0／1** | 377977207–208 LOW／378228635–636 LOW |
| DATA33 | 377977215–377977218 | 378228541–378228571 | 378228576–378228610 | **4／15／7／0／1** | 377977218–219 LOW／378228536–541 LOW |

兩unit皆成功CLEAR／START／STOP／READ、count在0..16383半限內、overflow0、每段API／level順序及總窗小於1秒；這是可解讀的有效零SCK計數與有效DATA4計數，不是NA補0。

- 依API括弧契約，SCK start_end377977206≤DATA start_begin377977215，餘9µs；DATA stop_end378228571≤SCK stop_begin378228637，餘66µs。DATA可能被計數的整個啟停括弧窗被SCK較大有效窗包住，而非假設同時啟停。
- SCK START相對anchor為+296–334µs，DATA為+343–346µs；原postcheck後最初343µs未被DATA counter覆蓋，343–346µs內的真正啟動邊界未定，至346µs後才確定已開始。此早期盲區不因HXF probe_start=+53而消失。event_begin377976937（+65）、event_end377977220（+348）保留GPIO驗證及API前置開銷，正對照和PCNT改時序已明示。
- DATA start_begin至stop_end為251356µs、SCK為251471µs；依API括弧，確定active的中央DATA區間start_end→stop_begin為251323µs，真正閘門邊緣只被括在API內，不是外部波形實測。
- HXF最後point end378228522，PCNT finish_begin378228527；先DATA stop、後SCK stop，read完成378228642、finish_end378228644。下一HX seq3450 start378228684，比SCK stop_end晚45µs、DATA stop_end晚113µs、finish_end晚40µs，沒有把下一合法25pulse train混入HOLD計數。

本次**在正對照、GPIO及API有效性契約支持下，取得DATA counter報4個雙邊緣、而包含它的SCK counter報0個邊緣**。這支持觀察窗內MCU側DATA有被計數的數位變化，並非單靠8個離散LOW就能推成一直LOW；可以縮小「失敗後MCU讀到的DATA完全不變」假說。

此計數不附edge時間或方向，**不能直接稱兩個HIGH脈衝、ADC自主轉換／自主恢復、HX端沒有clock、電源良好或根因已確診**。數位counter的閾值／頻寬、同步取樣、噪聲與可能未計到的短脈衝沒有獨立儀器驗證；SCK0不能擴張成物理SCK全程恆LOW，DATA4也不是24-bit資料正確性。診斷LOW hold刻意改變下一次讀取時序，不能把正常下一字當修復證據。

### 一般狀態與DUMP後副作用

355個不同RX的STATUS，首UTC05:58:54.401 RX397、末06:04:54.074 RX3631；皆IDLE／UNCAL，ERR0→1、AERR0，RAW快照−163884至−641。DUMP前FIFO2包含前述啟動1→2，不能把它全歸到後來匯出。

DUMP後行1298 UTC06:04:49.057為HX STALE／AGE5320ms、ADXL MISSING、FIFO2；前一STATUS UTC06:04:43.713。下一行1300 UTC06:04:50.059恢復HX／ADXL OK、FIFO3，之後至末ERR1／AERR0／FIFO3不增。這是同步匯出凍結後的不同排程條件，已知DUMP阻塞另列；不能回因於早於DUMP的ANY／LINK／PCNT edge，也不能定量漏樣或未查診斷的警示根因。

### 離線proof時間欄位修正

原v3.0.0工具把HOLD用的`elapsed_start_us=unsigned(start−failure_anchor)`也套在事件前proof，產生3917432990等回捲數值。這是可確認的離線呈現scope瑕疵，不能解讀成失敗後約65分鐘；原始CPU時間、GPIO、計數、CRC及真HOLD不受影響。Ampere先回報主代理與Gibbs，由其修正工具，未改FW／未新燒錄。

原analysis SHA256為`BDA9F714BFFAB9D2763C9078CFBAAD38E2F4B7ED76FAD28ECD8DDDC196F96377`，保留不覆寫。v3.0.1另存analysis SHA256為**`61A34F827BB91258C06CE1B262AD9FB880BA58BB6350F311437E3FB508EBD778`**；工具SHA：`hx-failure-diagnostic.cjs`=`F56C7838F15CA05FA3CB03C22E01A8649602CECAAFFAAD21C7F5EC9645FF8DC1`、`hx-pcnt-diagnostic.cjs`=`64B43AB135EB0E4178EFCAF55441C26BE64CB9D5FCC0FDCD92E1AFD52A377F85`。

Ampere逐leaf比原摘要與現工具重跑，差異只有分析版本、8個proof elapsed改null及新增PROOF／HOLD observation_scope；HOLD相對開始仍296／343µs、valid_count0／4，其餘完整物件不變。從原始serial用v3.0.1重跑與新封存analysis整物件完全一致。此修正僅解除該離線欄位問題，沒有解除資料品質或物理根因問題；主機測試不代實板。

## 匯出後90秒零TX被動窗

證據：[serial](../../calibration/20261028-hx711-pcnt-event-postdump-passive-141043.serial.log)、[JSONL](../../calibration/20261028-hx711-pcnt-event-postdump-passive-141043.lines.jsonl)、[host](../../calibration/20261028-hx711-pcnt-event-postdump-passive-141043.host.json)、[control](../../calibration/20261028-hx711-pcnt-event-postdump-passive-141043.control.log)、[stderr](../../calibration/20261028-hx711-pcnt-event-postdump-passive-141043.stderr.log)。這是另一個獨立capture；與事件窗末之間有未擷取缺口，不當成連續逐框，也沒有凍結後的新ring／PCNT事件紀錄。

- host PASSIVE_CAPTURE_START UTC06:09:12.906、END06:10:43.369，90.463秒；最後CLOSE期間另收到32 bytes，因此END當下total29764、final received／stored／raw均 **29796 bytes**。該正常收尾差異保留，不任意刪bytes湊計數。
- 180完整行＝90 STATUS＋90 WiFi，另有 **96-byte不完整尾片**，停於`# STATUS ... ADXL=OK AER`。尾片保留原文、不補字、不當完整狀態，不納入以下90筆統計。Ampere核原文／offset／line_no／所有fields／UTC及monotonic排序0差異，final SHA256 **`CC3E8A9969B7A27B3656DD0240BEC36DA1DAE34E104CE3A48E222EE09782589C`** 與host相同。
- 完整STATUS首UTC06:09:13.027至末06:10:42.379，90個不同RX5997→6814；全IDLE／UNCAL／HX OK／ADXL OK，ERR1／AERR0／FIFO3不增、RAW−163869至−163662。只支持本窗約1Hz快照中的累計值及近期更新，不能證故障已修、全ADC框正常或精度通過。
- 沒有DEVICE_TX或額外DTR／RTS切換；Control原文DESCRIBE為115200／8N1／DTR off／RTS off，OPEN／CLOSE／QUIT均OK、無控制錯誤。CLOSE UTC06:10:43.369、close及quit ACK06:10:43.373、process exit0於06:10:43.376；stderr0、overflow=false／failure=null、埠正常釋放。
- 此窗沒有重新讀version／session；依主代理無reset操作流程承接前窗，serial未見新boot或RX回退，但不將缺口推成已連續監看供電／狀態。前事件PCNT證據與新被動窗功能分開判，事後無新ERR不能抹除accepted−641及LINK事件。

## 新增附錄：14:45–14:50同版被動快照異常

主代理在每讀框sidecar仍處設計階段時，保持目前裝置狀態做一次5分鐘零TX被動記錄，沒有新燒錄或重新武裝ring。證據：[serial](../../calibration/20261028-hx711-pcnt-frame-design-passive-145034.serial.log)、[JSONL](../../calibration/20261028-hx711-pcnt-frame-design-passive-145034.lines.jsonl)、[host](../../calibration/20261028-hx711-pcnt-frame-design-passive-145034.host.json)、[control](../../calibration/20261028-hx711-pcnt-frame-design-passive-145034.control.log)、[stderr](../../calibration/20261028-hx711-pcnt-frame-design-passive-145034.stderr.log)。操作者將此窗依未重啟的操作流程承接DF497E3D；本窗沒有重新查version／session，不能由RX增加或未見banner單獨證明窗外供電／RAM連續性。

### 原始完整性及控制

- Capture UTC06:45:33.593–06:50:34.164（台北14:45:33.593–14:50:34.164），300.571秒；99095 bytes與host received／stored及END total一致，SHA256 **`749B4CEAEDE14D58DEA972E3C1FD70191B7CA055F8BF6A9EC9ABDBEF818565E1`**。Ampere重新掃原文LF，598完整行、其中299 STATUS，另有128-byte不完整STATUS尾片；原文／byte_offset／line_no／全部fields／UTC及monotonic排序均0差異。
- 尾片停在`... UP=0 DN=0 POL=-`，原樣保留；沒有補足POL或將尾片算作第300個STATUS，以下統計只用299完整筆。
- 299個不同RX，首行UTC06:45:34.074 RX25977、末行597 UTC06:50:33.169 RX28716；皆IDLE／UNCAL／HX OK／ADXL OK，ERR3／AERR0／FIFO3全窗不增。快照RAW範圍−164145至−1282，並非全部ADC框。
- `device_commands_sent=[]`、沒有DEVICE_TX；實際Control DESCRIBE核對OPEN前115200／8N1／DTR off／RTS off，OPEN／CLOSE／QUIT皆OK、沒有控制錯誤。CLOSE UTC06:50:34.164、close及quit ACK06:50:34.165、process exit0於06:50:34.168；stderr0、overflow=false／failure=null，COM3正常釋放。主機監控工具ACK與ESP32資料依然分列。

### 新異常的原文與限定結論

| 原始JSONL行 | 主機收行UTC（加08:00為台北） | RAW／RX | HX／ERR／AGE／FIFO |
|---|---|---|---|
| 113 | 06:46:30.283 | −164000／26491 | OK／3／167ms／3 |
| 115 | 06:46:31.285 | **−1282／26498** | **OK／3／250ms／3** |
| 117 | 06:46:32.285 | −164028／26508 | OK／3／143ms／3 |

Ampere從serial及JSONL獨立確認三筆一致，並在299個完整STATUS相鄰差中重算監控篩選：只有這一個離群快照造成兩個至少1000counts的變化，入變化+162718、出變化−162746。host兩個`RAW_JUMP_OBSERVED`事件與行115／117的值、RX及收行時間吻合；1000counts是偵測提示，**不是校正容許值或材料驗收門檻**，不能把同一次離群的進出差當成兩次獨立HX故障。

左右兩個快照的帶符號算術右移7位分別為`−164000 >> 7 = −1282`、`−164028 >> 7 = −1282`。這只是事後數值模型相容：兩鄰RX間隔分別7與10，不是相鄰ADC／讀框原字；沒有此異常的24-bit欄、read CPU括弧、實際SCK／DATA計數或當時的LINK逐框狀態。因此**不能證明少了7個clock／7位、不能認定與−594／−156／−641同因、不能以反向位移修RAW或猜正確重量**。異常時HX OK、ERR維持3，足以維持「後檢／健康旗標不能保證RAW品質」的限制，不能藉未新增ERR判成量測合格。

本報告前一已核90秒窗末UTC06:10:42.379為ERR1；此新窗首UTC06:45:34.074已為ERR3。這兩端相差+2，但本附錄未含中間的連續逐框證據，增加時刻及原因未知，**不**把+2歸到14:46:31的−1282、任何特定查詢／匯出或供電動作。即使主代理另有中間資料，也須另以其原文判讀，不能由這兩端補造時間。

既有ANY／LINK／HXF／PCNT已封存，這次STATUS不會為RAW−1282追加新的逐框或edge記錄。不得把先前seq3449的DATA4／SCK0直接套在RX26498，也不能用先前CRC補足新事件缺口。原−1282保留，維持UNCAL／校正暫停及物理根因未定；本窗完整性與零TX控制子項通過，**資料品質仍未通過**。

## 新增附錄二：15:04–15:09零TX窗ERR3→4→5

證據：[serial](../../calibration/20261028-hx711-pcnt-frame-design-passive-150900.serial.log)、[JSONL](../../calibration/20261028-hx711-pcnt-frame-design-passive-150900.lines.jsonl)、[host](../../calibration/20261028-hx711-pcnt-frame-design-passive-150900.host.json)、[control](../../calibration/20261028-hx711-pcnt-frame-design-passive-150900.control.log)、[stderr](../../calibration/20261028-hx711-pcnt-frame-design-passive-150900.stderr.log)。這是既有pcnt1／ring已封存狀態下新增的獨立被動窗，沒有新燒錄、重啟、設備查詢或校正；本窗沒有version／session讀回，同boot歸屬依主代理操作追溯，不能只由快照證明所有窗外供電及RAM連續性。

### 保存完整性與狀態

- Capture UTC07:03:59.617–07:09:00.304（台北15:03:59.617–15:09:00.304），300.687秒。99097 bytes與host received／stored／END total一致；SHA256 **`44511043AC6E8466CBF529AE24FF9674CEB34115FE9390B60754AEB113859D4D`**。598完整行／299完整STATUS，另有128-byte不完整STATUS尾，停於`... UP=0 DN=0 POL=-`。Ampere由原始bytes重掃LF，原文／offset／line_no／所有fields及UTC／monotonic排序均0差異；尾片保留、不補字、不算第300筆。
- 299個不同RX，完整STATUS首UTC07:04:00.148 RX36085／ERR3、末07:08:59.307 RX38822／ERR5；全IDLE／UNCAL／HX OK／ADXL OK，AERR0／FIFO3不變。RAW快照範圍−164356至−164081；獨立重算相鄰完整快照沒有至少1000counts的大跳，**但HX ERR實際新增2**，不能用RAW範圍或HX OK包裝成資料品質通過。
- 前一已核14:50窗末ERR3與本窗首ERR3相同，是跨窗兩端的累計觀察；本附錄沒有合併中間紀錄。以下3→4→5直接由此窗相鄰原始STATUS限定，不沿用先前ERR1→3缺口去猜時刻。

### ERR首次新增的主機觀察界線

| 變化 | 最後見舊ERR的完整JSONL行／台北收行時刻 | 首次見新ERR的完整JSONL行／台北收行時刻 | 同期快取RAW／RX／AGE |
|---|---|---|---|
| ERR3→4 | 行245／15:06:02.629 | 行247／15:06:03.635 | −164239／37209／207ms → −164271／37218／197ms |
| ERR4→5 | 行313／15:06:36.764 | 行315／15:06:37.767 | −164156／37521／169ms → −164186／37529／157ms |

這些是**相鄰主機接收快照中首次發現累計增加的界線**，不是HX錯誤實際發生的精確UTC。ERR新增當筆的HX仍OK，表示該快照保留的是近期成功資料狀態；表中RAW為有效快取，**不是被拒絕框的24-bit字或錯框RAW**。RX快照跨度分別9及8，不提供其間每個attempt或確切拒絕序號。既有ANY／LINK／HXF／PCNT已封存，無本次錯誤的逐框／edge資料，不能將先前seq3449的DATA4／SCK0回填到這兩次增加。

host `HX_ERR_INCREASE_OBSERVED`兩個事件的previous／current／RAW／RX與原文吻合；第一次告警處理UTC07:06:03.636比完整行收時07:06:03.635晚1ms，使用JSONL收行時間描述首次觀察，不冒充事件處理時刻即ADC時刻。

### 零TX與收尾

`device_commands_sent=[]`且沒有DEVICE_TX；Control原文DESCRIBE核對OPEN前115200／8N1／DTR off／RTS off，OPEN／CLOSE／QUIT皆OK、無控制錯誤。CLOSE UTC07:09:00.304、close ACK07:09:00.305、quit ACK07:09:00.306、process exit0於07:09:00.308；stderr0 bytes、overflow=false／failure=null、該capture正常釋放COM3。

兩次ERR增加是在沒有本窗主動設備命令下被觀察，不能歸到本窗不存在的查詢TX。原韌體的定期STATUS／OLED／WiFi等活動仍存在，零TX不等於零CPU／匯流排負載；此窗也不能唯一排除或確認時序、clock、線路、供電或與舊RAW尖峰的同因。原始值和警示照實保存，不清除、修值或擅自重校；證據保存／控制子項通過，錯誤新增與既有資料品質失敗維持待解，CAL366仍暫停。

## 本輪檢核方法與未測路徑

主代理單獨收集DF497E3D，不由Ampere開COM。上述完整bundle已按下列方法核對，未觸發或沒有獨立儀器的子項仍保留：

1. 期待／實際session、自然boot至觀察窗連續性限制、沒有額外boot／意外phase或CAL；raw bytes／SHA／JSONL／時間／尾片／實際TX及close／quit／exit已核；目視／供電未獨立量測。
2. ANY及LINK各自trigger／deadline／固定metadata，attempts=overwritten+retained+omitted；截止後未分類不得算無事件。兩槽重疊逐19word比對，只計唯一attempt。
3. 每列CRC／符號位元／RX與ERR／原接受狀態、HXF103word與PCNT405word獨立重算；只以完整外schema3 END結束，不以首槽或HXF尾行截斷。
4. **本session** BASE／DRIVER／READY／EVENT及最多8筆proof，4good／0bad才valid；GPIO復原、每unit API／mask／NA／半限overflow及SCK包DATA時窗。首LINK必須與HXF／PCNT seq／原read_end anchor綁定，counter停後才有resume。
5. 本次真LINK已發生並有限通過事件資料擷取；保留啟停盲區、有效／missed、count與level，不能由MCU總數推HX端波形或唯一根因。硬體busy／截斷／overflow／API故障／wrap與最小脈寬／獨立供電等未測；DUMP後FIFO2→3與啟動1→2分列。

## 待補與責任

| ID／程度 | 狀態 | 負責人／重測條件 |
|---|---|---|
| PCNT1-C02-Q01／P2 | precheck／一次重啟／自然版本及新session／原文時間映射／完整匯出子項通過；目視未測 | 主代理＋Ampere；不回填舊epoch、不以ACK代目視 |
| PCNT1-C02-Q02／P2 | 本session GPIO／proof／真LINK PCNT與CRC及時窗包絡通過；HXF8LOW／4missed、PCNT DATA4／SCK0可信但僅MCU數位總數 | Gibbs＋Kierkegaard＋Ampere；保留早期盲區及無edge定位，若需更強結論須額外可辨識證據，不重啟追正常值或擅加clock |
| PCNT1-C02-Q03／P2 | 啟動OLED缺ACK與DUMP前FIFO1→2、匯出後STALE／MISSING及FIFO2→3分列；根因未全部定位 | Gibbs追蹤快取與排程；Planck保留時間／原文，禁止以DUMP解釋尚未DUMP的事件或直接指定HX同因 |
| PCNT1-C02-Q04／P1 | accepted−641／ERR0與後續快取RAW−1282／HX OK／ERR3重現品質失敗，LINK拒絕−529408及數位變化根因仍未定，校正暫停保持 | 原因及必要修復充分驗證後，才做新近時TARE／CAL366與卸載／重放／額外重量驗收；CRC、右移相容、下一正常字及事後無錯均不能解除 |
| PCNT1-C02-Q05／P2 | 離線v3 proof elapsed無號回捲呈現瑕疵；v3.0.1修正scope／null並由原文重跑整物件核對通過 | Gibbs修工具、Planck保留新舊analysis；僅此欄位修正完成，不當成韌體或HX根因修復 |
| PCNT1-C02-Q06／P2 | 14:45被動窗證據完整、299快照及新−1282已保存；ERR1→3跨窗缺口未定位，ring已封存無此事件逐框／edge | Gibbs設計後續可辨識診斷、Kierkegaard限定候選模型、Planck保留新窗追溯；本缺口不是可由舊PCNT結果或修RAW補足的資料 |
| PCNT1-C02-Q07／P1 | 15:04零TX被動窗直接觀察ERR3→4→5，HX全OK且未見大跳不能解除新增錯誤；同期cache RAW不是拒絕字，無新逐框／edge | Gibbs追查並依AGENTS驗證後續診斷、Kierkegaard核時序界線；Planck保留相鄰主機收行及快取限制，禁止把未定位原因直接填成軟體／供電故障或允許CAL |

PCNT真事件擷取目標已達，主代理已停止循環reset；保持供電／UNCAL、禁止盲目CAL。已交precheck／boot／完整bundle、90秒及兩個新增5分鐘被動窗均正常關閉、COM3已釋放；若主代理後續另開新窗，須另記當時持埠狀態，不以本次close涵蓋未來。Ampere未開COM、未重置／燒錄、未改韌體或其它工程檔，未commit／push。本輪報告及新增附錄交主代理、Gibbs、Kierkegaard及Planck；後續依既有授權與可辨識的新證據追查，不重啟追正常結果或將本計數等同根因修復。
