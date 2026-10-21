// 從第五版實際函式轉譯的 FIFO 診斷主機檢查；不模擬硬體 FIFO、I²C 或真實排程。
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(path.join(__dirname, '..', '05-test', '05-test.ino'), 'utf8');
let passed = 0;
function test(name, run) { run(); console.log(`PASS ${++passed}: ${name}`); }

// 只取來源中的函式本體；C++ 區域型別轉為 JS，uint32_t 差值明確保留無號溢位語意。
function body(name) {
  const declaration = new RegExp(`^(?:bool|void)\\s+${name}\\(`, 'm').exec(source);
  assert.ok(declaration, `找不到實際來源函式 ${name}`);
  const open = source.indexOf('{', declaration.index);
  const end = source.indexOf('\n}', open);
  assert.ok(open >= 0 && end > open, `${name} 函式範圍不完整`);
  return source.slice(open + 1, end)
    .replace(/\/\/[^\n]*/g, '')
    .replace(/static_cast<[^>]+>\(([^()]*)\)/g, '($1)')
    .replace(/const uint32_t (\w+) = ([^;]+);/g, 'const $1 = (($2) >>> 0);')
    .replace(/\buint32_t (\w+) = ([^;]+);/g, 'let $1 = (($2) >>> 0);')
    .replace(/\bconst (?:bool|uint8_t|uint16_t|Phase) (\w+)/g, 'const $1')
    .replace(/\b(?:bool|uint8_t|uint16_t|Phase) (\w+)/g, 'let $1')
    .replace(/\b(now_us - fifo_poll_us)\b/g, '(($1) >>> 0)')
    .replace(/\b(\d+|0x[\dA-Fa-f]+)(?:UL|L)\b/g, '$1');
}

function environment() {
  const e = {
    IDLE: 0, ARMED: 1, WAIT_DN: 2, CAPTURE: 3, DONE: 4, FAULT: 5,
    phase: 0,
    fifo_full: 0, fifo_depth_hits: 0, fifo_overrun_hits: 0,
    fifo_last_entries: 0, fifo_last_source: 0, fifo_peak_entries: 0,
    fifo_first_us: 0, fifo_first_entries: 0, fifo_first_source: 0, fifo_first_phase: 0,
    fifo_event_us: 0, fifo_event_entries: 0, fifo_event_source: 0, fifo_event_phase: 0,
    fifo_poll_seen: false, fifo_poll_us: 0, fifo_poll_gap_max_us: 0,
    fifo_observed_seen: false, fifo_poll_gap_seen: false,
    max: Math.max,
    Serial: new Proxy({}, {get() { throw new Error('純診斷函式不可輸出 Serial'); }}),
  };
  vm.createContext(e);
  for (const [name, parameters] of [['isRecording', ''], ['observeFifo', 'entries, source, now_us'], ['noteFifoPoll', 'now_us']]) {
    vm.runInContext(`function ${name}(${parameters}) {${body(name)}}`, e);
  }
  return e;
}

test('正常讀值更新最近快照與最高深度，不產生警示', () => {
  const e = environment();
  e.observeFifo(12, 0x80, 100);
  e.observeFifo(3, 0x00, 200);
  assert.equal(e.fifo_last_entries, 3);
  assert.equal(e.fifo_last_source, 0);
  assert.equal(e.fifo_peak_entries, 12);
  assert.equal(e.fifo_observed_seen, true);
  assert.equal(e.fifo_full, 0);
  assert.equal(e.fifo_depth_hits, 0);
  assert.equal(e.fifo_overrun_hits, 0);
});

test('深度滿載與 D0 Overrun 分開計數', () => {
  const depth = environment();
  depth.observeFifo(32, 0, 100);
  assert.equal(depth.fifo_full, 1);
  assert.equal(depth.fifo_depth_hits, 1);
  assert.equal(depth.fifo_overrun_hits, 0);
  const overrun = environment();
  overrun.observeFifo(1, 1, 200);
  assert.equal(overrun.fifo_full, 1);
  assert.equal(overrun.fifo_depth_hits, 0);
  assert.equal(overrun.fifo_overrun_hits, 1);
});

test('深度與 Overrun 同時成立時總警示只加一次', () => {
  const e = environment();
  e.observeFifo(32, 0x83, 100);
  assert.equal(e.fifo_full, 1);
  assert.equal(e.fifo_depth_hits, 1);
  assert.equal(e.fifo_overrun_hits, 1);
  assert.equal(e.fifo_first_source, 0x83);
  assert.equal(e.fifo_event_source, 0x83);
});

test('Watermark D1 與 DATA_READY D7 本身不增加警示', () => {
  const e = environment();
  for (const source of [0x02, 0x80, 0x82, 0xFE]) e.observeFifo(31, source, 100);
  assert.equal(e.fifo_full, 0);
  assert.equal(e.fifo_depth_hits, 0);
  assert.equal(e.fifo_overrun_hits, 0);
  assert.equal(e.fifo_last_source, 0xFE);
  assert.equal(e.fifo_peak_entries, 31);
});

test('深度判斷保留原本大於等於三十二的門檻', () => {
  const e = environment();
  e.observeFifo(31, 0, 10);
  assert.equal(e.fifo_full, 0);
  e.observeFifo(32, 0, 20);
  e.observeFifo(33, 0, 30);
  assert.equal(e.fifo_full, 2);
  assert.equal(e.fifo_depth_hits, 2);
  assert.equal(e.fifo_overrun_hits, 0);
  assert.equal(e.fifo_peak_entries, 33);
});

test('首次事件時間為零也能保留，後續事件不覆寫首次', () => {
  const e = environment();
  e.phase = e.IDLE;
  e.observeFifo(2, 0x81, 0);
  e.phase = e.ARMED;
  e.observeFifo(32, 0x02, 500);
  assert.equal(e.fifo_first_us, 0);
  assert.equal(e.fifo_first_entries, 2);
  assert.equal(e.fifo_first_source, 0x81);
  assert.equal(e.fifo_first_phase, e.IDLE);
  assert.equal(e.fifo_event_us, 500);
  assert.equal(e.fifo_event_entries, 32);
  assert.equal(e.fifo_event_source, 0x02);
  assert.equal(e.fifo_event_phase, e.ARMED);
});

test('正常快照不改寫首次或最近事件，活動階段亦不重設統計', () => {
  const e = environment();
  e.phase = e.WAIT_DN;
  e.observeFifo(4, 1, 1000);
  e.phase = e.CAPTURE;
  e.observeFifo(32, 0x80, 2000);
  e.phase = e.DONE;
  e.observeFifo(0, 0x02, 3000);
  assert.equal(e.fifo_full, 2);
  assert.equal(e.fifo_depth_hits, 1);
  assert.equal(e.fifo_overrun_hits, 1);
  assert.equal(e.fifo_first_us, 1000);
  assert.equal(e.fifo_first_entries, 4);
  assert.equal(e.fifo_first_source, 1);
  assert.equal(e.fifo_first_phase, e.WAIT_DN);
  assert.equal(e.fifo_event_us, 2000);
  assert.equal(e.fifo_event_entries, 32);
  assert.equal(e.fifo_event_source, 0x80);
  assert.equal(e.fifo_event_phase, e.CAPTURE);
  assert.equal(e.fifo_last_entries, 0);
  assert.equal(e.fifo_last_source, 0x02);
  assert.equal(e.phase, e.DONE);
});

test('已有的累計與首次紀錄不被清零或回溯改寫', () => {
  const e = environment();
  Object.assign(e, {fifo_full: 8, fifo_depth_hits: 5, fifo_overrun_hits: 4, fifo_first_us: 1,
    fifo_first_entries: 7, fifo_first_source: 1, fifo_first_phase: e.IDLE, fifo_peak_entries: 32});
  e.phase = e.ARMED;
  e.observeFifo(1, 1, 500);
  e.observeFifo(0, 0, 600);
  assert.equal(e.fifo_full, 9);
  assert.equal(e.fifo_depth_hits, 5);
  assert.equal(e.fifo_overrun_hits, 5);
  assert.equal(e.fifo_first_us, 1);
  assert.equal(e.fifo_first_entries, 7);
  assert.equal(e.fifo_first_source, 1);
  assert.equal(e.fifo_first_phase, e.IDLE);
  assert.equal(e.fifo_peak_entries, 32);
  assert.equal(e.fifo_event_us, 500);
});

test('第一次輪詢不把開機至今時間誤當輪詢間隔', () => {
  for (const now of [0, 1000000, 0xFFFFFFF0]) {
    const e = environment();
    e.noteFifoPoll(now);
    assert.equal(e.fifo_poll_seen, true);
    assert.equal(e.fifo_poll_us, now);
    assert.equal(e.fifo_poll_gap_max_us, 0);
    assert.equal(e.fifo_poll_gap_seen, false);
  }
});

test('從零開始輪詢仍計算後續間隔，最高值不因較小間隔減少', () => {
  const e = environment();
  for (const now of [0, 10, 40, 45]) e.noteFifoPoll(now);
  assert.equal(e.fifo_poll_us, 45);
  assert.equal(e.fifo_poll_gap_max_us, 30);
  assert.equal(e.fifo_poll_gap_seen, true);
  assert.equal(e.fifo_full, 0);
});

test('輪詢計時跨三十二位微秒溢位仍使用正確無號差值', () => {
  const e = environment();
  e.noteFifoPoll(0xFFFFFFF0);
  e.noteFifoPoll(0x10);
  assert.equal(e.fifo_poll_gap_max_us, 32);
  assert.equal(e.fifo_poll_us, 0x10);
  e.noteFifoPoll(0x1F);
  assert.equal(e.fifo_poll_gap_max_us, 32);
  e.noteFifoPoll(0x80);
  assert.equal(e.fifo_poll_gap_max_us, 97);
});

test('純診斷函式不操作通訊、重設 FIFO 或讀取額外硬體時鐘', () => {
  for (const name of ['observeFifo', 'noteFifoPoll']) {
    assert.doesNotMatch(body(name), /\b(?:Serial|Wire|resetFifo|readAdxl|writeAdxl|micros|millis)\b/);
  }
});

// 直接執行來源的輸出函式；只攔截 Serial 呼叫，不另寫輸出條件或 FIFO 狀態機。
function diagnosticsEnvironment() {
  const e = environment();
  const output = [];
  Object.assign(e, {
    acc_rx: 0, fifo_diag_errors: 0, fifo_snapshot_us: 0, fifo_pre_valid: false,
    fifo_post_us: 0, fifo_post_entries: 0, fifo_post_source: 0, fifo_post_valid: false,
    fifo_init_pre_entries: 0, fifo_init_pre_source: 0, fifo_init_post_entries: 0, fifo_init_post_source: 0,
    fifo_init_pre_valid: false, fifo_init_post_valid: false, adxl_init_attempts: 0,
    oled_send_count: 0, oled_last_us: 0, oled_max_us: 0,
    Serial: {
      print(value) { output.push({kind: 'print', text: String(value)}); },
      println(value) { output.push({kind: 'println', text: String(value)}); },
      printf(format, ...args) { output.push({kind: 'printf', text: format, args}); },
    },
  });
  vm.runInContext(`function printFifoInit() {${body('printFifoInit')}}`, e);
  vm.runInContext(`function printFifoDiagnostics() {${body('printFifoDiagnostics')}}`, e);
  return {e, output};
}

test('沒有事件、有效快照或 OLED 傳送時明確輸出 NONE／NA，不洩漏失效快取', () => {
  const {e, output} = diagnosticsEnvironment();
  Object.assign(e, {
    fifo_snapshot_us: 123, fifo_last_entries: 31, fifo_last_source: 0x81,
    fifo_post_us: 234, fifo_post_entries: 25, fifo_post_source: 0x82,
    fifo_first_us: 345, fifo_first_entries: 32, fifo_first_source: 1,
    oled_last_us: 456, oled_max_us: 567,
  });
  e.printFifoDiagnostics();
  const lines = output.filter(call => call.kind === 'println').map(call => call.text);
  assert.ok(lines.includes('# FIFO_PRE NA'));
  assert.ok(lines.includes('# FIFO_POST NA'));
  assert.ok(lines.includes('# FIFO_DEPTH_MAX NA'));
  assert.ok(lines.includes('# FIFO_POLL_GAP_MAX NA'));
  assert.ok(lines.includes('# FIFO_EVENT NONE'));
  assert.ok(lines.includes('# OLED_TIMING SENDS=0 LAST_US=NA MAX_US=NA'));
  assert.equal(output.filter(call => call.kind === 'printf' && /^(?:# FIFO_(?:DEPTH_MAX N=|POLL_GAP_MAX US=|PRE US=|POST US=|EVENT FIRST_US=)|# OLED_TIMING)/.test(call.text)).length, 0);
  assert.ok(output.some(call => call.text === 'PRE_N=NA PRE_SRC=NA '));
  assert.ok(output.some(call => call.text === 'POST_N=NA POST_SRC=NA'));
  assert.ok(output.some(call => call.text.startsWith('# FIFO_INIT attempt=')));
  assert.match(output[0].text, /# FIFO_DIAG scope=since_boot/);
});

test('有效診斷快取如實輸出零時間首次事件與獨立快照，不讀硬體或改統計', () => {
  const {e, output} = diagnosticsEnvironment();
  e.observeFifo(2, 0x81, 0);
  e.phase = e.ARMED;
  e.observeFifo(32, 0x02, 500);
  e.noteFifoPoll(0);
  e.noteFifoPoll(100);
  e.phase = e.DONE;
  Object.assign(e, {fifo_pre_valid: true, fifo_post_valid: true, fifo_post_us: 900,
    fifo_post_entries: 1, fifo_post_source: 0x80, oled_send_count: 3, oled_last_us: 80000, oled_max_us: 90000});
  e.printFifoDiagnostics();
  const event = output.find(call => call.text.startsWith('# FIFO_EVENT FIRST_US='));
  assert.ok(event);
  assert.deepEqual(event.args, [0, 2, 0x81, e.IDLE, 500, 32, 0x02, e.ARMED]);
  assert.deepEqual(output.find(call => call.text.startsWith('# FIFO_PRE US=')).args, [500, 32, 0x02]);
  assert.deepEqual(output.find(call => call.text.startsWith('# FIFO_POST US=')).args, [900, 1, 0x80]);
  assert.deepEqual(output.find(call => call.text.startsWith('# FIFO_DEPTH_MAX N=')).args, [32]);
  assert.deepEqual(output.find(call => call.text.startsWith('# FIFO_POLL_GAP_MAX US=')).args, [100]);
  assert.deepEqual(output.find(call => call.text.startsWith('# OLED_TIMING SENDS=')).args, [3, 80000, 90000]);
  assert.equal(e.fifo_full, 2);
  assert.equal(e.fifo_depth_hits, 1);
  assert.equal(e.fifo_overrun_hits, 1);
  assert.doesNotMatch(body('printFifoDiagnostics'), /\b(?:Wire|readAdxl|writeAdxl|resetFifo|micros|millis)\b/);
  assert.match(body('printFifoDiagnostics'), /printFifoInit\(\)/);
});

test('所有記錄階段禁止診斷輸出，避免 Serial 阻塞活動視窗', () => {
  for (const phase of [1, 2, 3]) {
    const {e, output} = diagnosticsEnvironment();
    e.phase = phase;
    Object.assign(e, {fifo_full: 1, fifo_pre_valid: true, fifo_post_valid: true, oled_send_count: 1});
    e.printFifoDiagnostics();
    assert.equal(output.length, 0);
  }
});

test('ADXL_DIAG 命令位於記錄防護之後，僅呼叫快取輸出', () => {
  const command = body('processCommand');
  const guard = /if\s*\(isRecording\(\)\)\s*return\s*;/.exec(command);
  const diagnostic = /if\s*\(!strcmp\(cmd,\s*"ADXL_DIAG"\)\s*&&\s*!one\)\s*\{\s*printFifoDiagnostics\(\);\s*return;\s*\}/.exec(command);
  assert.ok(guard && diagnostic);
  assert.ok(guard.index < diagnostic.index);
});

test('新增讀後快照只在非記錄時讀取，選用錯誤與原加速度錯誤分開', () => {
  const polling = body('pollAdxl');
  const post = /if\s*\(!isRecording\(\)\s*&&\s*\(entries\s*\|\|\s*\(source\s*&\s*1\)\)\)\s*\{([\s\S]*?)\n\s*\}/.exec(polling);
  assert.ok(post, '找不到非記錄時才執行的讀後快照區塊');
  assert.match(post[1], /readAdxl\(0x39,\s*&fifo_post_entries,\s*1\)/);
  assert.match(post[1], /readAdxl\(0x30,\s*&fifo_post_source,\s*1\)/);
  assert.match(post[1], /if\s*\(!fifo_post_valid\)\s*\+\+fifo_diag_errors/);
  assert.doesNotMatch(post[1], /\b(?:acc_errors|resetFifo|observeFifo)\b/);
  const optionalInitErrors = body('initializeAdxl').split('\n').filter(line => /!fifo_init_(?:pre|post)_valid/.test(line));
  assert.equal(optionalInitErrors.length, 2);
  for (const line of optionalInitErrors) {
    assert.match(line, /\+\+fifo_diag_errors/);
    assert.doesNotMatch(line, /\bacc_errors\b/);
  }
});

test('只有首次輪詢時最大間隔為 NA；真正零深度與零間隔可印零', () => {
  const {e, output} = diagnosticsEnvironment();
  e.noteFifoPoll(123);
  e.printFifoDiagnostics();
  assert.equal(e.fifo_poll_gap_seen, false);
  assert.ok(output.some(call => call.text === '# FIFO_DEPTH_MAX NA'));
  assert.ok(output.some(call => call.text === '# FIFO_POLL_GAP_MAX NA'));
  e.observeFifo(0, 0, 123);
  e.noteFifoPoll(123);
  output.length = 0;
  e.printFifoDiagnostics();
  assert.equal(e.fifo_observed_seen, true);
  assert.equal(e.fifo_poll_gap_seen, true);
  assert.deepEqual(output.find(call => call.text.startsWith('# FIFO_DEPTH_MAX N=')).args, [0]);
  assert.deepEqual(output.find(call => call.text.startsWith('# FIFO_POLL_GAP_MAX US=')).args, [0]);
});

test('初始化前後快取各自依有效旗標輸出，無效為 NA，重印不讀硬體', () => {
  for (const [preValid, postValid] of [[false, false], [true, false], [false, true], [true, true]]) {
    const {e, output} = diagnosticsEnvironment();
    Object.assign(e, {adxl_init_attempts: 2, fifo_init_pre_valid: preValid, fifo_init_post_valid: postValid,
      fifo_init_pre_entries: 31, fifo_init_pre_source: 0x83, fifo_init_post_entries: 0, fifo_init_post_source: 0x80});
    e.printFifoInit();
    assert.deepEqual(output.find(call => call.text.startsWith('# FIFO_INIT attempt=')).args, [2, preValid]);
    if (preValid) assert.deepEqual(output.find(call => call.text.startsWith('PRE_N=')).args, [31, 0x83]);
    else assert.ok(output.some(call => call.kind === 'print' && call.text === 'PRE_N=NA PRE_SRC=NA '));
    assert.deepEqual(output.find(call => call.text.startsWith('POST_VALID=')).args, [postValid]);
    if (postValid) assert.deepEqual(output.find(call => call.text.startsWith('POST_N=')).args, [0, 0x80]);
    else assert.ok(output.some(call => call.kind === 'print' && call.text === 'POST_N=NA POST_SRC=NA'));
    assert.ok(output.some(call => call.text.includes('snapshots_not_simultaneous')));
    assert.equal(e.fifo_init_pre_valid, preValid);
    assert.equal(e.fifo_init_post_valid, postValid);
  }
  const {e, output} = diagnosticsEnvironment();
  e.phase = e.CAPTURE;
  e.printFifoInit();
  assert.equal(output.length, 0);
  assert.doesNotMatch(body('printFifoInit'), /\b(?:Wire|readAdxl|writeAdxl|resetFifo|micros|millis)\b/);
});

console.log(`All ${passed} FIFO diagnostic host checks passed; not hardware timing or data-loss validation.`);
