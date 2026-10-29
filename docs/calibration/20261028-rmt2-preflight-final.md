# RMT2 保留政策 preflight（Ampere）

狀態：**最終固定來源與定向重測完成，PF2-10 parser阻擋已解除；沒有剩餘preflash來源阻擋。這不代表實板QC或量測通過。** 本檔只在儲存庫外候選；RMT1 QC仍維持失敗，不覆寫其bad proof或原始證據。Gibbs唯一修改實作／測試／工具，主代理唯一操作COM3；Ampere只修改本檔並唯讀公開來源，不讀秘密或私密build。下方主機native產物由公開來源在專案外臨時目錄生成，不含WiFi或裝置資料。較下方阻擋文字是發現時的歷史，最後解除條件見末節。

本輪意圖是保留RMT1漏存的實際BAD_PROOF及GOOD_PROOF時序。原PCNT正對照valid分類、bad黏性、global proof_ready0守衛、原RAW接受與25pulse區塊、RMT4KiB上限／64symbols／driver clock／634word格式不放寬。新外層schema6與RMT schema2必須與RMT1隔離。

## 三slot的必要語意

| slot | 選擇依據 | 沒有RX／RX失敗時 | 不能推論 |
|---|---|---|---|
| FIRST_LINK | 捕獲窗口內實際append RAW accepted0，與HXF首LINK同seq | 鎖第一seq並保留NA／失敗原因，不跳下一LINK | 不表示完整波形、根因或已修 |
| BAD_PROOF | 本boot PCNT固定proof表中同seq的**實際新增proof**，其原valid0；原字／accepted／postcheck／packed units與此RAW／sidecar一致 | 同樣鎖第一個actual bad；不能因RMT未arm／timeout／wrongptr而重選 | bad不只DATA mismatch，也含API／overflow／後檢／邊界等既定分類；不可改為有效 |
| GOOD_PROOF | 本boot同seq實際proof原valid1，與RAW／sidecar交叉一致 | 鎖第一個actual good，即使RMT缺失／shape異常也保留失敗／前綴 | 一個good不抵銷先前bad、不提升global ready、不證量測或完整波形 |

`PROOF_SOURCE` policy bit只表示準備路徑的請求標記，原8筆上限耗盡後仍可能出現，不能當實際proof存在。查表必須找到唯一同seq的既定proof，並核對bits24／accepted／postcheck與兩unit的六個API時間、raw_count、API／attempt／overflow／valid／level打包資訊。不得從later metadata猜回該框是proof，也不得按RMT重新決定PCNT valid。

若整個RMT runtime heap配置失敗而根本沒有狀態，schema6仍可保留RAW／PCNT與RMT unavailable／全事件NA；這個全不可用例外須明確標記、parser不能要求虛構首LINK／BAD／GOOD事件。「首缺RX也鎖NA」針對有RMT狀態但該job不可用的分支，不能假裝null runtime存過選擇。

## 必要正反例（來源／native／actual exporter／parser）

1. **先bad後good且global ready始終0**：GOOD仍按該actual proof.valid1選中，不能沿用RMT1 `ap.usable()`／global ready門檻而再次漏存；原sidecar依然NO_PROOF，不得被提升為usable。
2. **先good後bad**：兩slot各鎖第一seq；後來good不得覆蓋第一good，bad保持sticky；GOOD資料不能回寫成全系統通過。
3. **8筆耗盡後帶PROOF_SOURCE bit但查不到seq**：不選BAD／GOOD，不偽造新proof；測seq>8及原proof表中不存在的任意seq。
4. **同seq但原字／accepted／postcheck或一個packedunit word不同**：即使重算CRC也拒絕角色／身分對接；相同raw數值不等於同一proof。
5. **BAD各分支**：DATA條件差、START／STOP／READ/API失敗、overflow、count超半限、起末level／包絡不符、原postcheck拒收。只要原actual proof.valid0就按BAD語意保留，不能縮成「只留多2 DATA的accepted框」。
6. **首事件沒有RMTjob**：setup API失敗、未eligible、pending重疊、RX start失敗等，均鎖正確的first proof seq／NOT_ARMED或相應原因；下一個健康job不可替換首筆以湊RMT通過。
7. **首BAD／GOOD timeout／partial／full64／wrongbuffer／duplicate或late callback**：事件選擇不撤銷；safe-copy規則與terminal不再arm、buffer生命及封存CRC不變。安全unit正duration前綴可以保留，不能把不安全unit當0symbol。
8. **LINK與BAD同框**：兩slot的RAW／PCNT／RMT canonical應相同、共享同一token而非重新arm；匯出可有兩角色但統計只計一個唯一attempt。
9. **GOOD前後GPIO範圍**：第五stage改為首BAD；GOOD只有READY＋每次prepare檢查的既定保護，不可虛構額外GOOD EVENT讀回。後來BAD的GPIO異常不得回寫先前GOOD。若GOOD本框另確認GPIO故障，reason須正確綁該事件而非僅global標記。
10. **期限／未append最後一框**：prepare在窗內、read開始或結束跨600s／20s而RAW未append，不能造RAW選slot；保留RMT1已修discard／NOT_BOUND有界收尾。close在callback前／後、timeout前／後都能封存，late callback不改payload。
11. **profile隔離**：outer6只接受對應RMT schema2／新slot profile；outer5／RMTschema1不得以相同634word長度被默默重標。改profile／role／seq／缺slot／多slot／錯順序／混version／混session即使CRC重算仍須拒收。
12. **null RMT＋真LINK及bad/good proof**：完整RAW／sidecar／HXF／PCNT資料可匯出，RMT明示unavailable，parser不要求虛構event。sidecar FIRST／SECOND／reserve失敗仍走明示schema3降級，不冒稱schema6完整新診斷。
13. **actual完整exporter**：至少正常先bad後good／先good後bad／LINK同BAD、RMT unavailable、兩個首事件RMT失敗、無bad／無good／無LINK到期。單獨RMT block mock通不代替完整外層與原PCNT proof表交叉。
14. **不變項**：原25pulse＋postcheck雜湊、RAW接受／ERR與cal邏輯、proof valid／bad黏性／ready守衛、heap上限與driver config有獨立來源檢查；不是只測文字宣告。新schema實際target連結及每次燒錄後獨立QC仍必要。

## GPIO、ISR及有限symbol的承接條件

RMT1 preflight已修的未綁定pending永不收斂、EVENT GPIO故障漏進當框reason兩條不得回歸。callback仍只保存metadata，safe_copy只在正ptr／token／一次最終callback及count範圍下讀driver buffer；timeout不重用ctx／buffer，關閉metadata不含會被晚callback改的欄位。

新BAD／GOOD政策改變保留對象，不能解除RX hardware error不可見、首／末censored、0marker、雙RX起點未知、driver-computed頻率未外校等限制。正HIGH encoded前綴與完整性分開；RMT不修原RAW、不宣稱HX端電壓／供電或CAL366通過。

## 後續審查與實板要求

Gibbs固定公開來源與主機證據後，Ampere再核上述反例及實際完整exporter／parser。主代理以原FQBN正式編譯／正確MAC四區校驗，成功後立即新RMT2 QC另立報告。第一輪實板仍必保留自然boot、GPIO／heap／channel／resolution、原PCNT8次分類、首BAD／GOOD同seq與所有NA原因、全部CRC及外層END、FIFO／同步DUMP干擾；不得因看見RMT2捕獲bad proof就將原bad改成good或恢復CAL。

## 2026-10-28 具體來源核對與獨立重跑

本節是preflash證據，不是已部署／實板QC。公開`docs/calibration/20261028-rmt2-source.json`所列12檔與外部候選逐檔SHA-256全符合；主代理另報正式編譯1,026,873程式／124,080靜態bytes，但本次沒有讀建置產物。主程式SHA-256為`7CB1846E85E6C22002C9AED507E156BEE1CAF8A098266C101497A771715B4485`；RMT adapter為`B2C5817B75D9558790CC965D83AFC3459D243B0F1DB8E098C95A3E9B336185D9`，純狀態header為`F9275D20E6F1ACC943A4BF87CC3721D45DF421389BFBCFD7F2E4F306677918DE`。來源若變動，本節只涵蓋上述固定版本。

`hxrFreshProofRole`只讀實際proof表最後一筆，要求同seq後依原valid0／1分類，不看global ready；單一主迴圈每完成attempt唯一配號，所以不存在把舊proof按新seq重選的正常路徑。`hxrProofIdentity`核原bits／accepted／postcheck／seq及兩個32-byte packed unit，未使用policy source bit代替proof表。`recordHxRmt`將身分不符標IDENTITY terminal／當框reason，而非悄悄提升有效。`hxrSelect`對FIRST_LINK／BAD／GOOD各以第一seq鎖定；有state但無job時保存NOT_ARMED／RAW／sidecar，後續健康job不替換。相同LINK＋BAD讀框同token／canonical共用，不新增接收。

新PCNT bookkeeping只有`proof_active`真才標`HXAP_PROOF_SOURCE`；滿8筆且ready0後為NO_PROOF／沒有source bit，不再謊稱新proof。原`hxpAddProof`分類、bad黏性及ready規則未修改。原25pulse區塊LF正規化489字元SHA-256仍`B00C0E5BBE74C37CA753FF248A1D03DBA79A0C01D6E21F547A0864412EB55BB5`；含原postcheck行與LF的554字元仍`08DFA86D6B694564C4227F9CCE3ABEB51607CC9763BCA848B3BCAF9052BA8E32`。兩種範圍不可混稱。

獨立執行結果：

| 範圍 | 結果／證據 | 限制 |
|---|---|---|
| actual PCNT／RMT native | 33＋36＝69 cases通，`Temp/irt-rmt2-ampere-regression-vkFAMd/run.cmd`，MSVC /W4 /wd4505，exit0 | mock／主機，不是pad／ISR真實驗證；/wd4505承接既有未使用static警告抑制 |
| Ampere新增臨時strict反例 | 28 cases通，`Temp/irt-rmt2-ampere-native-0oejqf/review.cpp`：bad→good仍ready0、identity五欄、packed16word各變一位、過期source bit無proof、三角色缺RX鎖first不重選 | 不修改Gibbs測試或來源；非全API故障窮舉 |
| 完整實際`.ino` exporter | `Temp/irt-rmt2-export-Nezhtw/run.cmd`獨立編譯／執行exit0；9 paths：normal、sidecar失敗2／3／4、RMT null、全good／全bad／LINK同BAD／bad→good，全部完整parser通 | 主機合成RAW；此批每個完整bundle都有LINK，無LINK到期完整outer6仍待補 |
| actual adapter＋實際PCNT／RMT exporter | `Temp/irt-rmt2-adapter-export-s0SQZY/run.cmd`，10 modes通；逐個以原proof表`rm.match`嚴格核對，timeout／null／unbound／late／wrong-buffer及兩角色首次NA等，二次dump byte-exact | 包含無LINK的adapter snapshot，但不代替完整outer6無LINK到期路徑 |
| 新schema負測 | 23個重算row／block CRC後的schema／role label／epoch／seq／bits／packed16word／缺BAD或GOOD mutation全部拒收 | CRC本身不保證語意；負測不代表所有任意破壞已窮舉 |
| 舊RMT1真事件回放 | 原183516完整bundle仍解析為outer5／RMT schema1，角色未被改為BAD／GOOD、proof_table_verified=false | 支援歷史回放不等於接受新profile冒名 |

完整exporter抽取函式SHA-256為`731FD92965D803B51343941164234E465DD6BB9AB6B03763DBD4A599089D3AFD`；adapter／PCNT exporter抽取函式為`3F2F70A5DD7222F95AB71F3B1F2B08F74ACC07DFA47E562D240553000354AF03`。上述`Temp/`均指`C:/Users/tseng/AppData/Local/Temp/`下的公開來源臨時驗證目錄。

### PF2-10：截止proof與未append RAW的可重現parser阻擋

原RAW用`record.start_us`檢共同600s／20s期限；read_end跨界但start在界內仍會保存，不應誤稱一律遺失。實際PCNT prepare在界內開始，其API括弧／中斷可使原diag_begin到界外；finish仍照原分類新增actual proof，RAW Observe則正確omit。此時RMT只能discard／NOT_BOUND，不能發明RAW或選phantom event。

Ampere以現有actual PCNT／RMT adapter與原Observe重現：`Temp/irt-rmt2-ampere-deadline-osoEsI/review.cpp`與`run.cmd`（exit0），prepare由599999994us開始，RAW start600000003／end600000204us，RAW count0，proof_count1／firstproof_valid1／seq1，RMT BAD0／GOOD0。這不是改壞proof、不是RMT分配失敗，也不能把missing當0symbol。目前新parser`match`無條件要求first valid1 proof有retained GOOD，會拒此合法的保留缺口。已通知Gibbs與root；需明確標示該proof不在可保留RAW範圍，保守接受完整但有此缺口的bundle，並加actual exporter／parser反例。不得為了通過而改原RAW期限、proof分類／ready或造RMT事件。

此外，啟動本輪獨立檢查時舊`05-hx-rmt-integration.test.cjs`尚硬要求schema5，舊`hx-rmt-diagnostic.test.cjs`尚以舊fixture對照新抽取hash，兩者確實失敗。Gibbs正做工具／fixture／測試收尾，未HOST READY；本報告不把這些中間態失敗改成已通。最終須在固定工具／fixtures後重新執行相關套件，並核PF2-10及完整無LINK到期後，才能解除preflight阻擋。這不影響已保存RMT1實板功能未過及CAL366未放行結論。

### 最終定向重測與PF2-10解除

最終manifest SHA-256 `170596D375CB5C23861C2A6699824B39459D87E3EB71BA0D136B3CD5F7A00AB9`；Ampere從repo公開副本再次核12個韌體hash及24個工具／測試檔，全部符合manifest，C來源與原正式compile一致，沒有因parser修正改動C。Gibbs固定證據為Node193／native148全通；Ampere在固定候選獨立針對性重跑`hx-rmt2-proof-pair.test.cjs`17組、`05-hx-rmt-integration.test.cjs`6組、歷史`hx-rmt-diagnostic.test.cjs`16組，全部exit0，包含先前兩項舊測試中間態失敗已修正。沒有為此重跑已通的全部主套件。

PF2-10現在只在configured RMT／NOT_BOUND terminal、actual first-role proof是最後proof、seq同時等於RMT attempts／lastseq／frozen LINK attempts、omitted恰1、所有RAW均無該seq、該role metadata與payload空、真600s WINDOW或有實際trigger的真20s POST（600s優先）、已知START不晚於freeze、STOP在freeze後1秒內等條件全真時，標`RAW_UNRETAINED_AT_DEADLINE`及waveform_available=false。此例外不改原valid／ready，也不把missing標成未觸發。兩型actual adapter＋Observe＋完整exporter的GOOD／BAD截止例，以及逐條破壞條件、無關NOT_BOUND豁免早期rollover、假數值deadline／ledger等負例，皆已在上述17組定向重跑驗證。Kierkegaard亦向root獨立回報數值反例已核閉。

新完整`hx-rmt2-roles-native-synthetic.log`第五bundle覆蓋400個accepted後600s Tick／Seal、無LINK、BAD3／GOOD1保留，完整outer6解析通；不是只靠adapter孤立block。最終主機矩陣為10個完整路徑、10個adapter模式與2個截止完整export，承接原NA／callback／CRC／schema限制。剩餘實板資源、真正symbol及新增時序干擾，必須在正確目標寫入後另建RMT2 QC；本preflight不能解除RMT1失敗、HX根因或CAL366限制。
