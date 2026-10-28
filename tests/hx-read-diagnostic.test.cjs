// 離線協定/截尾/CRC檢查，沒有實板操作。
const assert=require('node:assert/strict');
const fs=require('node:fs');
const {WORDS,COLUMNS,crc32,parseHxDiagnostics,summarize}=require('../tools/hx-read-diagnostic.cjs');
let n=0;function test(name,fn){fn();console.log(`PASS ${++n}: ${name}`);}
function row(seq,extra={}){return {seq,start_us:seq*1000,end_us:seq*1000+100,attempt_gap_us:1000,last_accept_ms_before:0,bits24:0xfd8f00,raw:-160000,rx_before:seq-1,rx_after:seq,err_before:0,err_after:0,flags:0,cal_count_after:0,phase_after:0,cal_mode_after:0,accepted:1,dout_after25:1,pulses_issued:25,context_flags:3,...extra};}
function encode(r){return ['HXRD',...WORDS.map(k=>k==='bits24'?'0x'+r[k].toString(16).padStart(6,'0'):String(r[k])),'0x'+crc32(WORDS.map(k=>r[k]>>>0)).toString(16).padStart(8,'0')].join(',');}
function dump(rows){const i=rows.findIndex(r=>r.flags),trigger=i>=0?rows[i]:null;return [`# BEGIN_HX_DIAG schema=1 version=test session=1234ABCD rows=${rows.length} crc=CRC32_ISO_HDLC_19_LE_U32 computed_on_export=1 output_blocks_polling=1`,`# HX_DIAG schema=1 version=test session=1234ABCD mode=FROZEN freeze_reason=${trigger?2:3} records=${rows.length} capacity=256 record_bytes=56 ring_bytes=14336`,`# HX_DIAG_TIME clock=boot_micros_u32 start_us=0 window_us=600000000 trigger_us=${trigger?.end_us??0} freeze_us=600000000 trigger_seq=${trigger?.seq??0} trigger_flags=${trigger?.flags??0} pre=${trigger?i:0} post=${trigger?rows.length-i-1:0} post_target=128 post_complete=0`,`# HX_DIAG_COUNTS attempts=${rows.length} pre_overwritten=0 omitted_after_freeze=0 overflow=0 seq_wrapped=0`,COLUMNS.join(','),...rows.map(encode),`# END_HX_DIAG rows=${rows.length} retained=1 read_only=1`].join('\n');}
test('完整正常/拒絕框，拒絕字不冒充有效RAW',()=>{const d=parseHxDiagnostics(dump([row(1),row(2,{accepted:0,dout_after25:0,bits24:0xffff64,raw:-156,rx_after:1,err_after:1,flags:1})]));assert.equal(summarize(d[0]).rejected,1);});
test('CRC損壞拒絕',()=>assert.throws(()=>parseHxDiagnostics(dump([row(1)]).replace(/0x[0-9a-f]{8}\n/,'0x00000000\n')),/CRC/));
test('尾行截斷/缺END/缺時間欄不當完整資料',()=>{assert.throws(()=>parseHxDiagnostics(dump([row(1)]).replace(/# END_HX_DIAG.*/,'')),/Incomplete/);assert.throws(()=>parseHxDiagnostics(dump([row(1)]).replace(/# HX_DIAG_TIME[^\n]+\n/,'')),/metadata|clock/);});
test('重複attempt序號拒絕',()=>assert.throws(()=>parseHxDiagnostics(dump([row(1),row(1)])),/sequence/));
test('行數不吻合拒絕',()=>assert.throws(()=>parseHxDiagnostics(dump([row(1)]).replace('rows=1','rows=2')),/Row count/));
test('即使CRC有效仍檢查bits解碼',()=>assert.throws(()=>parseHxDiagnostics(dump([row(1,{raw:-156})])),/decode/));
test('接受/拒絕接收與錯誤計數契約檢查',()=>{assert.throws(()=>parseHxDiagnostics(dump([row(1,{rx_after:2})])),/Counter/);assert.throws(()=>parseHxDiagnostics(dump([row(1),row(2,{rx_before:7,rx_after:8})])),/continuity/);});
test('時間與序號跨wrap保留無號差',()=>{const a=row(0xffffffff,{start_us:0xfffffff0,end_us:0x20,rx_before:0xfffffffe,rx_after:0xffffffff}),b=row(0,{start_us:0x100,end_us:0x150,attempt_gap_us:272,rx_before:0xffffffff,rx_after:0});const c=parseHxDiagnostics(dump([a,b]))[0];assert.equal(c.rows[0].read_cpu_us,48);});
test('混入其他epoch拒絕',()=>assert.throws(()=>parseHxDiagnostics(dump([row(1)]).replace('session=1234ABCD mode','session=00000000 mode')),/epoch/));
test('凍結但零筆有效，未見資料不假填RAW0',()=>{const c=parseHxDiagnostics(dump([]))[0];assert.equal(c.rows.length,0);assert.equal(summarize(c).accepted,0);});
test('不交錯的多個完整DUMP分開回傳',()=>assert.equal(parseHxDiagnostics(dump([row(1)])+'\n'+dump([row(1)])).length,2));
test('協定欄名與韌體來源一致',()=>{const source=fs.readFileSync(require('node:path').join(__dirname,'../05-test/05-test.ino'),'utf8');assert.ok(source.includes(COLUMNS.join(',')));});
console.log(`${n} offline HX protocol checks passed; not device validation.`);
