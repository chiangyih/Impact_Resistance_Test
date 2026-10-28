# 2026-10-28 HX711校正、量測資料與改善評估

整理者：Kierkegaard，依公開串列、逐行時間、階段紀錄及獨立QC離線核對。日期／時區：2026-10-28，Asia/Taipei（UTC+08:00）。本文件只新增評估，不操作COM、不改原始資料或韌體、不執行Git；不使用先前OLED數值作定量結論。

**目前可確認兩次CAL 241及三次TARE程序有成功回應，但重量量測品質尚未通過；CAL 366尚未送出／成功建立新比例。後續診斷重啟已另立UNCAL執行輪次，不能沿用舊係數。** 已部署的PCNT1取得失敗後數位變化證據，尚未定位根因；每讀框PCNT候選主機檢查通過後，正式連結因DRAM超出40400 bytes失敗，未部署。程序、資料保存、功能QC、量測精度及材料效果分開判定。[校正成功原文索引](#校正輪次與參數有效範圍)、[PCNT真事件QC](../quality/reports/2026-10-28-pcnt1-continuous02-qc.md)、[候選主機驗證](20261028-pcnt2-public-host-validation.json)、[正式連結失敗](20261028-pcnt2-compile-failed.log)。

## 證據口徑與未知

- RAW單位為counts。重量換算只在指定同輪列印參數仍適用的條件下使用 `mass_g = (RAW - offset) / counts_per_g`；不把CAL_USER旗標當精確參數讀回。TARE會使舊比例失效，RAM校正不能跨重啟沿用。[校正程序及狀態](2026-10-27-hx711-authorized-auto-calibration.md)、[重啟輪次界線](2026-10-28-hx711-per-read-diagnostic-session.md)。
- STATUS為約每秒的快取快照；同RX重印不能當成不同框。不同RX也不是全部ADC轉換覆蓋。逐框ring僅保留指定前後段；未保存區間不補值、不插值、不拼成連續資料。[USB重接後session](2026-10-27-hx711-usb-reconnect-serial-session.md)、[逐框QC](../quality/reports/2026-10-28-hxread1-qc.md)。
- JSONL時間是主機收到完整行的時刻；phases是請求或處理使用者回覆的時間，並非物理放置／取下時刻。裝置boot微秒是CPU括弧，並非ADC轉換或管腳邊緣時間。[366g第一輪時間定義](2026-10-27-hx711-366g-continuous-load-unload.md)、[PCNT真事件時間定義](../quality/reports/2026-10-28-pcnt1-continuous02-qc.md)。
- 空托盤、物品放穩、供電持續及環境未改變等屬使用者回報，沒有同步受力、溫度或電壓量測。366g來自使用者所稱「標準磅秤」，但型號、解析度、校正追溯及不確定度未知；241g亦未取得獨立不確定度資料。未知不能當成零誤差。[重量來源](2026-10-27-hx711-366g-continuous-load-unload.md)、[241g條件](20261027-hx711-241g-calibration-summary.json)、[環境回報界線](2026-10-27-hx711-negative-drift-investigation.md)。

## 校正輪次與參數有效範圍

早期兩輪沒有完整自然boot／session識別，因此以下以校正操作及USB重接背景區分，不補造精確重啟時刻或boot ID。

| 輪次 | 已確認命令及列印參數 | 後續結果與限制 | 精確證據 |
|---|---|---|---|
| A：10/27 16:57／16:59 | 一次TARE成功，offset=−157326.312 counts；一次CAL 241成功，scale=20.413381743 counts/g | 同一標定物仍留置的11個不同RX快照換算均值240.089532g、範圍237.996431～242.895178g。不是獨立準確度；首次卸載開埠失敗，沒有串列回零結果 | [TARE原文](20261027-hx711-241g-tare-165741.serial.log)、[CAL原文](20261027-hx711-241g-cal-165924.serial.log)、[卸載失敗host](20261027-hx711-241g-unload1-170057.host.json)、[核算摘要](20261027-hx711-241g-calibration-summary.json) |
| B：USB重接後10/27 18:23／18:24 | 先見UNCAL新串列；一次TARE成功，offset=−158294.438 counts；一次CAL 241成功，scale=20.577282158 counts/g | 留載、卸載及一次重放如下表；零點沒有保持在相同位置。參數為該輪裝置列印值，不移用至後續UNCAL／新boot | [新基線原文](20261027-hx711-241g-auto-capture-182156.serial.log)、[新TARE原文](20261027-hx711-241g-new-tare-182308.serial.log)、[新CAL原文](20261027-hx711-241g-new-cal-182456.serial.log)、[完整session](2026-10-27-hx711-usb-reconnect-serial-session.md) |
| C：10/27 23:51新TARE | 一次TARE成功，offset=−160216.281 counts；裝置轉UNCAL，舊scale失效 | 跨夜CAL前守衛阻止命令；其後近時空載前檢再見尖峰與ERR增加，新TARE／CAL停止。不能將此offset配回輪B比例 | [TARE原文](20261027-hx711-366g-authorized-tare-235135.serial.log)、[未送CAL的host](20261028-hx711-366g-authorized-cal-070616.host.json)、[授權校正紀錄](2026-10-27-hx711-authorized-auto-calibration.md) |
| D：10/28診斷重啟各輪 | 各自新session／UNCAL；沒有建立新有效重量比例 | USB仍接著不保留RAM校正；不跨boot拼接ERR增量或重量係數 | [逐框部署session](2026-10-28-hx711-per-read-diagnostic-session.md)、[雙槽重啟session](2026-10-28-hxdual-restart-session.md)、[PCNT session](2026-10-28-hxpcnt-diagnostic-session.md) |

輪A的TARE末快照ERR為0、CAL首快照已為1，增加發現於未保存缺口，不能指定時刻／原因；COM消失亦不單獨證明MCU重啟。該輪卸載量測缺失不是0g。[輪A核算與限制](20261027-hx711-241g-calibration-summary.json)。

輪B固定列印參數下的串列觀察如下；每列保留原窗口，未為接近目標而移窗或重去皮。

| 階段 | 不同RX數 | RAW均值（counts） | 條件換算均值（g） | 精確證據 |
|---|---:|---:|---:|---|
| CAL後同物留載 | 9 | −153335.444444 | 240.993612 | [CAL後原文／時間](20261027-hx711-241g-new-cal-182456.lines.jsonl) |
| 首次卸載 | 59 | −158310.322034 | −0.771921 | [首次卸載原文／時間](20261027-hx711-241g-auto-capture-182633.lines.jsonl) |
| 同一241g重放 | 60 | −153337.316667 | 240.902627 | [重放原文／時間](20261027-hx711-241g-reload-capture-182908.lines.jsonl) |
| 第二次卸載 | 60 | −158379.850000 | −4.150791 | [第二卸載原文／時間](20261027-hx711-241g-auto-capture-183040.lines.jsonl) |
| 空載續查 | 60 | −158439.016667 | −7.026130 | [續查原文／時間](20261027-hx711-241g-auto-capture-183158.lines.jsonl) |

上表計算與[輪B session](2026-10-27-hx711-usb-reconnect-serial-session.md)一致。CAL內部32個新框沒有逐筆輸出，STATUS不是那些內部框的替代。留置及重放同一標定物不構成獨立精度驗證；均值接近241g或0g不能掩蓋再卸載及續查的偏移。不同capture間未擷取時間不能用來估精確漂移率。

輪C的TARE成功至翌日首個CAL前快照相隔7小時14分20.323秒；跨日不是重啟證據，但已有零點變化及未擷取缺口，不應以昨晚零點直接建立隔晨最終比例。檔名含authorized-cal不代表已送CAL；host明列送出前守衛停止。[跨夜原文與計算](2026-10-27-hx711-authorized-auto-calibration.md)。

## 366g兩次連續載卸：有響應，尚不是CAL366

兩輪均保留原241g輪B參數，未送TARE／CAL。固定分析規則為：B0取放置請求前60秒；L取收到「放穩」確認後建立窗口起60秒；B1取收到卸載確認後建立窗口起60秒。不是依結果挑窗，也不把請求／回覆時間當物理動作時刻。[第一輪phases](20261027-hx711-366g-continuous-220458.phases.jsonl)、[第二輪phases](20261027-hx711-366g-repeat-223853.phases.jsonl)。

本文件從下列JSONL依上述固定邊界獨立重算；各窗恰有60個不同RX。SD為快照樣本標準差（分母n−1），不假定各快照獨立同分布，不換成準確度或均值置信區間。

| 輪次／窗口 | n | RAW均值（counts） | SD（counts） | 精確原始來源 |
|---|---:|---:|---:|---|
| 第一輪B0 | 60 | −159806.3333 | 50.1752 | [第一輪JSONL](20261027-hx711-366g-continuous-220458.lines.jsonl) |
| 第一輪L | 60 | −152183.3667 | 50.1043 | 同上，依第一輪phases固定載重窗口 |
| 第一輪B1 | 60 | −159578.7500 | 50.5219 | 同上，依第一輪phases固定卸載窗口 |
| 第二輪B0 | 60 | −159458.7000 | 41.4583 | [第二輪JSONL](20261027-hx711-366g-repeat-223853.lines.jsonl) |
| 第二輪L | 60 | −151842.4000 | 43.2855 | 同上，依第二輪phases固定載重窗口 |
| 第二輪B1 | 60 | −159384.3167 | 41.5768 | 同上，依第二輪phases固定卸載窗口 |

| 比較量 | 第一輪（counts） | 第二輪（counts） | 意義 |
|---|---:|---:|---|
| L−B0 | +7622.9667 | +7616.3000 | 相對前空載的載重增量 |
| L−B1 | +7395.3833 | +7541.9167 | 相對後空載的載重增量 |
| B1−B0 | +227.5833 | +74.3833 | 前後空載均值差；不是新比例 |

差值與[第一輪紀錄](2026-10-27-hx711-366g-continuous-load-unload.md)、[第二輪紀錄](2026-10-27-hx711-366g-repeat-load-unload.md)吻合。除以使用者回報的366g，第一輪兩側表觀尺度分別20.8277778／20.2059654 counts/g，第二輪為20.8095628／20.6063297 counts/g；它們沒有寫入裝置，也不是已驗證的新係數。

若只引用輪B保存的20.577282158 counts/g，兩側增量條件換算為第一輪370.4555／359.3955g、第二輪370.1315／366.5167g，前後空載差分別約11.0599／3.6148g。[第一輪條件換算](2026-10-27-hx711-366g-continuous-load-unload.md)、[第二輪條件換算](2026-10-27-hx711-366g-repeat-load-unload.md)。這些克數依賴保存係數仍適用；當時沒有重新讀回RAM精確係數。

兩側基線不是載重同時的零點，因此不能將兩個增量當成真實重量的上下界或置信區間，不能選較接近366g者驗收，也不能取平均便聲稱消除漂移。第二輪前後基線差較小只是一項觀察，物品來源誤差、放置位置、等待及熱／機械歷程仍未獨立控制；兩輪不足以給出正式重複性或線性結論。第一輪ERR7→8、第二輪ERR8→9均保留，不能因三窗均值可算而忽略。[第一輪錯誤界線](2026-10-27-hx711-366g-continuous-load-unload.md)、[第二輪錯誤界線](2026-10-27-hx711-366g-repeat-load-unload.md)。

## 空載慢變與離群字分開處理

空載零點變化與被接受的近零／極端字是不同的觀察尺度，現有資料未證明同一根因。部分連續窗有負向趨勢，但跨全歷史並非已證明單調負漂移。

例如18:35起同一個3分鐘空載窗，三個60秒RAW均值為−158557.3500／−158573.5000／−158582.1167 counts；18:52起另一個3分鐘窗為−158629.9833／−158641.9167／−158654.6833 counts。這支持各自窗內分段均值負向變化，不是溫度、供電、受力或軟體時序的原因鑑定。兩窗間的空白不能補成連續曲線。[原始窗口與分箱](2026-10-27-hx711-negative-drift-investigation.md)。

同文件的WiFi A-B-A並未顯示使零點恢復的明顯效果；單次有序對照不能排除細小、延遲或與時間混合的影響。不能因此宣稱WiFi、熱、供電或機械全部已排除。[WiFi對照及限制](2026-10-27-hx711-negative-drift-investigation.md)。

| 異常證據 | 已驗證事實 | 不可擴張的結論 | 來源 |
|---|---|---|---|
| 10/27第二輪366g過渡期RAW−594 | 原始STATUS存在，HX OK、ERR9；在預定三窗之外，未列入三窗統計 | 左右快照>>8數值相容只是事後候選，不能推少8個clock／修復原字／當瞬時重量 | [第二輪原文與候選界線](2026-10-27-hx711-366g-repeat-load-unload.md) |
| 10/28 07:12空載RAW−156 | 原始STATUS存在；ERR31→32稍後在快照中首次被觀察 | 左右快照>>10相容，不證明與−594同因；異常後無再現也不解除前檢失敗 | [CAL前檢原文與時間](2026-10-27-hx711-authorized-auto-calibration.md) |
| hxread1／238002AF | seq1922的−317拒絕、ERR0→1；下一seq1923的−4718593卻被接受 | 原後檢HIGH與CRC不能驗字真值，不能用模型反推／替換正確RAW | [逐框session](2026-10-28-hx711-per-read-diagnostic-session.md)、[獨立QC](../quality/reports/2026-10-28-hxread1-qc.md) |
| hxfail1／0462E7C9 | seq2347 RAW−80被接受、ERR0；失敗後DATA未觸發 | 兩側>>11相容僅數值候選；本次沒有hold，不能把下一attempt間隔當hold效果 | [hxfail1 QC](../quality/reports/2026-10-28-hxfail1-qc.md) |
| hxfail2／A1C20320 | 先接受seq3277 RAW−320／ERR0；後seq3804拒絕−160／ERR0→1，取得失敗後DATA | 先前accepted異常不能歸到後來hold；未取點之間不是已知LOW | [續錄session](2026-10-28-hxdual-until-link-session.md)、[真事件QC](../quality/reports/2026-10-28-hxfail2-continuous02-qc.md) |
| pcnt1／DF497E3D | 接受seq2430 RAW−641／ERR0；seq3449拒絕−529408／ERR0→1，後者綁定PCNT真事件 | −641兩側>>8都是−640，並非精確右移候選；不能硬套舊近零模式 | [原始bundle](20261028-hx711-hxpcnt-continuous-observe-140454.serial.log)、[真事件QC](../quality/reports/2026-10-28-pcnt1-continuous02-qc.md) |

### 失敗後離散LOW與PCNT真事件

A1C20320取得9個有效LOW離散點、3個missed／NA；有效括弧間最大缺口171034µs，不能說該窗全程LOW。DF497E3D新增PCNT後取得8個有效LOW、4個missed／NA，並在同一拒絕seq3449上取得有效DATA count=4／SCK count=0，兩unit無overflow、SCK啟停窗包住DATA窗。[前版實際點位](../quality/reports/2026-10-28-hxfail2-continuous02-qc.md)、[PCNT原始bundle](20261028-hx711-hxpcnt-continuous-observe-140454.serial.log)、[PCNT獨立QC](../quality/reports/2026-10-28-pcnt1-continuous02-qc.md)。

DF497E3D的DATA START在原postcheck結束anchor後343–346µs，早段未覆蓋；有效計數閘門長度依API括弧約251323～251356µs。SCK包DATA的前／後保守餘量為9／66µs，DATA STOP結束至下一read開始另有113µs。8個LOW離散點間最大缺口仍有127035µs，PCNT的4個邊緣沒有各自時間，不能定位到哪個間隙。[全部API與離散點原文](20261028-hx711-hxpcnt-continuous-observe-140454.serial.log)、[獨立時序核對](../quality/reports/2026-10-28-pcnt1-continuous02-qc.md)。

正向結論是：在GPIO讀回、有限正對照及API有效性契約下，該MCU數位接收窗有被計到的DATA變化，而包含它的SCK窗沒有被計到的邊緣。因此「後窗中MCU數位DATA一直LOW」的受限模型不符本次觀察。不能把4個雙邊緣稱為兩個已量得的HIGH脈衝，也不能直接判ADC自主轉換、供電良好、HX端沒有clock或原故障已修。filter disabled不表示無限頻寬；數位閾值、短脈衝、雜訊及HX遠端波形均未被獨立儀器驗證。hold刻意延後下一read，後續正常字不是修復證據。[PCNT物理限制與QC](../quality/reports/2026-10-28-pcnt1-continuous02-qc.md)。

### 後續−1282與ERR3→5

已封存ring後的零TX被動窗又觀察到下列快照；沒有這些新事件的逐框或PCNT資料，不能挪用先前seq3449的DATA4／SCK0。

| 台北主機收行時間 | RAW／RX | 當時狀態 | 精確證據 |
|---|---|---|---|
| 14:46:30.283 → 14:46:31.285 → 14:46:32.285 | −164000／26491 → −1282／26498 → −164028／26508 | HX OK、ERR3、FIFO3 | [14:45窗JSONL](20261028-hx711-pcnt-frame-design-passive-145034.lines.jsonl)、[該窗host](20261028-hx711-pcnt-frame-design-passive-145034.host.json)、[QC附錄](../quality/reports/2026-10-28-pcnt1-continuous02-qc.md) |
| 15:06:02.629 → 15:06:03.635 | −164239／37209 → −164271／37218 | ERR3→4；FIFO3 | [15:04起窗口JSONL](20261028-hx711-pcnt-frame-design-passive-150900.lines.jsonl) |
| 15:06:36.764 → 15:06:37.767 | −164156／37521 → −164186／37529 | ERR4→5；FIFO3 | 同上 |

−1282的左右快照算術>>7均為−1282，但RX分別跨7及10，不是緊鄰的讀框原字；不能證明少7個clock、與先前異常同因或還原正確重量。[該窗QC](../quality/reports/2026-10-28-pcnt1-continuous02-qc.md)。

15:04:00.148～15:08:59.307的299個完整STATUS，RAW均值−164217.086957 counts、範圍−164356～−164081，皆UNCAL、FIFO3；ERR仍增加兩次。原始尾端128 bytes保留、不補字、不納完整STATUS統計，host無裝置TX並正常關埠。這只界定錯誤首次被快照看見的時間，不能把較平滑RAW當成無錯／已修，亦不能回推被拒絕框原字、漏樣量或原因。[原始serial](20261028-hx711-pcnt-frame-design-passive-150900.serial.log)、[逐行JSONL](20261028-hx711-pcnt-frame-design-passive-150900.lines.jsonl)、[host與收尾](20261028-hx711-pcnt-frame-design-passive-150900.host.json)。

## DUMP、FIFO及每框候選分開判定

DF497E3D完整DUMP輸出5.169秒，之後首STATUS為HX STALE／AGE5320ms、ADXL MISSING，下一快照恢復更新且FIFO2→3。啟動階段此前已有FIFO1→2，須另列；這些不是同一個已證明原因。DUMP發生在ANY／LINK／PCNT封存之後，不可能用來解釋較早凍結的異常。FIFO警示是累計觀察，不能直接換成漏樣量或HX故障原因。[PCNT真事件QC及匯出時序](../quality/reports/2026-10-28-pcnt1-continuous02-qc.md)。

每讀框側錄候選hxfail4／pcnt2已完成公開主機檢查、合成匯出及有限紙審；[公開驗證摘要](20261028-pcnt2-public-host-validation.json)記錄的是正式連結前狀態，不是部署成功。其後Arduino正式連結明確因`.dram0.bss`超出40400 bytes失敗，沒有燒錄，也沒有該候選的新版實板QC。[正式失敗log](20261028-pcnt2-compile-failed.log)。

因此目前實板事實仍以已部署pcnt1及其QC為準。主代理本次封存以已部署pcnt1公開來源為基準，失敗候選WIP保存在儲存庫外，復原／備份與commit、push實際狀態由[handoff](../../handoff.md)記錄；不把主機通過、動態記憶體構想或原始碼紙審當已解決DRAM／資料品質。

若日後改動態配置，固定雙側錄需求仍為40960 bytes；只是改變配置位置，並未減少總需求。須在共同觀察窗之前單次配置並驗證，保留完整窗口、原故障、同seq／索引及凍結不可變契約；讀框／hold／ISR內不配置或重試。null／部分失敗要明確報診斷不可用，不縮窗、少槽或補0；實板仍須驗heap低水位／最大可配置塊、其他任務及匯出負載。[候選資源與時序設計](20261028-pcnt2-public-host-validation.json)。

## 改善優先序與正向驗收

以下為後續工作建議，並非已完成的測試或新授權硬體操作紀錄。

1. **補齊來源與測量目的。** 固定韌體版本／來源雜湊、裝置session、HX711與荷重元型號／量程／靈敏度／RATE設定及機械安裝紀錄；參考物保留實際重量來源、解析度、校正追溯和不確定度。先依用途訂允許的零點變化、回零、線性及重複性容差，再測試，不以結果反向放寬門檻。現有來源誤差未知，不能給獨立準確度認證。[現有重量來源](2026-10-27-hx711-366g-continuous-load-unload.md)、[目前QC未解項](../quality/reports/2026-10-28-pcnt1-continuous02-qc.md)。
2. **核實接線、供電與受力路徑，逐項改變。** 先記錄HX供電／共地／邏輯電位、SCK與DATA實際走線、荷重元端子、接頭與固定方式、線材牽拉、托盤是否擦碰及溫度／暖機歷程。使用者「無碰觸／環境未變」不能代替量測。可取得儀器時，優先量HX附近的供電、地與腳位波形；沒有儀器則明示缺口，不猜改線、改供電或宣稱已排除電氣因素。[已知現場條件及限制](2026-10-27-hx711-negative-drift-investigation.md)、[現有PCNT能觀察的範圍](../quality/reports/2026-10-28-pcnt1-continuous02-qc.md)。
3. **取得異常讀框本身的實際數位活動與時序。** 目前有效0／4只覆蓋失敗後窗；每框候選尚未部署。若日後資源及QC通過，將事件框計數、原RAW、接受狀態、前後框、API括弧與額外開銷同seq保存；計數失敗／overflow／未知保留NA，不改RAW接受、不修值。MCU計到50個SCK雙邊緣仍不驗25個HX端有效clock、脈寬或間隔；DATA轉變數吻合亦不驗位元序列。沒有計數不符，不能反過來指定類比根因。較強結論需要HX端實際邊緣／類比波形證據。[候選觀測契約及限制](20261028-pcnt2-public-host-validation.json)、[現板真事件QC](../quality/reports/2026-10-28-pcnt1-continuous02-qc.md)。
4. **分開驗資料品質與慢變。** 已確認原因及必要修復後，先以有界觀察驗證問題處理及其他任務；保留所有原始離群、拒絕框和警示，不以重測到安靜、重去皮、平滑或位移補值當修復。以固定真實時間分箱報均值、SD／跨度、首末與段間變化，同時列錯誤／有效性及資料覆蓋；有非平穩或相關性時，不以SD／√n宣稱準確度。未觀察全部ADC框者不報全ADC故障率。[既有慢變與快照限制](2026-10-27-hx711-negative-drift-investigation.md)、[品質失敗與未知](../quality/reports/2026-10-28-pcnt1-continuous02-qc.md)。
5. **根因／資料品質及必要修復經驗證後，再重建近時靜態校正。** 同boot記錄新空載基線，空載實際確認後一次TARE，再由現場放穩366g才CAL；留載、卸載、241g額外重量、再卸載、366g重放及再卸載均固定零點／比例、位置、預定等待與時間窗。接續多次載卸循環，保留動作過渡和失敗，不在每次卸載後追零。報告L−B0、L−B1及B1−B0，不平均掩蓋基線變化；241g額外點只能在其參考不確定度已知時支持相應準確度。實體放置／取下仍需現場完成，不能由舊歷史或RAW猜當前載荷。[已授權校正流程與未測項](2026-10-27-hx711-authorized-auto-calibration.md)、[固定載卸窗口實例](2026-10-27-hx711-366g-repeat-load-unload.md)。

最終報告應分列：程序是否接受、保存是否完整、功能／資源QC、資料品質、零點／回零、跨重量點與重複性、參考不確定度及允許容差。任何尚未測得者保留未知。即使將來靜態質量校正通過，也不能據此宣稱撞擊峰值力、動態頻寬、光學精度或材料效能通過；這些需要各自的量測方法、時間／頻率響應及實驗驗證。[功能與量測界線清單](../quality/post-flash-qc.md)。
