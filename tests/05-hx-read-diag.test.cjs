// 從公開C++狀態機轉譯的主機檢查，不代表GPIO/實板時序通過。
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const header = fs.readFileSync(path.join(__dirname, '../05-test/hx_read_diag.h'), 'utf8');
const source = fs.readFileSync(path.join(__dirname, '../05-test/05-test.ino'), 'utf8');
let passed = 0;
function test(name, run) { run(); console.log(`PASS ${++passed}: ${name}`); }
function body(text, name) {
  const start = text.search(new RegExp(`^(?:inline )?(?:void|bool|uint16_t|uint32_t) ${name}\\(`, 'm'));
  assert.ok(start >= 0, name);
  const open = text.indexOf('{', start), end = text.indexOf('\n}', open);
  return text.slice(open + 1, end).replace(/\/\/[^\n]*/g, '');
}
function environment() {
  const e = { HXD_CAPACITY:256,HXD_PRE:127,HXD_POST:128,HXD_WINDOW_US:600000000,HXD_POST_US:20000000,
    HXD_LIVE:0,HXD_POST_CAPTURE:1,HXD_FROZEN:2,HXD_LINK:1,HXD_SAT:2,HXD_JUMP:4,
    HXD_NOT_FROZEN:0,HXD_POST_FULL:1,HXD_POST_TIMEOUT:2,HXD_WINDOW_LIMIT:3,HXD_OVERFLOW:4 };
  vm.createContext(e);
  const signatures = {hxdiagDelta:'now, then',hxdiagBegin:'s, now',hxdiagFreeze:'s, now, reason',hxdiagTick:'s, now',hxdiagAppend:'s, record',hxdiagObserveShared:'s, record',hxdiagObserve:'s, record',hxdiagCanRetain:'s, start',hxdiagSeal:'s',hxdiagIndex:'s, ordinal',hxdiagCrcWord:'crc, value',hxdiagCrc:'r'};
  for (const [name,args] of Object.entries(signatures)) {
    let b=body(header,name).replace(/static_cast<uint32_t>\(r\.raw\)/g,'(r.raw >>> 0)')
      .replace(/const uint32_t words\[\] = \{([^}]+)\}/,'const words = [$1]')
      .replace(/\bconst (?:uint32_t|int32_t)\b/g,'const').replace(/\b(?:uint8_t|uint16_t|uint32_t|int32_t)\b/g,'let')
      .replace(/\b(\d+|0x[\dA-Fa-f]+)(?:UL|L)\b/g,'$1')
      .replace(/s\.records\[s\.next\] = record/g,'s.records[s.next] = {...record}')
      .replace(/\+\+s\.(attempts|omitted_after_freeze|pre_overwritten);/g,'s.$1=(s.$1+1)>>>0;')
      .replace(/(?<!>)>>(?!=|>)/g,'>>>').replace(/(?<!>)>>=/g,'>>>=');
    if(name==='hxdiagDelta') b=b.replace('return now - then;','return (now - then) >>> 0;');
    if(name==='hxdiagCrc') b=b.replace('return crc ^ 0xFFFFFFFF;','return (crc ^ 0xFFFFFFFF) >>> 0;');
    if(name==='hxdiagObserve'||name==='hxdiagObserveShared') b='record={...record};'+b;
    vm.runInContext(`function ${name}(${args}) {${b}}`,e);
  }
  return e;
}
function state() { return {records:Array(256),next:0,count:0,pre_count:0,post_count:0,attempts:0,omitted_after_freeze:0,pre_overwritten:0,trigger_mask:7,export_sealed:false,export_attempts:0,export_omitted:0,export_pre_overwritten:0,export_seq_wrapped:false,start_us:0,trigger_us:0,freeze_us:0,trigger_seq:0,trigger_flags:0,last_attempt_start_us:0,last_accepted_raw:0,mode:0,freeze_reason:0,started:false,attempt_seen:false,accepted_seen:false,overflow:false,seq_wrapped:false}; }
function record(i, extra={}) { return {seq:0,start_us:i*1000,end_us:i*1000+125,attempt_gap_us:0,last_accept_ms_before:0,bits24:0xfd8f00,raw:-160000,rx_before:i-1,rx_after:i,err_before:0,err_after:0,flags:0,cal_count_after:0,phase_after:0,cal_mode_after:0,accepted:1,dout_after25:1,pulses_issued:25,context_flags:1,...extra}; }
function rows(e,s) { return Array.from({length:s.count},(_,i)=>s.records[e.hxdiagIndex(s,i)]); }

test('固定容量、總期限及pulse train沒有逐bit診斷插碼',()=>{
  assert.match(header,/sizeof\(HxDiagRecord\) == 56/);
  const pulse=source.slice(source.indexOf('  delayMicroseconds(2);',source.indexOf('void pollHx()')),source.indexOf('  hx_frame_ok =',source.indexOf('void pollHx()')));
  assert.equal((pulse.match(/micros\(/g)||[]).length,0);
  assert.equal((pulse.match(/digitalRead\(HX_DATA\)/g)||[]).length,1);
  assert.match(pulse,/i < 25/); assert.match(pulse,/if \(i < 24\)/);
  assert.doesNotMatch(pulse,/Serial|hxdiag|malloc|new |crc/i);
});
test('一般前史只留127筆且明示已滾動覆寫，不濾RAW',()=>{
  const e=environment(),s=state();e.hxdiagBegin(s,0);
  for(let i=1;i<=400;i++) e.hxdiagObserve(s,record(i));
  assert.equal(s.count,127);assert.equal(s.pre_overwritten,273);assert.equal(s.mode,0);
  assert.equal(rows(e,s)[0].seq,274);assert.equal(rows(e,s).at(-1).seq,400);
});
test('127前史+第一拒絕框+128後史固定凍結，後續不覆寫',()=>{
  const e=environment(),s=state();e.hxdiagBegin(s,0);
  for(let i=1;i<=200;i++)e.hxdiagObserve(s,record(i));
  e.hxdiagObserve(s,record(201,{accepted:0,dout_after25:0,bits24:0xfffdae,raw:-594,rx_after:200,err_after:1}));
  for(let i=202;i<=329;i++)e.hxdiagObserve(s,record(i));
  assert.equal(s.mode,2);assert.equal(s.count,256);assert.equal(s.pre_count,127);assert.equal(s.post_count,128);
  assert.equal(s.freeze_reason,1);assert.equal(s.trigger_seq,201);assert.equal(s.trigger_flags,1);
  const before=JSON.stringify(rows(e,s));e.hxdiagObserve(s,record(330));
  assert.equal(s.omitted_after_freeze,1);assert.equal(JSON.stringify(rows(e,s)),before);
});
test('早期事件只列實際前史，新增事件不延後首事件期限',()=>{
  const e=environment(),s=state();e.hxdiagBegin(s,0);
  e.hxdiagObserve(s,record(1,{accepted:0,dout_after25:0}));const trigger=s.trigger_us;
  e.hxdiagObserve(s,record(2,{accepted:0,dout_after25:0}));
  assert.equal(s.pre_count,0);assert.equal(s.trigger_us,trigger);assert.equal(s.post_count,1);
  e.hxdiagTick(s,trigger+20000000);assert.equal(s.freeze_reason,2);assert.equal(s.post_count,1);
});
test('無attempt也到總期限，新begin不會清資料或延長',()=>{
  const e=environment(),s=state();e.hxdiagBegin(s,100);e.hxdiagBegin(s,200);
  assert.equal(s.start_us,100);e.hxdiagTick(s,600000099);assert.equal(s.mode,0);
  e.hxdiagTick(s,600000100);assert.equal(s.mode,2);assert.equal(s.freeze_reason,3);assert.equal(s.count,0);
});
test('接近總期限的事件後窗會被總期限截斷',()=>{
  const e=environment(),s=state();e.hxdiagBegin(s,0);
  e.hxdiagObserve(s,record(1,{start_us:599000000,end_us:599000125,accepted:0,dout_after25:0}));
  e.hxdiagTick(s,600000000);assert.equal(s.freeze_reason,3);assert.equal(s.post_count,0);
});
test('拒絕bits不當有效RAW比較基準；大跳仍存原值',()=>{
  const e=environment(),s=state();e.hxdiagBegin(s,0);e.hxdiagObserve(s,record(1));
  e.hxdiagObserve(s,record(2,{accepted:0,dout_after25:0,raw:-156,bits24:0xffff64}));
  e.hxdiagObserve(s,record(3));assert.equal(rows(e,s).at(-1).flags,0);
  e.hxdiagObserve(s,record(4,{raw:-594,bits24:0xfffdae}));
  assert.equal(rows(e,s).at(-1).raw,-594);assert.equal(rows(e,s).at(-1).flags,4);assert.equal(s.trigger_flags,1);
});
test('SAT及一千count只分類檢索，不改輸入紀錄',()=>{
  const e=environment(),s=state();e.hxdiagBegin(s,0);e.hxdiagObserve(s,record(1));
  const r=record(2,{raw:8388607,bits24:0x7fffff});e.hxdiagObserve(s,r);
  assert.equal(s.trigger_flags,6);assert.equal(rows(e,s).at(-1).accepted,1);assert.equal(r.flags,0);
});
test('時間跨uint32 wrap、attempt序號wrap與有效零正確分列',()=>{
  const e=environment(),s=state();e.hxdiagBegin(s,0xfff00000);s.attempts=0xffffffff;
  e.hxdiagObserve(s,record(1,{start_us:0xfff00100,end_us:0xfff00180,raw:0,bits24:0}));
  assert.equal(s.seq_wrapped,true);assert.equal(rows(e,s)[0].seq,0);assert.equal(s.accepted_seen,true);
  e.hxdiagTick(s,(0xfff00000+600000000)>>>0);assert.equal(s.freeze_reason,3);
});
test('意外容量不足有overflow且不覆寫既有證據',()=>{
  const e=environment(),s=state();e.hxdiagBegin(s,0);s.mode=1;s.count=256;s.trigger_us=100;
  e.hxdiagObserve(s,record(1));assert.equal(s.overflow,true);assert.equal(s.freeze_reason,4);assert.equal(s.count,256);
});
test('匯出CRC以獨立table參考比對canonical十九個word',()=>{
  const e=environment(),r=record(1,{raw:-156,bits24:0xffff64});
  const keys=['seq','start_us','end_us','attempt_gap_us','last_accept_ms_before','bits24','raw','rx_before','rx_after','err_before','err_after','flags','cal_count_after','phase_after','cal_mode_after','accepted','dout_after25','pulses_issued','context_flags'];
  const data=Buffer.alloc(76);keys.forEach((k,i)=>data.writeUInt32LE(r[k]>>>0,4*i));
  const table=Array.from({length:256},(_,n)=>{for(let j=0;j<8;j++)n=(n&1)?0xedb88320^(n>>>1):n>>>1;return n>>>0;});
  let crc=0xffffffff;for(const b of data)crc=table[(crc^b)&255]^(crc>>>8);crc=(crc^0xffffffff)>>>0;
  assert.equal(e.hxdiagCrc(r),crc);r.raw++;assert.notEqual(e.hxdiagCrc(r),crc);
});
test('唯讀介面不要求HX健康，不提供診斷clear/reset或自動CAL',()=>{
  const start=source.indexOf('if (!strcmp(cmd, "HX_DIAG"))'); const cmd=source.slice(start,source.indexOf('\n',start));
  assert.match(cmd,/cal_mode/);assert.doesNotMatch(cmd,/hxStatus|TARE|UNCAL|CLEAR|hxdiagBegin|hxdiagFreeze/);
  assert.match(source,/HX_DIAG_DUMP_REJECT not frozen/);assert.match(source,/output_blocks_polling=1/);
  assert.match(source,/crc=export_only/);assert.doesNotMatch(body(header,'hxdiagObserve'),/Serial|digitalWrite|digitalRead|crc/i);
});
console.log(`${passed} HX diagnostic host checks passed; no physical GPIO/timing validation.`);
