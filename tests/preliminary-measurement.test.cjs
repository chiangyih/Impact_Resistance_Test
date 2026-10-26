'use strict';

// All embedded data are SYNTHETIC software fixtures, never device measurements.
// These tests check an offline dataflow contract, not waveforms or measurement accuracy.
const test = require('node:test');
const assert = require('node:assert/strict');
const { createHash } = require('node:crypto');
const { spawnSync } = require('node:child_process');
const path = require('node:path');
const fs = require('node:fs');
const os = require('node:os');
const { analyzeDump } = require('../tools/preliminary-measurement.cjs');

const TOOL_PATH = path.resolve(__dirname, '../tools/preliminary-measurement.cjs');
const HEADER = 'kind,t_read_us,x_raw,y_raw,z_raw,x_g,y_g,z_g,mag_g,hx_raw,hx_status,mass_g_static';
const BEGIN = '# BEGIN trial=7 state=DONE reason=NONE CAL=UNCAL distance_mm=100.000000 polarity=0/0';
const TRIAL = '# TRIAL 7 DONE reason=NONE A=2 H=2 SAT=0 HX_STALE=0 ACC_STALE=0; DUMP to export; CLEAR before next ARM';
const QUALITY = '# tare_valid=0 offset=0.000000 counts_per_g=0.000000000 HX_ERR=0 ACC_ERR=0 FIFO_FULL=0 SAT=0 HX_STALE=0 ACC_STALE=0 ignored_edges=0';
const END = '# END; RAM data retained until CLEAR or power loss; no file automatically saved on computer';
const ROWS = [
  'ACC,1000,3,4,0,0.147000,0.196000,0.000000,0.245000,NA,NA,NA',
  'ACC,2000,-20,0,0,-0.980000,0.000000,0.000000,0.980000,NA,NA,NA',
  'HX,500,NA,NA,NA,NA,NA,NA,NA,-1234,OK,NA',
  'HX,1500,NA,NA,NA,NA,NA,NA,NA,-1200,OK,NA',
];

function syntheticDump({ trial = TRIAL, begin = BEGIN, quality = QUALITY, rows = ROWS } = {}) {
  return [
    '# SYNTHETIC TEST DATA ONLY; not a calibration, drop test, or acceptance record',
    trial,
    begin,
    '# HX_SCK=25 HX_DATA=33 RGB_PIN=32 A128 RATE=physical_L10_or_H80; ACC_ODR=100Hz; timestamps=host_readout_us_from_ARM; rows_grouped_by_sensor',
    quality,
    HEADER,
    ...rows,
    END,
  ].filter(line => line !== null).join('\n') + '\n';
}

function sha256(text) {
  return createHash('sha256').update(text, 'utf8').digest('hex');
}

function analyze(text, extra = {}) {
  return analyzeDump(text, { sourceKind: 'synthetic', ...extra });
}

function profileFor(text, overrides = {}) {
  return {
    profile_id: 'SYNTHETIC-PROFILE-ONLY',
    offset_counts: -1250,
    counts_per_g: 2,
    source_kind: 'synthetic',
    input_sha256: sha256(text),
    ...overrides,
  };
}

function rowWith(text, kind, ordinal, edits) {
  let seen = 0;
  let changed = false;
  const result = text.split('\n').map(line => {
    if (!line.startsWith(`${kind},`) || seen++ !== ordinal) return line;
    const fields = line.split(',');
    for (const [column, value] of Object.entries(edits)) fields[Number(column)] = String(value);
    changed = true;
    return fields.join(',');
  }).join('\n');
  assert.ok(changed, 'The synthetic row mutation must find its intended row');
  return result;
}

function assertOfflineOnly(report) {
  assert.deepEqual(report.validation, {
    stage: 'offline-dataflow-only',
    hardware_verified: false,
    measurement_accepted: false,
    material_comparison_accepted: false,
  });
}

function assertIssue(report, severity) {
  assert.ok(Array.isArray(report.integrity.issues));
  assert.ok(report.integrity.issues.some(issue => (
    issue.severity === severity && typeof issue.code === 'string' && issue.code.length > 0
    && typeof issue.message === 'string' && issue.message.length > 0
  )), `Expected a structured ${severity} issue`);
}

function assertIncompleteError(text) {
  const report = analyze(text);
  assert.equal(report.integrity.complete, false);
  assertIssue(report, 'error');
  assertOfflineOnly(report);
  return report;
}

function cli(args, input = '') {
  const result = spawnSync(process.execPath, [TOOL_PATH, ...args], {
    input,
    encoding: 'utf8',
    timeout: 10000,
    maxBuffer: 2 * 1024 * 1024,
    windowsHide: true,
  });
  assert.ifError(result.error);
  assert.equal(result.signal, null);
  return result;
}

test('SYNTHETIC complete DUMP verifies rows, summaries, raw hash and offline-only status', () => {
  const input = syntheticDump();
  const report = analyze(input);
  assert.equal(report.schema_version, 1);
  assert.equal(report.source_kind, 'synthetic');
  assert.equal(report.source_sha256, sha256(input));
  assert.equal(String(report.trial.id), '7');
  assert.equal(report.trial.state, 'DONE');
  assert.equal(report.trial.reason, 'NONE');
  assert.equal(report.integrity.structure_valid, true);
  assert.equal(report.integrity.export_count_verified, true);
  assert.equal(report.integrity.complete, true);
  assert.deepEqual(report.integrity.expected, { acc: 2, hx: 2 });
  assert.deepEqual(report.integrity.observed, { acc: 2, hx: 2 });
  assert.ok(!report.integrity.issues.some(issue => issue.severity === 'error'));
  assert.deepEqual(report.calibration, { mode: 'none', profile_id: null });
  assert.equal(report.quality.scope, 'frozen_trial_delta');
  for (const name of ['hx_errors', 'acc_errors', 'fifo_full', 'sat', 'ignored_edges']) {
    assert.equal(report.quality[name], 0, `${name} has an explicit synthetic zero`);
  }
  assert.equal(report.measurements.acc.count, 2);
  assert.ok(Math.abs(report.measurements.acc.sampled_magnitude_peak_g - 0.98) < 1e-12);
  assert.ok(Math.abs(report.measurements.acc.sampled_axis_abs_peak_g - 0.98) < 1e-12);
  assert.equal(report.measurements.hx.count, 2);
  assert.equal(report.measurements.hx.non_sat_count, 2);
  assert.equal(report.measurements.hx.raw_min, -1234);
  assert.equal(report.measurements.hx.raw_max, -1200);
  assert.equal(report.measurements.hx.recorded_mass_g_static_min, null);
  assert.equal(report.measurements.hx.recorded_mass_g_static_max, null);
  assert.equal(report.measurements.hx.recalculated_mass_g_min, null);
  assert.equal(report.measurements.hx.recalculated_mass_g_max, null);
  assertOfflineOnly(report);
});

test('sourceKind is required and rejects undeclared or unsupported provenance', () => {
  const input = syntheticDump();
  assert.throws(() => analyzeDump(input), Error);
  for (const sourceKind of [undefined, null, '', 'SYNTHETIC', 'unknown', 0]) {
    assert.throws(() => analyzeDump(input, { sourceKind }), Error);
  }
});

for (const sourceKind of ['synthetic', 'historical', 'new']) {
  test(`SYNTHETIC fixture exercises declared sourceKind=${sourceKind} without upgrading acceptance`, () => {
    const report = analyzeDump(syntheticDump(), { sourceKind });
    assert.equal(report.source_kind, sourceKind);
    assertOfflineOnly(report);
  });
}

test('missing expected counts is a warning, never a complete export', () => {
  const report = analyze(syntheticDump({ trial: null }));
  assert.equal(report.integrity.structure_valid, true);
  assert.equal(report.integrity.export_count_verified, false);
  assert.equal(report.integrity.complete, false);
  assert.deepEqual(report.integrity.expected, { acc: null, hx: null });
  assert.deepEqual(report.integrity.observed, { acc: 2, hx: 2 });
  assertIssue(report, 'warning');
  assertOfflineOnly(report);
});

test('a SYNTHETIC STATUS snapshot cannot masquerade as a trial DUMP', () => {
  const report = assertIncompleteError('# SYNTHETIC STATUS ONLY\n# STATUS phase=IDLE CAL=UNCAL HX=OK RAW=-1234 RX=1 ERR=0\n');
  assert.equal(report.trial.id, null);
  assert.equal(report.measurements.acc.sampled_magnitude_peak_g, null);
  assert.equal(report.measurements.hx.raw_min, null);
  assert.equal(report.measurements.hx.recalculated_mass_g_min, null);
});

test('a complete FAULT export is retained without turning dataflow success into measurement acceptance', () => {
  const input = syntheticDump({
    trial: TRIAL.replace('DONE reason=NONE', 'FAULT reason=USER_STOP'),
    begin: BEGIN.replace('state=DONE reason=NONE', 'state=FAULT reason=USER_STOP'),
  });
  const report = analyze(input);
  assert.equal(report.integrity.complete, true);
  assert.equal(report.trial.state, 'FAULT');
  assert.equal(report.trial.reason, 'USER_STOP');
  assert.deepEqual(report.integrity.observed, { acc: 2, hx: 2 });
  assertIssue(report, 'warning');
  assertOfflineOnly(report);
});

const invalidCases = [
  ['missing BEGIN', text => text.replace(BEGIN + '\n', '')],
  ['missing END', text => text.replace(END + '\n', '')],
  ['duplicate BEGIN', text => text.replace(BEGIN, BEGIN + '\n' + BEGIN)],
  ['missing schema header', text => text.replace(HEADER + '\n', '')],
  ['missing schema field', text => text.replace(HEADER, HEADER.replace(',mass_g_static', ''))],
  ['TRIAL and BEGIN have different trial IDs', text => text.replace('# TRIAL 7 ', '# TRIAL 8 ')],
  ['expected ACC count differs from rows', text => text.replace('A=2 H=2', 'A=3 H=2')],
  ['expected HX count differs from rows', text => text.replace('A=2 H=2', 'A=2 H=1')],
  ['duplicate TRIAL summary', text => text.replace(TRIAL, TRIAL + '\n' + TRIAL)],
  ['extra row field', text => text.replace(ROWS[0], ROWS[0] + ',NA')],
  ['missing row field', text => text.replace(ROWS[0], ROWS[0].slice(0, ROWS[0].lastIndexOf(',')))],
  ['unknown sensor kind', text => text.replace(ROWS[0], ROWS[0].replace('ACC,', 'OTHER,'))],
  ['NaN acceleration', text => rowWith(text, 'ACC', 0, { 5: 'NaN' })],
  ['infinite acceleration', text => rowWith(text, 'ACC', 0, { 6: 'Infinity' })],
  ['non-finite time', text => rowWith(text, 'ACC', 0, { 1: 'NaN' })],
  ['negative time', text => rowWith(text, 'HX', 0, { 1: -1 })],
  ['ACC time reverses', text => rowWith(text, 'ACC', 1, { 1: 999 })],
  ['HX time reverses', text => rowWith(text, 'HX', 1, { 1: 499 })],
  ['HX raw exceeds positive 24-bit range', text => rowWith(text, 'HX', 0, { 9: 8388608 })],
  ['HX raw exceeds negative 24-bit range', text => rowWith(text, 'HX', 0, { 9: -8388609 })],
  ['ACC raw exceeds signed 16-bit range', text => rowWith(text, 'ACC', 0, { 2: 32768 })],
  ['required ACC raw is NA', text => rowWith(text, 'ACC', 0, { 2: 'NA' })],
  ['required ACC g is NA', text => rowWith(text, 'ACC', 0, { 5: 'NA' })],
  ['required HX raw is NA', text => rowWith(text, 'HX', 0, { 9: 'NA' })],
  ['required HX status is NA', text => rowWith(text, 'HX', 0, { 10: 'NA' })],
  ['ACC row carries an HX raw value', text => rowWith(text, 'ACC', 0, { 9: 0 })],
  ['HX row carries acceleration raw', text => rowWith(text, 'HX', 0, { 2: 0 })],
  ['HX row carries acceleration g', text => rowWith(text, 'HX', 0, { 5: 0 })],
];
for (const [name, corrupt] of invalidCases) {
  test(`SYNTHETIC malformed export: ${name}`, () => assertIncompleteError(corrupt(syntheticDump())));
}

test('grouped ACC/HX readout times are not compared across sensors', () => {
  const input = rowWith(rowWith(syntheticDump(), 'HX', 0, { 1: 1 }), 'HX', 1, { 1: 2 });
  const report = analyze(input);
  assert.equal(report.integrity.complete, true);
  assert.equal(report.integrity.structure_valid, true);
  assertOfflineOnly(report);
});

test('unknown quality counters stay null instead of becoming invented zeros', () => {
  const report = analyze(syntheticDump({ trial: null, quality: null }));
  for (const key of ['hx_errors', 'acc_errors', 'fifo_full', 'hx_stale', 'acc_stale', 'ignored_edges']) {
    assert.equal(report.quality[key], null, `${key} was not recorded`);
  }
  assert.equal(report.quality.scope, 'frozen_trial_delta');
});

test('frozen trial error counters are preserved without accepting hardware or measurements', () => {
  const quality = QUALITY.replace('HX_ERR=0', 'HX_ERR=3').replace('ACC_ERR=0', 'ACC_ERR=2')
    .replace('FIFO_FULL=0', 'FIFO_FULL=1').replace('ignored_edges=0', 'ignored_edges=4');
  const report = analyze(syntheticDump({ quality }));
  assert.equal(report.quality.hx_errors, 3);
  assert.equal(report.quality.acc_errors, 2);
  assert.equal(report.quality.fifo_full, 1);
  assert.equal(report.quality.ignored_edges, 4);
  assert.equal(report.quality.scope, 'frozen_trial_delta');
  assertOfflineOnly(report);
});

test('SYNTHETIC SAT rails are retained as rows, not trusted raw/mass extrema or zero mass', () => {
  const rows = ROWS.slice(0, 2).concat([
    'HX,500,NA,NA,NA,NA,NA,NA,NA,-8388608,SAT,NA',
    'HX,1500,NA,NA,NA,NA,NA,NA,NA,8388607,SAT,NA',
  ]);
  const input = syntheticDump({ rows, trial: TRIAL.replace('SAT=0', 'SAT=2'), quality: QUALITY.replace('SAT=0', 'SAT=2') });
  const report = analyze(input, { calibrationProfile: profileFor(input) });
  assert.equal(report.integrity.structure_valid, true);
  assert.equal(report.integrity.export_count_verified, true);
  assert.equal(report.quality.sat, 2);
  assert.equal(report.measurements.hx.count, 2);
  assert.equal(report.measurements.hx.non_sat_count, 0);
  for (const key of ['raw_min', 'raw_max', 'recorded_mass_g_static_min', 'recorded_mass_g_static_max', 'recalculated_mass_g_min', 'recalculated_mass_g_max']) {
    assert.equal(report.measurements.hx[key], null, `${key} must not convert a rail into a measurement`);
  }
  assertOfflineOnly(report);
});

test('even an OK label cannot make a rail eligible for external recalculation', () => {
  let input = rowWith(syntheticDump(), 'HX', 0, { 9: -8388608 });
  input = rowWith(input, 'HX', 1, { 9: 8388607 });
  const report = analyze(input, { calibrationProfile: profileFor(input) });
  assert.equal(report.measurements.hx.recalculated_mass_g_min, null);
  assert.equal(report.measurements.hx.recalculated_mass_g_max, null);
  assertOfflineOnly(report);
});

function syntheticCalUserDump() {
  return syntheticDump({
    begin: BEGIN.replace('CAL=UNCAL', 'CAL=CAL_USER_STATIC_NOT_VALIDATED'),
    quality: QUALITY.replace('tare_valid=0', 'tare_valid=1').replace('offset=0.000000', 'offset=-1300.000000')
      .replace('counts_per_g=0.000000000', 'counts_per_g=10.000000000'),
    rows: ROWS.slice(0, 2).concat([
      'HX,500,NA,NA,NA,NA,NA,NA,NA,-1234,OK,6.600000',
      'HX,1500,NA,NA,NA,NA,NA,NA,NA,-1200,OK,10.000000',
    ]),
  });
}

test('CAL_USER retains recorded unvalidated masses without implicitly applying its printed coefficients', () => {
  const report = analyze(syntheticCalUserDump());
  assert.deepEqual(report.calibration, { mode: 'none', profile_id: null });
  assert.equal(report.measurements.hx.recorded_mass_g_static_min, 6.6);
  assert.equal(report.measurements.hx.recorded_mass_g_static_max, 10);
  assert.equal(report.measurements.hx.recalculated_mass_g_min, null);
  assert.equal(report.measurements.hx.recalculated_mass_g_max, null);
  assertOfflineOnly(report);
});

test('bound external profile recalculates separate masses and preserves source bytes and recorded masses', () => {
  const input = syntheticCalUserDump();
  const originalText = input;
  const profile = Object.freeze(profileFor(input));
  const report = analyze(input, { calibrationProfile: profile });
  assert.equal(input, originalText);
  assert.equal(report.source_sha256, sha256(originalText));
  assert.deepEqual(report.calibration, { mode: 'external_unvalidated', profile_id: profile.profile_id });
  assert.equal(report.measurements.hx.recorded_mass_g_static_min, 6.6);
  assert.equal(report.measurements.hx.recorded_mass_g_static_max, 10);
  assert.equal(report.measurements.hx.recalculated_mass_g_min, 8);
  assert.equal(report.measurements.hx.recalculated_mass_g_max, 25);
  assert.deepEqual(report.records.hx.map(row => row.hx_raw), [-1234, -1200]);
  assert.deepEqual(report.records.hx.map(row => row.recorded_mass_g_static), [6.6, 10]);
  assert.deepEqual(report.records.hx.map(row => row.recalculated_mass_g), [8, 25]);
  assert.equal(analyze(input).measurements.hx.recalculated_mass_g_min, null);
  assertOfflineOnly(report);
});

test('an explicitly recorded CAL_USER zero is retained as zero, unlike missing UNCAL mass', () => {
  const input = rowWith(syntheticCalUserDump(), 'HX', 0, { 9: -1300, 11: 0 });
  const report = analyze(input);
  assert.equal(report.integrity.complete, true);
  assert.equal(report.measurements.hx.recorded_mass_g_static_min, 0);
  assert.equal(report.measurements.hx.recorded_mass_g_static_max, 10);
  assert.equal(report.measurements.hx.recalculated_mass_g_min, null);
  assertOfflineOnly(report);
});

test('negative finite profile scale is allowed and recalculated extrema are ordered by mass', () => {
  const input = syntheticDump();
  const report = analyze(input, { calibrationProfile: profileFor(input, { counts_per_g: -2 }) });
  assert.equal(report.measurements.hx.recalculated_mass_g_min, -25);
  assert.equal(report.measurements.hx.recalculated_mass_g_max, -8);
  assert.equal(report.measurements.hx.recorded_mass_g_static_min, null);
  assertOfflineOnly(report);
});

test('CRLF input is hashed verbatim and cannot reuse a profile bound to LF text', () => {
  const lf = syntheticDump();
  const crlf = lf.replace(/\n/g, '\r\n');
  const report = analyze(crlf, { calibrationProfile: profileFor(crlf) });
  assert.equal(report.source_sha256, sha256(crlf));
  assert.notEqual(report.source_sha256, sha256(lf));
  assert.equal(report.integrity.complete, true);
  assert.throws(() => analyze(crlf, { calibrationProfile: profileFor(lf) }), Error);
});

test('external profile requires every provenance and calibration field', () => {
  const input = syntheticDump();
  for (const field of ['profile_id', 'offset_counts', 'counts_per_g', 'source_kind', 'input_sha256']) {
    const profile = profileFor(input);
    delete profile[field];
    assert.throws(() => analyze(input, { calibrationProfile: profile }), Error, `Missing ${field}`);
  }
});

test('an explicitly supplied malformed profile cannot silently fall back to no calibration', () => {
  const input = syntheticDump();
  for (const calibrationProfile of [null, false, 0, '', true, [], {}]) {
    assert.throws(() => analyze(input, { calibrationProfile }), Error,
      `Invalid explicit profile: ${JSON.stringify(calibrationProfile)}`);
  }
});

test('external profile rejects non-finite values, zero scale and provenance mismatches', () => {
  const input = syntheticDump();
  for (const overrides of [
    { offset_counts: NaN }, { offset_counts: Infinity }, { offset_counts: null },
    { counts_per_g: NaN }, { counts_per_g: -Infinity }, { counts_per_g: 0 }, { counts_per_g: '2' },
    { source_kind: 'historical' }, { input_sha256: '0'.repeat(64) }, { input_sha256: 'invalid' },
  ]) {
    assert.throws(() => analyze(input, { calibrationProfile: profileFor(input, overrides) }), Error);
  }
});

test('CLI stdin: complete SYNTHETIC export returns report and exit 0', () => {
  const input = syntheticDump();
  const result = cli(['--stdin', '--source-kind', 'synthetic'], input);
  assert.equal(result.status, 0, result.stderr);
  const report = JSON.parse(result.stdout);
  assert.equal(report.source_sha256, sha256(input));
  assert.equal(report.integrity.complete, true);
  assertOfflineOnly(report);
});

test('CLI stdin: malformed export returns JSON report and exit 2', () => {
  const result = cli(['--stdin', '--source-kind', 'synthetic'], syntheticDump().replace(END + '\n', ''));
  assert.equal(result.status, 2, result.stderr);
  const report = JSON.parse(result.stdout);
  assert.equal(report.integrity.complete, false);
  assertIssue(report, 'error');
  assertOfflineOnly(report);
});

test('CLI stdin: missing expected counts returns report and exit 2', () => {
  const result = cli(['--stdin', '--source-kind', 'synthetic'], syntheticDump({ trial: null }));
  assert.equal(result.status, 2, result.stderr);
  const report = JSON.parse(result.stdout);
  assert.equal(report.integrity.export_count_verified, false);
  assert.equal(report.integrity.complete, false);
  assertIssue(report, 'warning');
});

test('CLI configuration errors return exit 1', () => {
  for (const args of [
    ['--stdin'],
    ['--stdin', '--source-kind', 'unknown'],
    ['--stdin', '--input', __filename, '--source-kind', 'synthetic'],
    ['--stdin', '--source-kind', 'synthetic', '--unknown-option'],
  ]) {
    const result = cli(args, syntheticDump());
    assert.equal(result.status, 1, `Configuration arguments: ${args.join(' ')}\n${result.stderr}`);
  }
});

test('CLI help does not require input or a source kind', () => {
  const result = cli(['--help']);
  assert.equal(result.status, 0, result.stderr);
  assert.ok(result.stdout.trim().length > 0);
});

test('metadata comments retain printed calibration and NEAR_RANGE without applying them', () => {
  const input = syntheticDump().replace(HEADER, '# ACC_PEAK magnitude_g=0.980000 axis_abs_g=0.980000 includes_gravity=1 NEAR_RANGE=1 window=whole_ARM_trial\n' + HEADER);
  const report = analyze(input);
  assert.ok(report.recorded_metadata.some(row => row.key_values.NEAR_RANGE === '1'));
  assert.ok(report.recorded_metadata.some(row => row.key_values.offset === '0.000000'));
  assert.equal(report.calibration.mode, 'none');
  assert.equal(report.measurements.hx.recalculated_mass_g_min, null);
  assertOfflineOnly(report);
});

test('CLI regular-file input/profile/output preserves input and refuses overwrite', t => {
  const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'irt-preliminary-dataflow-'));
  const inputPath = path.join(directory, 'synthetic-input.log');
  const profilePath = path.join(directory, 'synthetic-profile.json');
  const outputPath = path.join(directory, 'report.json');
  const existingPath = path.join(directory, 'existing-report.json');
  t.after(() => {
    // Only these explicitly created files are removed; no recursive cleanup.
    for (const file of [inputPath, profilePath, outputPath, existingPath]) if (fs.existsSync(file)) fs.unlinkSync(file);
    fs.rmdirSync(directory);
  });
  const input = syntheticDump();
  const profile = JSON.stringify(profileFor(input));
  const sentinel = 'SYNTHETIC existing output must remain unchanged\n';
  fs.writeFileSync(inputPath, input);
  fs.writeFileSync(profilePath, profile);
  fs.writeFileSync(existingPath, sentinel);
  const result = cli(['--input', inputPath, '--source-kind', 'synthetic', '--calibration', profilePath, '--output', outputPath]);
  assert.equal(result.status, 0, result.stderr);
  const report = JSON.parse(fs.readFileSync(outputPath, 'utf8'));
  assert.equal(report.calibration.mode, 'external_unvalidated');
  assert.equal(report.source_sha256, sha256(input));
  assertOfflineOnly(report);
  for (const target of [existingPath, inputPath]) {
    const refused = cli(['--input', inputPath, '--source-kind', 'synthetic', '--output', target]);
    assert.equal(refused.status, 1, refused.stderr);
  }
  assert.equal(fs.readFileSync(inputPath, 'utf8'), input);
  assert.equal(fs.readFileSync(profilePath, 'utf8'), profile);
  assert.equal(fs.readFileSync(existingPath, 'utf8'), sentinel);
  const directoryInput = cli(['--input', directory, '--source-kind', 'synthetic']);
  assert.equal(directoryInput.status, 1);
});
