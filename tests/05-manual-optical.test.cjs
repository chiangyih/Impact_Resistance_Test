'use strict';

// Execute PUBLIC firmware functions with synthetic edges and in-memory mocks.
// No serial port, radio, ADC, private credentials, or hardware timing is used.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const firmware = fs.readFileSync(path.join(__dirname, '..', '05-test', '05-test.ino'), 'utf8');
const header = fs.readFileSync(path.join(__dirname, '..', '05-test', 'manual_optical.h'), 'utf8');

function body(source, name) {
  const declaration = new RegExp(`^(?:bool|void|const char \\*)\\s*(?:ARDUINO_ISR_ATTR )?${name}\\(`, 'm').exec(source);
  assert.ok(declaration, `Missing public source function ${name}`);
  const open = source.indexOf('{', declaration.index);
  const end = source.indexOf('\n}', open);
  assert.ok(open >= 0 && end > open, `Incomplete public source function ${name}`);
  return source.slice(open + 1, end).replace(/\/\/[^\n]*/g, '');
}

function translate(text) {
  return text
    .replace(/static const char \*names\[\] = \{([^}]+)\};/g, 'const names = [$1];')
    .replace(/static_cast<uint32_t>\(([^()]*)\)/g, '(($1) >>> 0)')
    .replace(/static_cast<[^>]+>\(([^()]*)\)/g, '($1)')
    // Preserve the C++ uint32_t difference used for micros() rollover.
    .replace(/const uint32_t ([^;]+);/g, (_match, declarations) => 'const ' + declarations.split(/,\s*(?=\w+\s*=)/).map(declaration => declaration.replace(/^(\w+)\s*=\s*(.+)$/, '$1 = (($2) >>> 0)')).join(', ') + ';')
    .replace(/\bconst (?:bool|int|uint8_t|uint16_t|double) (\w+)/g, 'const $1')
    .replace(/\b(?:bool|uint8_t|uint16_t|uint32_t|double) (\w+)/g, 'let $1')
    .replace(/char \*(\w+)/g, 'let $1')
    .replace(/,\s*\*(\w+)\s*=/g, ', $1 =')
    .replace(/let number = 0;/g, 'number = 0;')
    .replace(/&(?:save|gate_mux)\b/g, match => match.slice(1))
    .replace(/\bnullptr\b/g, 'null')
    .replace(/\b(0x[\dA-Fa-f]+|\d+)(?:UL|U|L)\b/g, '$1');
}

function compile(e, source, name, parameters = '') {
  vm.runInContext(`function ${name}(${parameters}) {${translate(body(source, name))}}`, e);
}

function environment() {
  const e = {
    IDLE: 0, ARMED: 1, WAIT_DN: 2, CAPTURE: 3, DONE: 4, FAULT: 5,
    NONE: 0, LOWER_FIRST: 1, TOO_FAST: 2, UPPER_TIMEOUT: 3, LOWER_TIMEOUT: 4, USER_STOP: 5, BUFFER_FULL: 6,
    phase: 0, reason: 0, cal_mode: 0, trial_id: 17,
    armed_us: 11, upper_us: 12, lower_us: 13, ignored_edges: 14,
    acc_count: 7, hx_count: 8, trial_sat: 1,
    tare_valid: true, calibration_valid: true, tare_raw: -12345, counts_per_g: 12.5,
    upper_active: 1, lower_active: 1, UP_PIN: 34, DN_PIN: 35, levels: {34: 0, 35: 0},
    distance_mm: 220, now: 0, pauseSucceeds: true,
    optical_state: 0, optical_reason: 0, optical_upper_us: 0, optical_lower_us: 0, optical_ignored: 0,
    optical_upper_seen: false, optical_lower_seen: false, optical_id: 0, optical_distance_mm: 0,
    optical_upper_active: -1, optical_lower_active: -1, optical_reported: false,
    gate_mux: {}, number: 0, calls: [], output: [],
    isfinite: Number.isFinite,
  };
  for (const enumName of ['OpticalState', 'OpticalReason']) {
    const match = new RegExp(`enum ${enumName}\\s*:[^{]+\\{([^}]+)\\}`).exec(header);
    assert.ok(match, `Missing ${enumName}`);
    match[1].split(',').forEach((name, index) => { e[name.trim()] = index; });
  }
  for (const match of header.matchAll(/\b(OPTICAL_(?:MIN|MAX)_US)\s*=\s*(\d+)/g)) e[match[1]] = Number(match[2]);
  e.FIRMWARE_VERSION = /const char FIRMWARE_VERSION\[\] = "([^"]+)"/.exec(firmware)[1];
  const record = (name, ...args) => e.calls.push({name, args});
  const forbidden = name => () => { throw new Error(`Pure optical mode must not call ${name}`); };
  e.micros = () => e.now >>> 0;
  e.millis = () => Math.floor(e.now / 1000) >>> 0;
  e.digitalRead = pin => e.levels[pin];
  e.strcmp = (a, b) => a === b ? 0 : 1;
  e.pauseWifiForMeasurement = () => { record('pauseWifi'); return e.pauseSucceeds; };
  e.portENTER_CRITICAL = () => record('lock');
  e.portEXIT_CRITICAL = () => record('unlock');
  e.portENTER_CRITICAL_ISR = () => record('isrLock');
  e.portEXIT_CRITICAL_ISR = () => record('isrUnlock');
  e.Serial = {
    print: (...args) => e.output.push(['print', ...args]),
    println: (...args) => e.output.push(['println', ...args]),
    printf: (...args) => e.output.push(['printf', ...args]),
    flush: () => record('serialFlush'),
  };
  for (const name of ['pollHx', 'pollAdxl', 'resetFifo', 'readAdxl', 'writeAdxl', 'hxStatus', 'collectCalibration', 'serviceWifi']) {
    e[name] = forbidden(name);
  }
  e.setWifiEnabled = forbidden('setWifiEnabled');
  e.drawOpticalScreen = state => record('drawOpticalScreen', state, e.optical_state);
  for (const name of ['serviceTrial', 'serviceSerial', 'updateOled', 'printStatus', 'printWifiStatus', 'printFifoDiagnostics', 'dumpTrial', 'abortTrial', 'armTrial']) {
    e[name] = () => record(name);
  }
  e.delay = value => record('delay', value);
  let tokens = [];
  e.strtok_r = text => {
    if (text !== null) tokens = String(text).trim().split(/\s+/).filter(Boolean);
    return tokens.length ? tokens.shift() : null;
  };
  e.parseNumber = text => {
    if (typeof text !== 'string' || !/^[+-]?(?:\d+(?:\.\d*)?|\.\d+)$/.test(text)) return false;
    e.number = Number(text);
    return Number.isFinite(e.number);
  };
  vm.createContext(e);
  for (const [name, params] of [
    ['opticalMode', ''], ['opticalBeginState', ''], ['opticalUpperEdge', 'now'], ['opticalLowerEdge', 'now'],
    ['opticalTick', 'now'], ['opticalStopState', ''], ['opticalClearState', ''],
  ]) compile(e, header, name, params);
  for (const [name, params] of [
    ['isRecording', ''], ['opticalStateName', 'value'], ['opticalReasonName', 'value'],
    ['printOpticalStatus', ''], ['serviceOptical', ''], ['startOptical', ''],
    ['handleOpticalCommand', 'cmd, one, two, extra'], ['processCommand', 'line'],
    ['upperInterrupt', ''], ['lowerInterrupt', ''], ['loop', ''],
  ]) compile(e, firmware, name, params);
  return e;
}

function outputText(e) {
  return e.output.map(([kind, value, ...args]) => {
    if (kind === 'printf') {
      let index = 0;
      return value.replace(/%(?:\.(\d+))?(?:l)?([sduf])/g, (_match, precision, type) => {
        const arg = args[index++];
        return type === 'f' ? Number(arg).toFixed(Number(precision ?? 6)) : String(arg);
      });
    }
    return (typeof value === 'number' && args.length ? value.toFixed(args[0]) : String(value)) + (kind === 'println' ? '\n' : '');
  }).join('');
}

function speedFields(e) {
  const line = outputText(e).split('\n').filter(line => line.startsWith('# SPEED ')).at(-1);
  assert.ok(line, 'Expected an actual SPEED status response');
  return Object.fromEntries(Array.from(line.matchAll(/(\w+)=([^\s;]+)/g), match => [match[1], match[2]]));
}

function opticalSnapshot(e) {
  return Object.fromEntries([
    'optical_state', 'optical_reason', 'optical_upper_us', 'optical_lower_us', 'optical_ignored',
    'optical_upper_seen', 'optical_lower_seen', 'optical_id', 'optical_distance_mm',
    'optical_upper_active', 'optical_lower_active', 'optical_reported',
  ].map(name => [name, e[name]]));
}

function started() {
  const e = environment();
  e.processCommand('SPEED START');
  assert.equal(e.optical_state, e.OPT_WAIT_UP);
  e.calls.length = e.output.length = 0;
  return e;
}

function done() {
  const e = started();
  e.opticalUpperEdge(0);
  e.opticalLowerEdge(100000);
  assert.equal(e.optical_state, e.OPT_DONE);
  return e;
}

function normalSnapshot(e) {
  return Object.fromEntries([
    'phase', 'reason', 'cal_mode', 'trial_id', 'armed_us', 'upper_us', 'lower_us', 'ignored_edges',
    'acc_count', 'hx_count', 'trial_sat', 'tare_valid', 'calibration_valid', 'tare_raw', 'counts_per_g',
    'upper_active', 'lower_active', 'distance_mm',
  ].map(name => [name, e[name]]));
}

test('public optical mode defines all five independent states', () => {
  for (const name of ['OPT_OFF', 'OPT_WAIT_UP', 'OPT_WAIT_DN', 'OPT_DONE', 'OPT_FAULT']) {
    assert.match(header, new RegExp(`\\b${name}\\b`));
  }
});

test('actual manual loop bypasses ADC, FIFO, normal trial, OLED and WiFi services', () => {
  const e = environment();
  e.opticalMode = () => true;
  e.serviceOptical = () => e.calls.push({name: 'serviceOptical', args: []});
  compile(e, firmware, 'loop');
  const before = normalSnapshot(e);
  e.loop();
  assert.ok(e.calls.some(call => call.name === 'serviceOptical'));
  assert.ok(e.calls.some(call => call.name === 'serviceSerial'));
  assert.ok(e.calls.some(call => call.name === 'delay'));
  assert.ok(e.calls.every(call => ['serviceOptical', 'serviceSerial', 'delay'].includes(call.name)));
  assert.deepEqual(normalSnapshot(e), before);
});

test('CLEAR received in the manual branch does not fall through to ADC polling in the same iteration', () => {
  const e = environment();
  let manual = true;
  e.opticalMode = () => manual;
  e.serviceOptical = () => {};
  e.serviceSerial = () => { manual = false; };
  compile(e, firmware, 'loop');
  e.loop();
  assert.equal(manual, false);
  assert.deepEqual(e.calls.map(call => call.name), ['delay']);
});

test('SPEED START inside normal serviceSerial short-circuits the rest of that iteration', () => {
  const e = environment();
  e.pollHx = () => e.calls.push({name: 'pollHx'});
  e.pollAdxl = () => e.calls.push({name: 'pollAdxl'});
  e.serviceSerial = () => {
    e.calls.push({name: 'serviceSerial'});
    e.processCommand('SPEED START');
  };
  const before = normalSnapshot(e);
  e.loop();
  assert.equal(e.optical_state, e.OPT_WAIT_UP);
  const command = e.calls.findIndex(call => call.name === 'serviceSerial');
  assert.ok(command >= 0);
  assert.ok(e.calls.slice(command + 1).every(call => !['pollHx', 'pollAdxl', 'serviceTrial', 'updateOled', 'serviceWifi'].includes(call.name)));
  assert.equal(e.calls.filter(call => call.name === 'drawOpticalScreen').length, 1);
  assert.deepEqual(normalSnapshot(e), before);
});

test('OFF status reports current distance/polarity and missing times without touching ADC', () => {
  const e = environment();
  e.distance_mm = 321;
  e.upper_active = 0;
  e.processCommand('SPEED STATUS');
  const fields = speedFields(e);
  assert.equal(fields.state, 'OFF');
  assert.equal(Number(fields.DIST_mm), 321);
  assert.equal(fields.POL, '0/1');
  for (const field of ['upper_boot_us', 'lower_boot_us', 'dt_us', 'speed_mps']) assert.equal(fields[field], 'NA');
});

test('SPEED START stops radio and draws READY before enabling edges, independent of ADC/calibration', () => {
  const e = environment();
  e.adxl_ready = e.acc_seen = e.hx_seen = e.tare_valid = e.calibration_valid = false;
  const before = normalSnapshot(e);
  e.processCommand('SPEED START');
  assert.equal(e.opticalMode(), true);
  assert.equal(e.optical_state, e.OPT_WAIT_UP);
  assert.equal(e.optical_id, 1);
  assert.equal(e.optical_distance_mm, 220);
  assert.equal(e.optical_upper_active, 1);
  assert.equal(e.optical_lower_active, 1);
  const pause = e.calls.findIndex(call => call.name === 'pauseWifi');
  const draw = e.calls.findIndex(call => call.name === 'drawOpticalScreen');
  const lock = e.calls.findIndex(call => call.name === 'lock');
  assert.ok(pause >= 0 && draw > pause && lock > draw);
  assert.deepEqual(e.calls[draw].args, [e.OPT_WAIT_UP, e.OPT_OFF]);
  assert.deepEqual(normalSnapshot(e), before);
});

test('SPEED START rejects retained/active normal trial and calibration before any radio call', () => {
  for (const phase of [1, 2, 3, 4, 5]) {
    const e = environment();
    e.phase = phase;
    const before = normalSnapshot(e);
    e.processCommand('SPEED START');
    assert.equal(e.opticalMode(), false);
    assert.deepEqual(normalSnapshot(e), before);
    assert.equal(e.calls.length, 0);
  }
  for (const mode of [1, 2]) {
    const e = environment();
    e.cal_mode = mode;
    e.processCommand('SPEED START');
    assert.equal(e.opticalMode(), false);
    assert.equal(e.cal_mode, mode);
    assert.equal(e.calls.length, 0);
  }
});

test('invalid polarity, nonpositive/nonfinite distance and initially triggered sensors reject START', () => {
  for (const changes of [
    {upper_active: -1}, {lower_active: -1}, {upper_active: 2}, {lower_active: 2},
    {distance_mm: 0}, {distance_mm: -1}, {distance_mm: NaN}, {distance_mm: Infinity},
    {levels: {34: 1, 35: 0}}, {levels: {34: 0, 35: 1}},
  ]) {
    const e = environment();
    Object.assign(e, changes);
    e.processCommand('SPEED START');
    assert.equal(e.opticalMode(), false);
    assert.equal(e.optical_id, 0);
    assert.equal(e.calls.length, 0);
  }
});

test('failed radio stop rejects START without beginning a round or changing normal data', () => {
  const e = environment();
  e.pauseSucceeds = false;
  const before = normalSnapshot(e);
  e.processCommand('SPEED START');
  assert.equal(e.opticalMode(), false);
  assert.equal(e.optical_id, 0);
  assert.deepEqual(e.calls.map(call => call.name), ['pauseWifi']);
  assert.deepEqual(normalSnapshot(e), before);
});

test('marker entering during READY preparation rejects START without accepting the early edge', () => {
  const e = environment();
  e.drawOpticalScreen = () => { e.levels[34] = 1; };
  e.processCommand('SPEED START');
  assert.equal(e.opticalMode(), false);
  assert.equal(e.optical_id, 0);
  assert.equal(e.optical_upper_seen, false);
  assert.ok(e.calls.some(call => call.name === 'updateOled'));
});

test('WAIT_UP has no start deadline, including uint32 wrap, and does not refresh OLED', () => {
  const e = started();
  for (const now of [30000001, 0xffffffff, 0, 0x80000000]) {
    e.now = now;
    e.serviceOptical();
    assert.equal(e.optical_state, e.OPT_WAIT_UP);
    assert.equal(e.optical_upper_seen, false);
  }
  assert.equal(e.calls.filter(call => call.name === 'drawOpticalScreen').length, 0);
  assert.equal(e.output.length, 0);
  e.opticalUpperEdge(0);
  assert.equal(e.optical_state, e.OPT_WAIT_DN);
  assert.equal(e.optical_upper_seen, true);
  assert.equal(e.optical_upper_us, 0);
});

for (const origin of [0, 0xffffff00]) {
  for (const [delta, result, reason] of [
    [0, 'OPT_FAULT', 'OPT_TOO_FAST'], [199, 'OPT_FAULT', 'OPT_TOO_FAST'],
    [200, 'OPT_DONE', 'OPT_NONE'], [29999999, 'OPT_DONE', 'OPT_NONE'],
    [30000000, 'OPT_DONE', 'OPT_NONE'], [30000001, 'OPT_FAULT', 'OPT_LOWER_TIMEOUT'],
  ]) {
    test(`actual edge timing: origin=${origin}, delta=${delta}us`, () => {
      const e = started();
      e.opticalUpperEdge(origin);
      const down = (origin + delta) >>> 0;
      e.opticalLowerEdge(down);
      assert.equal(e.optical_state, e[result]);
      assert.equal(e.optical_reason, e[reason]);
      assert.equal(e.optical_lower_us, down);
      assert.equal(e.optical_lower_seen, true);
    });
  }
}

test('Tick accepts exactly 30 seconds and times out only after the bound, also across wrap', () => {
  for (const origin of [0, 0xffffff00]) {
    const e = started();
    e.opticalUpperEdge(origin);
    e.opticalTick((origin + 30000000) >>> 0);
    assert.equal(e.optical_state, e.OPT_WAIT_DN);
    e.opticalTick((origin + 30000001) >>> 0);
    assert.equal(e.optical_state, e.OPT_FAULT);
    assert.equal(e.optical_reason, e.OPT_LOWER_TIMEOUT);
    assert.equal(e.optical_lower_seen, false);
  }
});

test('lower-first at time zero is a retained fault with missing upper time and no numeric speed', () => {
  const e = started();
  e.opticalLowerEdge(0);
  e.processCommand('SPEED DUMP');
  assert.equal(e.optical_reason, e.OPT_LOWER_FIRST);
  const fields = speedFields(e);
  assert.equal(fields.upper_boot_us, 'NA');
  assert.equal(fields.lower_boot_us, '0');
  assert.equal(fields.dt_us, 'NA');
  assert.equal(fields.speed_mps, 'NA');
});

test('repeat upper edges increment only the optical ignored count without moving the first timestamp', () => {
  const e = started();
  e.opticalUpperEdge(1000);
  e.opticalUpperEdge(2000);
  e.opticalUpperEdge(3000);
  assert.equal(e.optical_upper_us, 1000);
  assert.equal(e.optical_ignored, 2);
  assert.equal(e.ignored_edges, 14);
  e.opticalLowerEdge(4000);
  assert.equal(e.optical_state, e.OPT_DONE);
});

test('DONE timestamps and reason survive later edges, timeouts, STOP and rejected START', () => {
  const e = done();
  e.serviceOptical();
  const before = opticalSnapshot(e);
  const normal = normalSnapshot(e);
  e.opticalUpperEdge(500000);
  e.opticalLowerEdge(600000);
  e.opticalTick(0xffffffff);
  e.processCommand('STOP');
  e.processCommand('SPEED START');
  assert.deepEqual(opticalSnapshot(e), before);
  assert.deepEqual(normalSnapshot(e), normal);
});

test('fault evidence is not overwritten by later edges or STOP', () => {
  const e = started();
  e.opticalLowerEdge(1000);
  e.serviceOptical();
  const before = opticalSnapshot(e);
  e.opticalUpperEdge(2000);
  e.opticalLowerEdge(3000);
  e.processCommand('STOP');
  assert.deepEqual(opticalSnapshot(e), before);
});

for (const waitingForLower of [false, true]) {
  test(`STOP cancels ${waitingForLower ? 'WAIT_DN' : 'WAIT_UP'} without cancelling normal trial/calibration`, () => {
    const e = started();
    if (waitingForLower) e.opticalUpperEdge(0);
    const before = normalSnapshot(e);
    e.processCommand('STOP');
    assert.equal(e.optical_state, e.OPT_FAULT);
    assert.equal(e.optical_reason, e.OPT_USER_STOP);
    assert.deepEqual(normalSnapshot(e), before);
    assert.equal(e.calls.some(call => call.name === 'abortTrial'), false);
    assert.equal(e.calls.filter(call => call.name === 'drawOpticalScreen').length, 1);
  });
}

test('completion reports/draws once; waiting and later completed-loop iterations do not refresh', () => {
  const e = started();
  e.loop();
  assert.equal(e.calls.some(call => call.name === 'drawOpticalScreen'), false);
  e.opticalUpperEdge(0);
  e.now = 50000;
  e.loop();
  assert.equal(e.output.length, 0);
  e.opticalLowerEdge(100000);
  e.now = 100000;
  e.loop();
  const output = outputText(e);
  assert.equal(e.calls.filter(call => call.name === 'drawOpticalScreen').length, 1);
  e.loop();
  assert.equal(outputText(e), output);
  assert.equal(e.calls.filter(call => call.name === 'drawOpticalScreen').length, 1);
});

test('valid DUMP keeps zero boot timestamp, calculates two-point mean, and retains result', () => {
  const e = done();
  const before = opticalSnapshot(e);
  const normal = normalSnapshot(e);
  e.processCommand('SPEED DUMP');
  const fields = speedFields(e);
  assert.equal(fields.state, 'DONE');
  assert.equal(fields.upper_boot_us, '0');
  assert.equal(fields.lower_boot_us, '100000');
  assert.equal(fields.dt_us, '100000');
  assert.equal(Number(fields.speed_mps), 2.2);
  assert.equal(fields.optical_only, '1');
  assert.equal(Number(fields.A), normal.acc_count);
  assert.equal(Number(fields.H), normal.hx_count);
  assert.match(outputText(e), /# BEGIN_SPEED .*clock=boot_micros.*no_ADC_records/);
  assert.match(outputText(e), /# END_SPEED/);
  assert.deepEqual(opticalSnapshot(e), before);
  assert.deepEqual(normalSnapshot(e), normal);
});

test('DUMP calculates uint32 elapsed time across wrap instead of a negative interval', () => {
  const e = started();
  e.opticalUpperEdge(0xffffff00);
  e.opticalLowerEdge(0x300);
  e.processCommand('SPEED DUMP');
  assert.equal(speedFields(e).dt_us, '1024');
  assert.equal(Number(speedFields(e).speed_mps), 214.84375);
});

test('active/completed optical status uses frozen settings rather than current DIST/POL', () => {
  const e = started();
  e.distance_mm = 999;
  e.upper_active = e.lower_active = 0;
  e.processCommand('SPEED STATUS');
  assert.equal(Number(speedFields(e).DIST_mm), 220);
  assert.equal(speedFields(e).POL, '1/1');
  e.opticalUpperEdge(0);
  e.opticalLowerEdge(100000);
  e.processCommand('SPEED DUMP');
  assert.equal(Number(speedFields(e).speed_mps), 2.2);
});

for (const terminal of ['DONE', 'FAULT']) {
  test(`SPEED CLEAR clears only ${terminal} optical result and preserves normal buffers/calibration`, () => {
    const e = terminal === 'DONE' ? done() : started();
    if (terminal === 'FAULT') e.opticalLowerEdge(0);
    const before = normalSnapshot(e);
    const id = e.optical_id;
    e.processCommand('SPEED CLEAR');
    assert.equal(e.opticalMode(), false);
    assert.equal(e.optical_id, id);
    assert.equal(e.optical_upper_seen, false);
    assert.equal(e.optical_lower_seen, false);
    assert.deepEqual(normalSnapshot(e), before);
    e.processCommand('SPEED START');
    assert.equal(e.optical_id, id + 1);
  });
}

test('DUMP/CLEAR before completion cannot manufacture or discard a pending result', () => {
  for (const state of ['OPT_OFF', 'OPT_WAIT_UP', 'OPT_WAIT_DN']) {
    const e = state === 'OPT_OFF' ? environment() : started();
    if (state === 'OPT_WAIT_DN') e.opticalUpperEdge(0);
    const before = opticalSnapshot(e);
    e.processCommand('SPEED DUMP');
    e.processCommand('SPEED CLEAR');
    assert.deepEqual(opticalSnapshot(e), before);
    assert.doesNotMatch(outputText(e), /# BEGIN_SPEED|# SPEED_CLEARED/);
  }
});

test('timing WAIT_DN suppresses all query/setting output except STOP', () => {
  const e = started();
  e.opticalUpperEdge(0);
  const before = opticalSnapshot(e);
  for (const command of ['SPEED STATUS', 'SPEED DUMP', 'SPEED CLEAR', 'HELP', 'VERSION', 'STATUS', 'ADXL_DIAG', 'WIFI ON', 'ARM', 'CAL 200']) e.processCommand(command);
  assert.equal(e.output.length, 0);
  assert.deepEqual(opticalSnapshot(e), before);
  assert.equal(e.calls.length, 0);
});

test('manual waiting/results block normal ARM/calibration/settings/clear/radio commands', () => {
  for (const state of ['OPT_WAIT_UP', 'OPT_DONE', 'OPT_FAULT']) {
    for (const command of ['ARM', 'TARE', 'CAL 200', 'UNCAL', 'DIST 123', 'POL 0 0', 'CLEAR', 'DUMP', 'ADXL_DIAG', 'WIFI ON', 'WIFI OFF']) {
      const e = started();
      if (state === 'OPT_DONE') { e.opticalUpperEdge(0); e.opticalLowerEdge(100000); }
      if (state === 'OPT_FAULT') e.opticalLowerEdge(0);
      const before = normalSnapshot(e);
      const optical = opticalSnapshot(e);
      e.processCommand(command);
      assert.deepEqual(normalSnapshot(e), before, `${state}: ${command}`);
      assert.deepEqual(opticalSnapshot(e), optical, `${state}: ${command}`);
      assert.equal(e.calls.length, 0, `${state}: ${command} must not reach normal handlers`);
    }
  }
});

test('normal recording guard precedes optical command dispatch and preserves original ARM isolation', () => {
  for (const phase of [1, 2, 3]) {
    const e = environment();
    e.phase = phase;
    const before = normalSnapshot(e);
    let reached = false;
    e.handleOpticalCommand = () => { reached = true; return true; };
    for (const command of ['SPEED START', 'SPEED STATUS', 'SPEED CLEAR', 'VERSION']) e.processCommand(command);
    assert.equal(reached, false);
    assert.equal(e.output.length, 0);
    assert.deepEqual(normalSnapshot(e), before);
  }
});

test('invalid SPEED grammar leaves both modes unchanged', () => {
  for (const command of ['SPEED', 'SPEED UNKNOWN', 'SPEED START extra', 'SPEED STATUS extra', 'SPEED CLEAR extra extra']) {
    const e = environment();
    const before = normalSnapshot(e);
    e.processCommand(command);
    assert.equal(e.opticalMode(), false);
    assert.deepEqual(normalSnapshot(e), before);
    assert.equal(e.calls.length, 0);
  }
});

test('actual ISR dispatch filters polarity, unlocks optical branches and preserves normal timestamps', () => {
  const e = started();
  const before = normalSnapshot(e);
  e.now = 0;
  e.upperInterrupt();
  assert.equal(e.optical_state, e.OPT_WAIT_UP);
  e.levels[34] = 1;
  e.upperInterrupt();
  assert.equal(e.optical_state, e.OPT_WAIT_DN);
  e.now = 500;
  e.upperInterrupt();
  assert.equal(e.optical_upper_us, 0);
  e.now = 100000;
  e.levels[35] = 1;
  e.lowerInterrupt();
  assert.equal(e.optical_state, e.OPT_DONE);
  assert.deepEqual(normalSnapshot(e), before);
  assert.equal(e.calls.filter(call => call.name === 'isrLock').length, 3);
  assert.equal(e.calls.filter(call => call.name === 'isrUnlock').length, 3);
});

test('VERSION uses the same public constant as startup and does not change any measurement state', () => {
  const e = environment();
  const before = normalSnapshot(e);
  const optical = opticalSnapshot(e);
  e.processCommand('VERSION');
  assert.ok(outputText(e).includes(e.FIRMWARE_VERSION));
  assert.match(body(firmware, 'setup'), /FIRMWARE_VERSION/);
  assert.deepEqual(normalSnapshot(e), before);
  assert.deepEqual(opticalSnapshot(e), optical);
  assert.equal(e.calls.length, 0);
});
