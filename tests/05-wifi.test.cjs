'use strict';

// Execute functions extracted from PUBLIC source only. No private header is opened
// or preprocessed. Dummy credentials and WiFi mocks never access a radio or port.
// Passing checks do not validate API latency, power, waveforms, or sensor accuracy.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const header = fs.readFileSync(path.join(__dirname, '..', '05-test', 'network_wifi.h'), 'utf8');
const firmware = fs.readFileSync(path.join(__dirname, '..', '05-test', '05-test.ino'), 'utf8');
const DUMMY_SSID = 'SYNTHETIC-TEST-NETWORK';
const DUMMY_PASSWORD = 'synthetic-not-a-real-password';

function body(source, name) {
  const declaration = new RegExp(`^(?:bool|void|const char \\*)\\s*${name}\\(`, 'm').exec(source);
  assert.ok(declaration, `Missing public source function ${name}`);
  const open = source.indexOf('{', declaration.index);
  const end = source.indexOf('\n}', open);
  assert.ok(open >= 0 && end > open, `Incomplete source function ${name}`);
  return source.slice(open + 1, end).replace(/\/\/[^\n]*/g, '');
}

function translate(text) {
  return text
    .replace(/static const char \*names\[\] = \{([^}]+)\};/, 'const names = [$1];')
    .replace(/static_cast<uint32_t>\(([^()]*)\)/g, '(($1) >>> 0)')
    .replace(/static_cast<[^>]+>\(([^()]*)\)/g, '($1)')
    .replace(/const uint32_t (\w+) = ([^;]+);/g, 'const $1 = (($2) >>> 0);')
    .replace(/\bconst (?:bool|uint8_t|uint16_t|wl_status_t|IPAddress) (\w+)/g, 'const $1')
    .replace(/\b(?:bool|uint8_t|uint16_t|WifiPhase) (\w+)/g, 'let $1')
    .replace(/char \*(\w+)/g, 'let $1')
    .replace(/,\s*\*(\w+)\s*=/g, ', $1 =')
    // Bridge parseNumber's C++ output-reference through the VM's global number.
    .replace(/double number = 0;/g, 'number = 0;')
    .replace(/&(?:save|gate_mux)\b/g, match => match.slice(1))
    .replace(/\bnullptr\b/g, 'null')
    .replace(/\b(\d+|0x[\dA-Fa-f]+)(?:UL|L)\b/g, '$1');
}

function environment({ enabled = true, ssid = DUMMY_SSID, password = DUMMY_PASSWORD } = {}) {
  const e = {
    IRT_WIFI_ENABLED: enabled ? 1 : 0, IRT_WIFI_SSID: ssid, IRT_WIFI_PASSWORD: password,
    WIFI_STA: 1, WIFI_OFF: 0, WL_CONNECTED: 3, WL_DISCONNECTED: 6,
    wifi_phase: 0, wifi_user_enabled: false, wifi_radio_started: false,
    wifi_poll_seen: false, wifi_retry_seen: false, wifi_poll_ms: 0,
    wifi_attempt_ms: 0, wifi_retry_ms: 0, wifi_ip: [0, 0, 0, 0], wifi_ip_valid: false,
    IDLE: 0, ARMED: 1, WAIT_DN: 2, CAPTURE: 3, DONE: 4, FAULT: 5,
    NONE: 0, USER_STOP: 5, phase: 0, reason: 0, cal_mode: 0, cal_count: 9,
    cal_sum: 99, cal_mass_g: 0, cal_start_ms: 1000,
    tare_raw: -10, counts_per_g: 2, tare_valid: true, calibration_valid: true,
    upper_active: 0, lower_active: 0, UP_PIN: 34, DN_PIN: 35,
    levels: {34: 1, 35: 1}, adxl_ready: true, acc_seen: true,
    acc_last_ms: 995, hxStatusValue: 'OK', oled_address: 0,
    acc_count: 8, hx_count: 6, trial_sat: 1,
    trial_stale: true, trial_acc_stale: true, completion_reported: true,
    trial_hx_errors: 0, trial_acc_errors: 0, trial_fifo_full: 0,
    hx_errors: 2, acc_errors: 3, fifo_full: 4, trial_id: 10,
    upper_us: 3, lower_us: 4, armed_us: 5, ignored_edges: 6,
    last_retry_ms: 1000, last_display_ms: 1000, last_live_ms: 1000,
    gate_mux: {}, number: 0,
    now: 1000, calls: [], output: [], linkStatus: 6, localIp: [0, 0, 0, 0],
    staSucceeds: true, offSucceeds: true, modeDelayMs: 0, fifoSucceeds: true,
    forbidNetwork: false,
    opticalMode: () => false, handleOpticalCommand: () => false,
  };
  const enumMatch = /enum WifiPhase\s*:[^{]+\{([^}]+)\}/.exec(header);
  assert.ok(enumMatch, 'Missing public WifiPhase enum');
  enumMatch[1].split(',').forEach((name, index) => { e[name.trim()] = index; });
  for (const match of header.matchAll(/\b(WIFI_(?:POLL|TIMEOUT|RETRY)_MS)\s*=\s*(\d+)/g)) e[match[1]] = Number(match[2]);
  for (const name of ['WIFI_POLL_MS', 'WIFI_TIMEOUT_MS', 'WIFI_RETRY_MS']) assert.ok(e[name] > 0, `Missing ${name}`);

  const record = (name, args = []) => e.calls.push({name, args, radio: e.wifi_radio_started, phase: e.phase, cal: e.cal_mode, now: e.now});
  const network = (name, args = []) => {
    assert.equal(e.forbidNetwork, false, `${name} called while network APIs were forbidden`);
    record(name, args);
  };
  e.WiFi = {
    persistent(value) { network('persistent', [value]); },
    setAutoReconnect(value) { network('autoReconnect', [value]); return true; },
    mode(value) {
      network('mode', [value]);
      e.now = (e.now + e.modeDelayMs) >>> 0;
      return value === e.WIFI_OFF ? e.offSucceeds : e.staSucceeds;
    },
    begin(...args) { network('begin', args); return e.linkStatus; },
    status() { network('status'); return e.linkStatus; },
    localIP() { network('localIP'); return e.localIp.slice(); },
  };
  e.Serial = {
    printf(...args) { e.output.push(['printf', ...args]); },
    print(...args) { e.output.push(['print', ...args]); },
    println(...args) { e.output.push(['println', ...args]); },
    flush() { record('serialFlush'); },
  };
  e.millis = () => e.now >>> 0;
  e.micros = () => { record('armedTimestamp'); return (e.now * 1000) >>> 0; };
  e.strlen = text => text.length;
  e.strcmp = (left, right) => left === right ? 0 : 1;
  e.digitalRead = pin => e.levels[pin];
  e.hxStatus = () => e.hxStatusValue;
  e.resetFifo = () => { record('resetFifo'); return e.fifoSucceeds; };
  e.portENTER_CRITICAL = () => record('criticalEnter');
  e.portEXIT_CRITICAL = () => record('criticalExit');
  e.drawOledLine = () => record('drawOledLine');
  e.sendOledBuffer = () => record('sendOledBuffer');
  e.oled = {clearBuffer() { record('clearOled'); }};
  for (const name of ['serviceTrial', 'pollHx', 'pollAdxl', 'serviceSerial', 'initializeOled', 'initializeAdxl', 'updateOled', 'printStatus', 'printFifoDiagnostics', 'dumpTrial']) {
    e[name] = () => record(name);
  }
  e.abortTrial = reason => { record('abortTrial', [reason]); };
  e.delay = value => { record('schedulerDelay', [value]); e.now = (e.now + value) >>> 0; };
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
    ['wifiConfigured', ''], ['wifiPhaseName', ''], ['printWifiStatus', ''],
    ['initializeWifi', ''], ['pauseWifiForMeasurement', ''],
    ['setWifiEnabled', 'enabled'], ['serviceWifi', 'allowed'],
  ]) vm.runInContext(`function ${name}(${params}) {${translate(body(header, name))}}`, e);
  for (const [name, params] of [['isRecording', ''], ['armTrial', ''], ['processCommand', 'line'], ['loop', '']]) {
    vm.runInContext(`function ${name}(${params}) {${translate(body(firmware, name))}}`, e);
  }
  return e;
}

function initialized(options) {
  const e = environment(options);
  e.initializeWifi();
  e.calls.length = e.output.length = 0;
  return e;
}

function start(e, now = e.now) {
  e.now = now >>> 0;
  e.serviceWifi(true);
  assert.equal(e.wifi_phase, e.NET_CONNECTING);
  assert.equal(e.wifi_radio_started, true);
}

function connected() {
  const e = initialized();
  start(e);
  e.now = (e.now + e.WIFI_POLL_MS) >>> 0;
  e.linkStatus = e.WL_CONNECTED;
  e.localIp = [192, 0, 2, 10]; // Documentation/test address, not a real LAN.
  e.serviceWifi(true);
  assert.equal(e.wifi_phase, e.NET_CONNECTED);
  assert.equal(e.wifi_ip_valid, true);
  e.calls.length = e.output.length = 0;
  return e;
}

function networkCalls(e, name) {
  const names = new Set(['persistent', 'autoReconnect', 'mode', 'begin', 'status', 'localIP']);
  return e.calls.filter(call => name ? call.name === name : names.has(call.name));
}

test('initialization disables persistence/reconnect without starting a radio', () => {
  const e = environment();
  e.initializeWifi();
  assert.equal(e.wifi_phase, e.NET_WAIT);
  assert.equal(e.wifi_user_enabled, true);
  assert.equal(e.wifi_radio_started, false);
  assert.equal(e.wifi_ip_valid, false);
  assert.deepEqual(networkCalls(e).map(call => [call.name, ...call.args]), [['persistent', false], ['autoReconnect', false]]);
});

for (const options of [{enabled: false}, {ssid: ''}]) {
  test(`disabled/missing dummy configuration stays inactive: ${JSON.stringify(options)}`, () => {
    const e = initialized(options);
    e.forbidNetwork = true;
    e.serviceWifi(true);
    assert.equal(e.wifi_phase, e.NET_DISABLED);
    assert.equal(e.setWifiEnabled(true), false);
    assert.equal(networkCalls(e).length, 0);
  });
}

test('an explicitly configured open dummy network is not rejected for an empty password', () => {
  const e = initialized({password: ''});
  start(e);
  assert.deepEqual(networkCalls(e, 'begin')[0].args, [DUMMY_SSID, '']);
});

test('service allowed=false calls no WiFi API and changes no cached state', () => {
  const e = connected();
  const before = [e.wifi_phase, e.wifi_radio_started, e.wifi_ip_valid, e.wifi_poll_ms];
  e.forbidNetwork = true;
  e.serviceWifi(false);
  assert.deepEqual([e.wifi_phase, e.wifi_radio_started, e.wifi_ip_valid, e.wifi_poll_ms], before);
  assert.equal(networkCalls(e).length, 0);
});

test('first idle poll at millis=0 is valid and requests one STA connection with dummy credentials', () => {
  const e = initialized();
  start(e, 0);
  assert.equal(e.wifi_poll_seen, true);
  assert.equal(e.wifi_poll_ms, 0);
  assert.deepEqual(networkCalls(e, 'begin').map(call => call.args), [[DUMMY_SSID, DUMMY_PASSWORD]]);
  assert.deepEqual(networkCalls(e, 'autoReconnect').map(call => call.args), [[false]]);
  assert.equal(networkCalls(e, 'mode')[0].args[0], e.WIFI_STA);
});

test('poll throttle works across uint32 millis rollover', () => {
  const e = initialized();
  const origin = 0xFFFFFFF0;
  start(e, origin);
  e.calls.length = 0;
  e.now = (origin + e.WIFI_POLL_MS - 1) >>> 0;
  e.serviceWifi(true);
  assert.equal(networkCalls(e).length, 0);
  e.now = (origin + e.WIFI_POLL_MS) >>> 0;
  e.serviceWifi(true);
  assert.equal(networkCalls(e, 'status').length, 1);
  assert.equal(networkCalls(e, 'begin').length, 0);
});

test('valid connected status and nonzero IP update cache without exposing credentials', () => {
  const e = initialized();
  start(e);
  e.now += e.WIFI_POLL_MS;
  e.linkStatus = e.WL_CONNECTED;
  e.localIp = [192, 0, 2, 10];
  e.serviceWifi(true);
  assert.equal(e.wifi_phase, e.NET_CONNECTED);
  assert.equal(e.wifi_ip_valid, true);
  assert.deepEqual(Array.from(e.wifi_ip), [192, 0, 2, 10]);
  const rendered = JSON.stringify(e.output);
  assert.ok(!rendered.includes(DUMMY_SSID));
  assert.ok(!rendered.includes(DUMMY_PASSWORD));
});

test('unchanged connected IP does not repeatedly print; an IP change is reported', () => {
  const e = connected();
  e.now += e.WIFI_POLL_MS;
  e.serviceWifi(true);
  assert.equal(e.output.length, 0);
  e.now += e.WIFI_POLL_MS;
  e.localIp = [192, 0, 2, 11];
  e.serviceWifi(true);
  assert.equal(e.wifi_ip[3], 11);
  assert.ok(e.output.length > 0);
});

test('connected status with zero IP does not claim connection before the deadline', () => {
  const e = initialized();
  start(e);
  e.now += e.WIFI_POLL_MS;
  e.linkStatus = e.WL_CONNECTED;
  e.serviceWifi(true);
  assert.equal(e.wifi_phase, e.NET_CONNECTING);
  assert.equal(e.wifi_ip_valid, false);
  assert.equal(networkCalls(e, 'mode').length, 1);
});

test('connection timeout is bounded across rollover and does not begin again in the same poll', () => {
  const e = initialized();
  const origin = 0xFFFFFF00;
  start(e, origin);
  e.now = (origin + e.WIFI_TIMEOUT_MS - e.WIFI_POLL_MS) >>> 0;
  e.serviceWifi(true);
  assert.equal(e.wifi_phase, e.NET_CONNECTING);
  e.now = (origin + e.WIFI_TIMEOUT_MS) >>> 0;
  e.serviceWifi(true);
  assert.equal(e.wifi_phase, e.NET_WAIT);
  assert.equal(e.wifi_radio_started, false);
  assert.equal(e.wifi_ip_valid, false);
  assert.equal(e.wifi_retry_seen, true);
  assert.equal(networkCalls(e, 'begin').length, 1);
  assert.equal(networkCalls(e, 'mode').at(-1).args[0], e.WIFI_OFF);
});

test('cooldown is respected across rollover before the next STA attempt', () => {
  const e = connected();
  e.now = 0xFFFFFF00;
  assert.equal(e.pauseWifiForMeasurement(), true);
  const stoppedAt = e.wifi_retry_ms;
  e.calls.length = 0;
  e.now = (stoppedAt + e.WIFI_RETRY_MS - e.WIFI_POLL_MS) >>> 0;
  e.serviceWifi(true);
  assert.equal(networkCalls(e, 'begin').length, 0);
  e.now = (stoppedAt + e.WIFI_RETRY_MS) >>> 0;
  e.serviceWifi(true);
  assert.equal(e.wifi_phase, e.NET_CONNECTING);
  assert.equal(networkCalls(e, 'begin').length, 1);
});

for (const kind of ['link lost', 'connected with zero IP']) {
  test(`${kind} invalidates a formerly valid IP and schedules idle retry`, () => {
    const e = connected();
    e.now += e.WIFI_POLL_MS;
    if (kind === 'link lost') e.linkStatus = e.WL_DISCONNECTED;
    else e.localIp = [0, 0, 0, 0];
    e.serviceWifi(true);
    assert.equal(e.wifi_phase, e.NET_WAIT);
    assert.equal(e.wifi_radio_started, false);
    assert.equal(e.wifi_ip_valid, false);
    assert.equal(networkCalls(e, 'begin').length, 0);
    assert.deepEqual(networkCalls(e, 'mode').map(call => call.args), [[e.WIFI_OFF]]);
  });
}

test('pause succeeds only after mode OFF, clears old IP and retains requested ON intent', () => {
  const e = connected();
  e.modeDelayMs = 17; // Model synchronous call time without claiming real latency.
  const before = e.now;
  assert.equal(e.pauseWifiForMeasurement(), true);
  assert.equal(e.wifi_phase, e.NET_SUSPENDED);
  assert.equal(e.wifi_radio_started, false);
  assert.equal(e.wifi_ip_valid, false);
  assert.equal(e.wifi_user_enabled, true);
  assert.equal(e.wifi_retry_ms, before + 17);
  assert.deepEqual(networkCalls(e, 'mode').map(call => call.args), [[e.WIFI_OFF]]);
});

test('pause failure remains FAILED with radio possibly active, never a false suspended state', () => {
  const e = connected();
  e.offSucceeds = false;
  assert.equal(e.pauseWifiForMeasurement(), false);
  assert.equal(e.wifi_phase, e.NET_FAILED);
  assert.equal(e.wifi_radio_started, true);
  assert.equal(e.wifi_ip_valid, false);
  assert.equal(e.wifi_retry_seen, true);
});

test('pause with no started radio does not call WiFi mode', () => {
  const e = initialized();
  e.forbidNetwork = true;
  assert.equal(e.pauseWifiForMeasurement(), true);
  assert.equal(e.wifi_phase, e.NET_SUSPENDED);
  assert.equal(networkCalls(e).length, 0);
});

test('FAILED radio stop retries only after cooldown, and stopping does not also restart it', () => {
  const e = connected();
  e.offSucceeds = false;
  assert.equal(e.pauseWifiForMeasurement(), false);
  const failedAt = e.wifi_retry_ms;
  e.calls.length = 0;
  e.now = (failedAt + e.WIFI_RETRY_MS - e.WIFI_POLL_MS) >>> 0;
  e.serviceWifi(true);
  assert.equal(networkCalls(e).length, 0);
  e.offSucceeds = true;
  e.now = (failedAt + e.WIFI_RETRY_MS) >>> 0;
  e.serviceWifi(true);
  assert.equal(e.wifi_phase, e.NET_WAIT);
  assert.equal(e.wifi_radio_started, false);
  assert.equal(networkCalls(e, 'begin').length, 0);
  assert.deepEqual(networkCalls(e, 'mode').map(call => call.args), [[e.WIFI_OFF]]);
});

test('partial STA start failure conservatively marks the radio before mode is attempted', () => {
  const e = initialized();
  e.staSucceeds = false;
  e.serviceWifi(true);
  assert.equal(networkCalls(e, 'mode')[0].radio, true);
  assert.equal(e.wifi_phase, e.NET_FAILED);
  assert.equal(e.wifi_radio_started, true);
  assert.equal(networkCalls(e, 'begin').length, 0);
  e.calls.length = 0;
  e.offSucceeds = false;
  assert.equal(e.pauseWifiForMeasurement(), false);
  assert.equal(e.wifi_radio_started, true);
  assert.deepEqual(networkCalls(e, 'mode').map(call => call.args), [[e.WIFI_OFF]]);
});

test('manual OFF stops radio and later idle polls remain inactive', () => {
  const e = connected();
  assert.equal(e.setWifiEnabled(false), true);
  assert.equal(e.wifi_user_enabled, false);
  assert.equal(e.wifi_phase, e.NET_DISABLED);
  e.calls.length = 0;
  e.now += e.WIFI_RETRY_MS * 2;
  e.forbidNetwork = true;
  e.serviceWifi(true);
  assert.equal(networkCalls(e).length, 0);
});

test('manual OFF failure preserves failure evidence without allowing a new STA begin', () => {
  const e = connected();
  e.offSucceeds = false;
  assert.equal(e.setWifiEnabled(false), false);
  assert.equal(e.wifi_user_enabled, false);
  assert.equal(e.wifi_radio_started, true);
  assert.equal(e.wifi_phase, e.NET_FAILED);
  e.calls.length = 0;
  e.now += e.WIFI_RETRY_MS * 2;
  e.serviceWifi(true);
  assert.equal(networkCalls(e, 'begin').length, 0);
});

test('manual ON only schedules a later idle connection and releases cooldown', () => {
  const e = initialized();
  assert.equal(e.setWifiEnabled(false), true);
  e.calls.length = 0;
  assert.equal(e.setWifiEnabled(true), true);
  assert.equal(e.wifi_phase, e.NET_WAIT);
  assert.equal(e.wifi_retry_seen, false);
  assert.equal(e.wifi_poll_seen, false);
  assert.equal(networkCalls(e).length, 0);
  start(e);
});

test('manual ON while connected does not restart the driver', () => {
  const e = connected();
  assert.equal(e.setWifiEnabled(true), true);
  e.serviceWifi(true);
  assert.equal(e.wifi_phase, e.NET_CONNECTED);
  assert.equal(networkCalls(e, 'begin').length, 0);
  assert.equal(networkCalls(e, 'mode').length, 0);
});

test('WiFi STATUS prints cache only, with no WiFi API or dummy credential disclosure', () => {
  const e = connected();
  e.forbidNetwork = true;
  e.processCommand('WIFI STATUS');
  assert.equal(networkCalls(e).length, 0);
  const output = JSON.stringify(e.output);
  assert.ok(!output.includes(DUMMY_SSID));
  assert.ok(!output.includes(DUMMY_PASSWORD));
  assert.ok(e.output.length > 0);
});

test('ARM stops WiFi before FIFO reset and the actual ARM timestamp', () => {
  const e = connected();
  e.acc_last_ms = e.now;
  e.modeDelayMs = 17;
  e.armTrial();
  assert.equal(e.phase, e.ARMED);
  assert.equal(e.wifi_radio_started, false);
  assert.equal(e.trial_id, 11);
  const stop = e.calls.findIndex(call => call.name === 'mode' && call.args[0] === e.WIFI_OFF);
  const fifo = e.calls.findIndex(call => call.name === 'resetFifo');
  const origin = e.calls.findIndex(call => call.name === 'armedTimestamp');
  assert.ok(stop >= 0 && fifo > stop && origin > fifo);
  assert.ok(e.calls[origin].now >= e.calls[stop].now + 17);
  e.calls.length = 0;
  e.forbidNetwork = true;
  e.loop();
  assert.equal(networkCalls(e).length, 0);
});

test('ARM radio-stop failure preserves prior trial and never resets FIFO or timestamps', () => {
  const e = connected();
  e.acc_last_ms = e.now;
  e.offSucceeds = false;
  e.armTrial();
  assert.equal(e.phase, e.IDLE);
  assert.equal(e.trial_id, 10);
  assert.equal(e.acc_count, 8);
  assert.equal(e.hx_count, 6);
  assert.equal(e.wifi_radio_started, true);
  assert.ok(!e.calls.some(call => ['resetFifo', 'armedTimestamp'].includes(call.name)));
});

for (const [command, mode] of [['TARE', 1], ['CAL 200', 2]]) {
  test(`${command} stops WiFi before enabling collection and starting its clock`, () => {
    const e = connected();
    e.modeDelayMs = 17;
    const before = e.now;
    e.processCommand(command);
    const stop = networkCalls(e, 'mode').find(call => call.args[0] === e.WIFI_OFF);
    assert.ok(stop);
    assert.equal(stop.cal, 0);
    assert.equal(e.cal_mode, mode);
    assert.equal(e.cal_start_ms, before + 17);
    assert.equal(e.wifi_radio_started, false);
    assert.equal(e.cal_count, 0);
    if (mode === 2) assert.equal(e.cal_mass_g, 200);
    e.calls.length = 0;
    e.last_retry_ms = e.last_display_ms = e.last_live_ms = e.now;
    e.forbidNetwork = true;
    e.loop();
    assert.equal(networkCalls(e).length, 0);
  });

  test(`${command} cannot collect or alter calibration when radio stop fails`, () => {
    const e = connected();
    e.offSucceeds = false;
    e.processCommand(command);
    assert.equal(e.cal_mode, 0);
    assert.equal(e.cal_count, 9);
    assert.equal(e.tare_raw, -10);
    assert.equal(e.counts_per_g, 2);
    assert.equal(e.calibration_valid, true);
    assert.equal(e.wifi_radio_started, true);
  });
}

test('CAL without successful TARE rejects before touching WiFi', () => {
  const e = connected();
  e.tare_valid = false;
  e.forbidNetwork = true;
  e.processCommand('CAL 200');
  assert.equal(e.cal_mode, 0);
  assert.equal(networkCalls(e).length, 0);
});

for (const phase of ['ARMED', 'WAIT_DN', 'CAPTURE', 'DONE', 'FAULT']) {
  test(`actual loop and WIFI ON/OFF guards forbid network activity in ${phase}`, () => {
    const e = initialized();
    e.phase = e[phase];
    e.forbidNetwork = true;
    e.processCommand('WIFI ON');
    e.processCommand('WIFI OFF');
    e.loop();
    assert.equal(e.wifi_user_enabled, true);
    assert.equal(e.wifi_radio_started, false);
    assert.equal(networkCalls(e).length, 0);
  });
}

for (const calMode of [1, 2]) {
  test(`idle calibration mode ${calMode} cannot honor WIFI ON/OFF or restart radio`, () => {
    const e = initialized();
    e.cal_mode = calMode;
    e.forbidNetwork = true;
    e.processCommand('WIFI ON');
    e.processCommand('WIFI OFF');
    e.loop();
    assert.equal(e.wifi_user_enabled, true);
    assert.equal(e.cal_mode, calMode);
    assert.equal(networkCalls(e).length, 0);
  });
}
