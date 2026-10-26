'use strict';

// Public C++ functions run against an in-memory OLED mock. No private header,
// hardware, WiFi stack, font rasterizer, or serial port is loaded by these tests.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const source = fs.readFileSync(path.join(__dirname, '..', '05-test', '05-test.ino'), 'utf8');
const publicNetworkHeader = fs.readFileSync(path.join(__dirname, '..', '05-test', 'network_wifi.h'), 'utf8');

function body(name) {
  const declaration = new RegExp(`^(?:bool|void|const char \\*)\\s*${name}\\(`, 'm').exec(source);
  assert.ok(declaration, `Missing public source function ${name}`);
  const open = source.indexOf('{', declaration.index);
  const end = source.indexOf('\n}', open);
  assert.ok(open >= 0 && end > open, `Incomplete function ${name}`);
  return source.slice(open + 1, end).replace(/\/\/[^\n]*/g, '');
}

function translate(text) {
  return text
    .replace(/static const uint16_t (\w+)\[\d+\] = \{([^}]+)\};/g, 'const $1 = [$2];')
    .replace(/static const char \*names\[\] = \{([^}]+)\};/g, 'const names = [$1];')
    .replace(/static_cast<[^>]+>\(([^()]*)\)/g, '($1)')
    .replace(/char (\w+)\[(\d+)\];/g, "let $1 = Array($2).fill('\\0');")
    .replace(/sizeof\((\w+)\)/g, '$1.length')
    .replace(/\bsize_t (\w+)/g, 'let $1')
    .replace(/\b(?:uint8_t|uint16_t|uint32_t) (\w+)/g, 'let $1')
    .replace(/\b(0x[\dA-Fa-f]+|\d+)(?:UL|U|L)\b/g, '$1');
}

function cString(value) {
  if (typeof value === 'string') return value;
  const end = value.indexOf('\0');
  return value.slice(0, end < 0 ? value.length : end).join('');
}

// Only models the bounded ASCII formatting used by updateOled, not font pixels.
function snprintf(output, size, format, ...values) {
  let index = 0;
  const text = format.replace(/%([+]?)(?:\.(\d+))?(l?[sduf])/g, (_match, sign, precision, type) => {
    const value = values[index++];
    if (type === 's') return String(value);
    const number = Number(value);
    let rendered = type === 'f' ? number.toFixed(Number(precision ?? 6)) : String(type === 'u' ? number >>> 0 : Math.trunc(number));
    if (sign === '+' && number >= 0) rendered = '+' + rendered;
    return rendered;
  });
  output.fill('\0');
  const clipped = text.slice(0, size - 1);
  for (let i = 0; i < clipped.length; i++) output[i] = clipped[i];
  return text.length;
}

function environment() {
  const e = {
    IDLE: 0, ARMED: 1, WAIT_DN: 2, CAPTURE: 3, DONE: 4, FAULT: 5,
    phase: 0, reason: 0, cal_mode: 0, cal_count: 7, CAL_SAMPLES: 32,
    calibration_valid: false, oled_address: 0x3C, ack: true,
    acc_seen: true, acc_last_ms: 990, acc_x: 3, acc_y: 4, acc_z: -20,
    hx_seen: true, hx_raw: -1234, hxState: 'OK', UP_PIN: 34, DN_PIN: 35,
    levels: {34: 1, 35: 0}, acc_count: 12, hx_count: 4,
    upper_us: 1000, lower_us: 11000, MIN_TRANSIT_US: 200,
    distance_mm: 100, upper_active: 0, lower_active: 1,
    wifi_phase: 0, wifi_ip_valid: false, wifi_radio_started: false,
    now: 1000, pixels: new Set(), lines: [], frames: [], trace: [], networkTouches: [],
    sqrt: Math.sqrt, snprintf,
  };
  const phaseEnum = /enum WifiPhase\s*:[^{]+\{([^}]+)\}/.exec(publicNetworkHeader);
  assert.ok(phaseEnum, 'Missing public WifiPhase enum');
  phaseEnum[1].split(',').forEach((name, index) => { e[name.trim()] = index; });
  e.WiFi = new Proxy({}, {
    get(_target, name) {
      e.networkTouches.push(String(name));
      throw new Error(`OLED drawing must not access WiFi.${String(name)}`);
    },
  });
  e.millis = () => e.now;
  e.hxStatus = () => e.hxState;
  e.digitalRead = pin => e.levels[pin];
  e.i2cAck = address => { e.trace.push({kind: 'ack', address}); return e.ack; };
  e.strlen = text => cString(text).length;
  e.strncpy = (out, text, limit) => {
    const value = cString(text);
    for (let i = 0; i < limit; i++) out[i] = i < value.length ? value[i] : '\0';
  };
  e.oled = {
    clearBuffer() {
      e.trace.push({kind: 'clear'});
      e.pixels.clear();
      e.lines.length = 0;
    },
    drawPixel(x, y) {
      e.trace.push({kind: 'pixel', x, y});
      e.pixels.add(`${x},${y}`);
    },
    getStrWidth(text) { return cString(text).length * 4; },
    setClipWindow(...bounds) { assert.deepEqual(bounds, [4, 4, 124, 60]); },
    setMaxClipWindow() {},
    drawStr(x, baseline, text) {
      const value = cString(text);
      const line = {x, baseline, text: value};
      e.lines.push(line);
      e.trace.push({kind: 'text', ...line});
    },
  };
  e.sendOledBuffer = () => {
    e.trace.push({kind: 'send'});
    e.frames.push({
      pixels: Array.from(e.pixels),
      lines: e.lines.map(line => ({...line})),
    });
  };
  vm.createContext(e);
  for (const [name, params] of [
    ['isRecording', ''], ['phaseName', ''], ['reasonName', ''],
    ['drawOledLine', 'row, text'], ['drawWifiIcon', ''], ['updateOled', ''],
  ]) vm.runInContext(`function ${name}(${params}) {${translate(body(name))}}`, e);
  return e;
}

function setConnected(e) {
  e.wifi_phase = e.NET_CONNECTED;
  e.wifi_ip_valid = true;
  e.wifi_radio_started = true;
}

function assertIconBounds(pixelKeys) {
  assert.ok(pixelKeys.length > 0, 'A valid connection must produce visible icon points');
  const pixels = pixelKeys.map(key => key.split(',').map(Number));
  for (const [x, y] of pixels) {
    assert.ok(Number.isInteger(x) && x >= 115 && x <= 123, `Icon x=${x} outside reserved area`);
    assert.ok(Number.isInteger(y) && y >= 4 && y <= 9, `Icon y=${y} outside first row`);
  }
  assert.equal(Math.min(...pixels.map(point => point[0])), 115);
  assert.equal(Math.max(...pixels.map(point => point[0])), 123);
  assert.equal(Math.min(...pixels.map(point => point[1])), 4);
  assert.equal(Math.max(...pixels.map(point => point[1])), 9);
}

for (const phase of ['NET_DISABLED', 'NET_WAIT', 'NET_CONNECTING', 'NET_CONNECTED', 'NET_SUSPENDED', 'NET_FAILED']) {
  for (const validIp of [false, true]) {
    for (const radio of [false, true]) {
      test(`actual icon guard: ${phase}, validIP=${validIp}, radio=${radio}`, () => {
        const e = environment();
        e.wifi_phase = e[phase];
        e.wifi_ip_valid = validIp;
        e.wifi_radio_started = radio;
        e.drawWifiIcon();
        if (phase === 'NET_CONNECTED' && validIp && radio) assertIconBounds(Array.from(e.pixels));
        else assert.equal(e.pixels.size, 0);
        assert.equal(e.networkTouches.length, 0);
        assert.equal(e.trace.filter(call => call.kind === 'send').length, 0);
      });
    }
  }
}

test('connected refresh clears first, draws bounded icon and sends one complete eight-row frame', () => {
  const e = environment();
  setConnected(e);
  e.updateOled();
  assert.equal(e.frames.length, 1);
  assert.equal(e.trace.filter(call => call.kind === 'clear').length, 1);
  assert.equal(e.trace.filter(call => call.kind === 'send').length, 1);
  assert.equal(e.frames[0].lines.length, 8);
  assertIconBounds(e.frames[0].pixels);
  const clear = e.trace.findIndex(call => call.kind === 'clear');
  const pixel = e.trace.findIndex(call => call.kind === 'pixel');
  const send = e.trace.findIndex(call => call.kind === 'send');
  assert.ok(clear >= 0 && pixel > clear && send > pixel);
  assert.equal(e.networkTouches.length, 0);
});

for (const [name, change] of [
  ['disconnected', e => { e.wifi_phase = e.NET_WAIT; e.wifi_ip_valid = false; }],
  ['invalid IP', e => { e.wifi_ip_valid = false; }],
  ['radio stopped', e => { e.wifi_radio_started = false; }],
  ['suspended', e => { e.wifi_phase = e.NET_SUSPENDED; e.wifi_radio_started = false; }],
]) {
  test(`next ${name} refresh erases the previously visible icon`, () => {
    const e = environment();
    setConnected(e);
    e.updateOled();
    assertIconBounds(e.frames[0].pixels);
    change(e);
    e.updateOled();
    assert.equal(e.frames.length, 2);
    assert.deepEqual(e.frames[1].pixels, []);
    assert.equal(e.frames[1].lines.length, 8);
    assert.equal(e.trace.filter(call => call.kind === 'clear').length, 2);
    assert.equal(e.networkTouches.length, 0);
  });
}

for (const calibrated of [false, true]) {
  test(`first-row calibration label starts at x4 without overlapping the right icon, calibrated=${calibrated}`, () => {
    const e = environment();
    e.calibration_valid = calibrated;
    setConnected(e);
    e.updateOled();
    const first = e.frames[0].lines[0];
    assert.equal(first.x, 4);
    assert.equal(first.baseline, 9);
    assert.equal(first.text, `05 TEST ${calibrated ? 'CAL_USER' : 'UNCAL'}`);
    const firstGlyphX = first.x + first.text.match(/^ */)[0].length * 4;
    assert.equal(firstGlyphX, 4);
    const lastGlyphX = first.x + e.oled.getStrWidth(first.text) - 1;
    assert.ok(Array.from(e.pixels).every(key => Number(key.split(',')[0]) > lastGlyphX));
  });
}

for (const scenario of ['idle', 'done', 'fault', 'calibrating', 'sensor missing']) {
  test(`icon visibility does not change the other seven lines: ${scenario}`, () => {
    const e = environment();
    if (scenario === 'done') e.phase = e.DONE;
    if (scenario === 'fault') { e.phase = e.FAULT; e.reason = 3; }
    if (scenario === 'calibrating') { e.cal_mode = 1; e.calibration_valid = true; }
    if (scenario === 'sensor missing') { e.acc_seen = false; e.hx_seen = false; e.hxState = 'TIMEOUT'; }
    e.updateOled();
    const offline = e.frames[0];
    setConnected(e);
    e.updateOled();
    const online = e.frames[1];
    assert.equal(offline.lines.length, 8);
    assert.equal(online.lines.length, 8);
    assert.deepEqual(online.lines, offline.lines);
    assert.deepEqual(online.lines.slice(1).map(line => line.baseline), [16, 23, 30, 37, 44, 51, 58]);
    assert.ok(online.lines.every(line => line.text.length > 0));
    assert.deepEqual(offline.pixels, []);
    assertIconBounds(online.pixels);
    assert.equal(e.networkTouches.length, 0);
  });
}

for (const phase of ['ARMED', 'WAIT_DN', 'CAPTURE']) {
  test(`recording guard in ${phase} prevents clear, icon draw and OLED send`, () => {
    const e = environment();
    setConnected(e);
    e.phase = e[phase];
    e.updateOled();
    assert.deepEqual(e.trace, []);
    assert.equal(e.frames.length, 0);
    assert.equal(e.pixels.size, 0);
    assert.equal(e.networkTouches.length, 0);
  });
}

test('missing OLED returns before draw and does not access WiFi', () => {
  const e = environment();
  e.oled_address = 0;
  setConnected(e);
  e.updateOled();
  assert.deepEqual(e.trace, []);
  assert.equal(e.frames.length, 0);
  assert.equal(e.networkTouches.length, 0);
});

test('failed OLED ACK disables further drawing without claiming a refreshed screen', () => {
  const e = environment();
  e.ack = false;
  setConnected(e);
  e.updateOled();
  assert.equal(e.oled_address, 0);
  assert.deepEqual(e.trace.map(call => call.kind), ['ack']);
  assert.equal(e.frames.length, 0);
  assert.equal(e.networkTouches.length, 0);
});
