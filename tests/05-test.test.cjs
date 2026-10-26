// 第五版來源邏輯的主機模擬；不模擬實體波形、I²C 或 ESP32 中斷延遲。
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(path.join(__dirname, '..', '05-test', '05-test.ino'), 'utf8');
let passed = 0;
function test(name, run) { run(); console.log(`PASS ${++passed}: ${name}`); }

// 從實際韌體抽取指定函式，不另寫一份狀態機；僅轉譯下列簡單 C++ 型別與無號算術。
function body(name) {
  const declaration = new RegExp(`^(?:bool|void|int32_t|const char \\*)\\s*(?:ARDUINO_ISR_ATTR )?${name}\\(`, 'm').exec(source);
  assert.ok(declaration, `找不到 ${name}`);
  const start = declaration.index;
  const open = source.indexOf('{', start);
  const end = source.indexOf('\n}', open);
  assert.ok(end > open);
  return source.slice(open + 1, end)
    .replace(/\/\/[^\n]*/g, '')
    .replace(/static_cast<[^>]+>\(([^()]*)\)/g, '($1)')
    .replace(/const uint32_t (\w+) = ([^;]+);/g, 'const $1 = (($2) >>> 0);')
    .replace(/const double (\w+)/g, 'const $1')
    .replace(/\b(now - (?:armed_us|upper_us|lower_us))\b/g, '(($1) >>> 0)')
    .replace(/\b(\d+|0x[\dA-Fa-f]+)(?:UL|L)\b/g, '$1');
}
function environment() {
  const e = {
    IDLE: 0, ARMED: 1, WAIT_DN: 2, CAPTURE: 3, DONE: 4, FAULT: 5,
    NONE: 0, LOWER_FIRST: 1, TOO_FAST: 2, UPPER_TIMEOUT: 3, LOWER_TIMEOUT: 4, USER_STOP: 5, BUFFER_FULL: 6,
    phase: 0, reason: 0, armed_us: 1000, upper_us: 0, lower_us: 0, ignored_edges: 0,
    upper_active: 0, lower_active: 0, UP_PIN: 34, DN_PIN: 35, levels: {34: 1, 35: 1}, clock: 1000,
    ARM_TIMEOUT_US: 10000000, TRANSIT_TIMEOUT_US: 2000000, MIN_TRANSIT_US: 200, POST_CAPTURE_US: 2000000,
    gate_mux: {}, portENTER_CRITICAL_ISR() {}, portEXIT_CRITICAL_ISR() {}, portENTER_CRITICAL() {}, portEXIT_CRITICAL() {},
    hx_last_ms: 1, acc_last_ms: 1, HX_TIMEOUT_MS: 1000, trial_stale: false, trial_acc_stale: false,
    completion_reported: false, hx_errors: 0, trial_hx_errors: 0, acc_errors: 0, trial_acc_errors: 0,
    fifo_full: 0, trial_fifo_full: 0, trial_hx_end: 0, trial_acc_end: 0, trial_fifo_end: 0,
    trial_id: 1, acc_count: 0, hx_count: 0, trial_sat: 0,
    hx_frame_ok: true, hx_seen: true, hx_start_ms: 0, hx_raw: 123,
    cal_mode: 0, cal_count: 0, cal_min: 0, cal_max: 0, cal_sum: 0, CAL_SAMPLES: 32,
    CAL_MAX_SPAN: 2000, CAL_MIN_DELTA: 100, cal_mass_g: 100,
    tare_raw: 0, tare_valid: false, calibration_valid: false, counts_per_g: 0,
    fmin: Math.min, fmax: Math.max, fabs: Math.abs,
    Serial: {println() {}, printf() {}},
    opticalMode: () => false,
  };
  e.micros = () => e.clock >>> 0;
  e.millis = () => 10;
  e.digitalRead = pin => e.levels[pin];
  vm.createContext(e);
  for (const name of ['isRecording', 'decodeHx', 'phaseName', 'reasonName', 'upperInterrupt', 'lowerInterrupt', 'collectCalibration', 'serviceTrial', 'hxStatus']) {
    if (name === 'phaseName' || name === 'reasonName') { e[name] = () => 'MOCK'; continue; }
    const parameters = name === 'decodeHx' ? 'bits' : name === 'collectCalibration' ? 'raw' : '';
    vm.runInContext(`function ${name}(${parameters}) {${body(name).replace(/&gate_mux/g, 'gate_mux')}}`, e);
  }
  return e;
}

test('每個非空韌體行均含中文註解，無繼電器腳位初始化', () => {
  for (const [i, line] of source.split(/\r?\n/).entries()) if (line.trim()) assert.match(line, /\/\/.*[\u4e00-\u9fff]/, `第 ${i + 1} 行`);
  assert.doesNotMatch(source, /(?:pinMode|digitalWrite)\(\s*(?:26|27)\b/);
  assert.match(source, /HX_SCK = 25, HX_DATA = 33/);
});
test('補數零、正負邊界與二十四位遮罩', () => {
  const e = environment();
  for (const [input, expected] of [[0, 0], [0x7fffff, 8388607], [0x800000, -8388608], [0xffffff, -1], [0xfffd8940, -161472]]) assert.equal(e.decodeHx(input), expected);
});
test('使用者更正有效觸發間距為220毫米，極性仍待確認', () => {
  const distance = Number(/double distance_mm = ([\d.]+)/.exec(source)[1]);
  assert.equal(distance, 220);
  assert.equal(distance * 1000 / 100000, 2.2);
  assert.match(source, /volatile int upper_active = -1, lower_active = -1/);
});
test('未啟動及未設定極性不觸發', () => {
  const e = environment(); e.levels[34] = 0; e.upperInterrupt(); assert.equal(e.phase, e.IDLE);
  e.phase = e.ARMED; e.upper_active = -1; e.upperInterrupt(); assert.equal(e.phase, e.ARMED);
});
test('錯極性與退出緣被忽略', () => {
  const e = environment(); e.phase = e.ARMED; e.upperInterrupt(); e.lowerInterrupt(); assert.equal(e.phase, e.ARMED);
});
test('下閘先到明確中止且不計速度', () => {
  const e = environment(); e.phase = e.ARMED; e.levels[35] = 0; e.lowerInterrupt(); assert.equal(e.phase, e.FAULT); assert.equal(e.reason, e.LOWER_FIRST);
});
test('正確順序、第一次時間鎖定與兩秒後錄', () => {
  const e = environment(); e.phase = e.ARMED; e.clock = 2000; e.levels[34] = 0; e.upperInterrupt(); assert.equal(e.phase, e.WAIT_DN);
  e.clock = 2500; e.upperInterrupt(); assert.equal(e.upper_us, 2000); assert.equal(e.ignored_edges, 1);
  e.clock = 12000; e.levels[35] = 0; e.lowerInterrupt(); assert.equal(e.phase, e.CAPTURE); assert.equal(e.lower_us - e.upper_us, 10000);
  e.clock += 100; e.lowerInterrupt(); assert.equal(e.lower_us, 12000);
  e.clock = 2011999; e.serviceTrial(); assert.equal(e.phase, e.CAPTURE);
  e.clock = 2012000; e.serviceTrial(); assert.equal(e.phase, e.DONE); assert.equal(e.completion_reported, true);
});
test('過短間隔被拒絕，二百微秒邊界可接受', () => {
  for (const [dt, expected] of [[0, 5], [199, 5], [200, 3]]) {
    const e = environment(); e.phase = e.WAIT_DN; e.upper_us = 1000; e.clock = 1000 + dt; e.levels[35] = 0; e.lowerInterrupt(); assert.equal(e.phase, expected);
  }
});
test('上下閘逾時及晚到上閘不被接受', () => {
  const e = environment(); e.phase = e.ARMED; e.clock += 10000000; e.serviceTrial(); assert.equal(e.reason, e.UPPER_TIMEOUT);
  const f = environment(); f.phase = f.ARMED; f.clock += 10000000; f.levels[34] = 0; f.upperInterrupt(); assert.equal(f.phase, f.FAULT); assert.equal(f.reason, f.UPPER_TIMEOUT);
  const g = environment(); g.phase = g.WAIT_DN; g.upper_us = 1000; g.clock = 2001001; g.levels[35] = 0; g.lowerInterrupt(); assert.equal(g.reason, g.LOWER_TIMEOUT);
});
test('微秒計時跨三十二位溢位仍能正確通過', () => {
  const e = environment(); e.armed_us = 0xffffff00; e.clock = 0xffffff10; e.phase = e.ARMED; e.levels[34] = 0; e.upperInterrupt();
  e.clock = 0x310; e.levels[35] = 0; e.lowerInterrupt(); assert.equal(e.phase, e.CAPTURE); assert.equal((e.lower_us - e.upper_us) >>> 0, 1024);
  e.clock = (e.lower_us + 2000000) >>> 0; e.serviceTrial(); assert.equal(e.phase, e.DONE);
});
test('WAIT、TIMEOUT、LINK_ERR、STALE、正負 SAT 與 OK', () => {
  const e = environment(); e.hx_seen = false; assert.equal(e.hxStatus(), 'WAIT'); e.millis = () => 1000; assert.equal(e.hxStatus(), 'TIMEOUT');
  e.hx_frame_ok = false; assert.equal(e.hxStatus(), 'LINK_ERR'); e.hx_frame_ok = true; e.hx_seen = true; e.hx_last_ms = 0; assert.equal(e.hxStatus(), 'STALE');
  e.hx_last_ms = 1000; for (const raw of [-8388608, 8388607]) { e.hx_raw = raw; assert.equal(e.hxStatus(), 'SAT'); }
  e.hx_raw = -161472; assert.equal(e.hxStatus(), 'OK');
});
test('三十二筆新框去皮後仍未校正，舊比例失效', () => {
  const e = environment(); e.cal_mode = 1; e.calibration_valid = true; e.counts_per_g = 5;
  for (let i = 0; i < 31; i++) e.collectCalibration(-160000); assert.equal(e.tare_valid, false);
  e.collectCalibration(-160000); assert.equal(e.tare_raw, -160000); assert.equal(e.tare_valid, true); assert.equal(e.calibration_valid, false); assert.equal(e.counts_per_g, 0);
});
test('已知重量比例允許正向或反向訊號', () => {
  for (const sign of [-1, 1]) { const e = environment(); e.cal_mode = 2; e.tare_valid = true; e.tare_raw = -160000; e.cal_mass_g = 100;
    for (let i = 0; i < 32; i++) e.collectCalibration(-160000 + sign * 5000);
    assert.equal(e.calibration_valid, true); assert.equal(e.counts_per_g, sign * 50);
  }
});
test('飽和、不穩定或接近零差值不改寫舊校正', () => {
  for (const values of [[8388607], [-8388608], Array.from({length: 32}, (_, i) => i % 2 ? 10000 : 0), Array(32).fill(50)]) {
    const e = environment(); e.cal_mode = 2; e.tare_valid = true; e.calibration_valid = true; e.counts_per_g = 9;
    for (const raw of values) e.collectCalibration(raw);
    assert.equal(e.counts_per_g, 9); assert.equal(e.calibration_valid, true); assert.equal(e.cal_mode, 0);
  }
});
test('記錄期間 OLED 被禁止，輸出用 NA 保留未校正與未量距離', () => {
  assert.match(body('updateOled'), /!oled_address \|\| isRecording\(\)/);
  assert.match(body('dumpTrial'), /if \(calibration_valid && !h\.sat\)/);
  assert.match(body('dumpTrial'), /if \(distance_mm > 0\)/);
  assert.match(body('dumpTrial'), /# ACC_PEAK NA/);
  assert.match(source, /timestamps=host_readout_us_from_ARM/);
  assert.match(body('pollHx'), /i < 25/);
  assert.match(body('pollHx'), /portEXIT_CRITICAL\(&hx_mux\); delayMicroseconds\(2\)/);
  assert.ok(body('armTrial').indexOf('Serial.flush()') < body('armTrial').indexOf('!resetFifo()'));
  assert.ok(body('serviceTrial').indexOf('portENTER_CRITICAL') < body('serviceTrial').indexOf('micros()'));
});
test('NMK99 RGB32與HX SCK25分離，只開機要求全黑', () => {
  assert.match(source, /BOARD_RGB_PIN = 32/);
  assert.match(source, /static_assert\(BOARD_RGB_PIN != HX_SCK/);
  assert.match(body('setup'), /rgbLedWrite\(BOARD_RGB_PIN, 0, 0, 0\)/);
  assert.match(body('setup'), /digitalWrite\(BOARD_RGB_PIN, LOW\)/);
  assert.doesNotMatch(body('loop'), /rgbLedWrite|BOARD_RGB_PIN/);
  assert.match(body('dumpTrial'), /HX_SCK=%u HX_DATA=%u RGB_PIN=%u/);
});
test('OLED明確使用SH1106、4x6字型與正常方向，不使用串流自動換行', () => {
  assert.match(source, /U8G2_SH1106_128X64_NONAME_F_HW_I2C/);
  assert.match(body('initializeOled'), /setFont\(u8g2_font_4x6_tr\)/);
  assert.match(body('initializeOled'), /setDisplayRotation\(U8G2_R0\)/);
  assert.doesNotMatch(body('updateOled'), /oled\.(?:printf|print|setCursor)\(/);
  assert.doesNotMatch(body('armTrial'), /oled\.(?:printf|print|setCursor)\(/);
  for (let row = 0; row < 8; row++) assert.match(body('updateOled'), new RegExp(`drawOledLine\\(${row},`));
});
test('從韌體轉譯單行繪字，驗證長字串、四邊界線及越界行拒絕', () => {
  const drawBody = body('drawOledLine')
    .replace(/char clipped\[31\];/, "let clipped = Array(31).fill('\\0');")
    .replace(/sizeof\(clipped\)/g, 'clipped.length')
    .replace(/size_t length =/, 'let length =');
  for (const glyphWidth of [4, 6]) {
    const drawn = [];
    const textOf = a => { const stop = a.indexOf('\0'); return a.slice(0, stop < 0 ? a.length : stop).join(''); };
    const e = {
      strncpy: (out, text, limit) => { for (let i = 0; i < Math.min(text.length, limit); i++) out[i] = text[i]; },
      strlen: a => textOf(a).length,
      oled: {
        getStrWidth: a => textOf(a).length * glyphWidth,
        setClipWindow: (...coords) => assert.deepEqual(coords, [4, 4, 124, 60]),
        setMaxClipWindow() {},
        drawStr: (x, y, a) => drawn.push({x, y, text: textOf(a)}),
      },
    };
    vm.createContext(e); vm.runInContext(`function draw(row, text) {${drawBody}}`, e);
    for (let row = 0; row < 8; row++) e.draw(row, '1234567890'.repeat(20));
    e.draw(8, 'INVALID'); e.draw(255, 'INVALID'); e.draw(0, null);
    assert.equal(drawn.length, 8);
    for (const [row, d] of drawn.entries()) {
      assert.equal(d.x, 4); assert.equal(d.y, 9 + row * 7);
      assert.ok(d.text.length <= 30 && d.text.length * glyphWidth <= 120);
      assert.ok(d.x + d.text.length * glyphWidth <= 124);
      assert.ok(d.y - 5 >= 4 && d.y + 1 < 60);
    }
  }
});
console.log(`All ${passed} host simulation checks passed; not hardware timing validation.`);
