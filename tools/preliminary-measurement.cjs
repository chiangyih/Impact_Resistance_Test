#!/usr/bin/env node
'use strict';

// 離線讀取第五版DUMP；沒有串列埠、重置、燒錄或裝置命令路徑。
const fs = require('node:fs');
const crypto = require('node:crypto');

const HEADER = 'kind,t_read_us,x_raw,y_raw,z_raw,x_g,y_g,z_g,mag_g,hx_raw,hx_status,mass_g_static';
const SOURCE_KINDS = new Set(['synthetic', 'historical', 'new']);
const RAILS = new Set([-8388608, 8388607]);

function keyValues(line) {
  return Object.fromEntries([...line.matchAll(/([A-Za-z_][A-Za-z0-9_]*)=([^\s;]+)/g)].map(m => [m[1], m[2]]));
}

function finite(value, label, integer = false) {
  if (!/^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?$/.test(value)) throw new Error(`${label}不是有限數值`);
  const number = Number(value);
  if (!Number.isFinite(number) || (integer && !Number.isSafeInteger(number))) throw new Error(`${label}格式或範圍不正確`);
  return number;
}

function validateProfile(profile, sourceKind, hash) {
  if (profile === undefined) return null;
  if (!profile || typeof profile !== 'object' || Array.isArray(profile) || typeof profile.profile_id !== 'string' || !profile.profile_id.trim()) {
    throw new Error('校正profile必須有非空白profile_id');
  }
  if (!Number.isFinite(profile.offset_counts) || !Number.isFinite(profile.counts_per_g) || profile.counts_per_g === 0) {
    throw new Error('校正profile的offset_counts須有限，counts_per_g須有限且非零');
  }
  if (profile.source_kind !== sourceKind) throw new Error('校正profile來源類型與輸入不符');
  if (typeof profile.input_sha256 !== 'string' || profile.input_sha256.toLowerCase() !== hash) {
    throw new Error('校正profile的input_sha256未綁定本次完整輸入');
  }
  return profile;
}

function range(values) {
  if (!values.length) return {min: null, max: null};
  return values.reduce((result, value) => ({min: Math.min(result.min, value), max: Math.max(result.max, value)}), {min: Infinity, max: -Infinity});
}

function filePathOnly(filePath, mustExist = false) {
  // Windows裝置名稱與裝置命名空間不得成為一般檔案輸入／輸出。
  if (/^(?:\\\\[.?]\\|\/\/[.?]\/)/.test(filePath) || /(?:^|[\\/])(?:CON|PRN|AUX|NUL|COM\d+|LPT\d+)(?:[ .:][^\\/]*)?(?:[\\/]|$)/i.test(filePath)) {
    throw new Error('只接受一般檔案路徑，不接受裝置或命名管線');
  }
  if (mustExist && !fs.statSync(filePath).isFile()) throw new Error('輸入及校正profile必須是已存在的一般檔案');
  return filePath;
}

function analyzeDump(text, options = {}) {
  if (typeof text !== 'string') throw new Error('輸入必須為UTF-8文字');
  if (!SOURCE_KINDS.has(options.sourceKind)) throw new Error('必須明確指定sourceKind：synthetic、historical或new');
  const hash = crypto.createHash('sha256').update(text, 'utf8').digest('hex');
  const profile = validateProfile(options.calibrationProfile, options.sourceKind, hash);
  const issues = [];
  const issue = (code, severity, message, line) => issues.push({code, severity, message, ...(line ? {line} : {})});
  const acc = [], hx = [], summaries = [], recordedMetadata = [];
  const observed = {acc: 0, hx: 0};
  const lastTime = {ACC: null, HX: null};
  let begin = null, beginCount = 0, endCount = 0, headerCount = 0, ended = false;
  let qualitySeen = false;
  const quality = {scope: 'frozen_trial_delta', hx_errors: null, acc_errors: null, fifo_full: null, sat: null, hx_stale: null, acc_stale: null, ignored_edges: null};

  for (const [index, original] of text.replace(/^\uFEFF/, '').split(/\r?\n/).entries()) {
    const line = original.trim();
    const lineNumber = index + 1;
    if (!line) continue;
    if (line.startsWith('#')) recordedMetadata.push({source_line: lineNumber, text: original, key_values: keyValues(line)});
    const trialSummary = /^# TRIAL\s+(\d+)\s+(\S+)\s+(.+)$/.exec(line);
    if (trialSummary) {
      summaries.push({id: trialSummary[1], state: trialSummary[2], values: keyValues(trialSummary[3]), line: lineNumber});
      continue;
    }
    if (/^# BEGIN\b/.test(line)) {
      beginCount++;
      if (beginCount > 1) issue('MULTIPLE_TRIALS', 'error', '一次只接受一份DUMP，不合併多輪資料', lineNumber);
      if (!begin) {
        begin = keyValues(line);
        if (!/^\d+$/.test(begin.trial || '') || !['DONE', 'FAULT'].includes(begin.state) || !begin.reason || !begin.CAL) {
          issue('INVALID_BEGIN', 'error', 'BEGIN缺少有效trial、DONE/FAULT、reason或CAL', lineNumber);
        }
      }
      if (ended) issue('BEGIN_AFTER_END', 'error', 'END之後又出現BEGIN', lineNumber);
      continue;
    }
    if (/^# END\b/.test(line)) {
      endCount++;
      if (!begin || !headerCount) issue('UNEXPECTED_END', 'error', 'END之前沒有BEGIN及CSV表頭', lineNumber);
      if (endCount > 1) issue('DUPLICATE_END', 'error', '重複END', lineNumber);
      ended = true;
      continue;
    }
    if (line === HEADER) {
      headerCount++;
      if (!begin || ended || headerCount > 1) issue('INVALID_HEADER_POSITION', 'error', 'CSV表頭缺少BEGIN、重複或位於END之後', lineNumber);
      continue;
    }
    if (line.startsWith('#')) {
      if (/^# tare_valid=/.test(line) && begin && !ended) {
        if (qualitySeen) issue('DUPLICATE_QUALITY', 'error', '重複校正／品質中繼資料', lineNumber);
        qualitySeen = true;
        const values = keyValues(line);
        const mapping = {hx_errors: 'HX_ERR', acc_errors: 'ACC_ERR', fifo_full: 'FIFO_FULL', sat: 'SAT', hx_stale: 'HX_STALE', acc_stale: 'ACC_STALE', ignored_edges: 'ignored_edges'};
        for (const [name, key] of Object.entries(mapping)) {
          if (!(key in values)) continue;
          try {
            const value = finite(values[key], key, true);
            if (value < 0 || (['hx_stale', 'acc_stale'].includes(name) && value > 1)) throw new Error(`${key}超出範圍`);
            quality[name] = value;
          } catch (error) { issue('INVALID_QUALITY', 'error', error.message, lineNumber); }
        }
      }
      continue;
    }
    if (!begin || !headerCount || ended) {
      issue('DATA_OUTSIDE_EXPORT', 'error', '資料不在BEGIN、表頭與END之間；STATUS不是DUMP', lineNumber);
      continue;
    }
    const cells = line.split(',').map(value => value.trim());
    const kind = cells[0];
    if (kind === 'ACC') observed.acc++;
    if (kind === 'HX') observed.hx++;
    try {
      if (cells.length !== 12 || !['ACC', 'HX'].includes(kind)) throw new Error('列型別或12欄CSV格式不符');
      const time = finite(cells[1], 't_read_us', true);
      if (time < 0) throw new Error('t_read_us不可為負數');
      if (lastTime[kind] !== null && time < lastTime[kind]) throw new Error('同一感測器的讀出時間倒序');
      lastTime[kind] = time;
      if (kind === 'ACC') {
        if (!cells.slice(9).every(value => value === 'NA')) throw new Error('ACC列混入HX欄位');
        const raw = cells.slice(2, 5).map(value => finite(value, 'ACC RAW', true));
        if (raw.some(value => value < -32768 || value > 32767)) throw new Error('ACC RAW超出int16範圍');
        const g = cells.slice(5, 9).map(value => finite(value, 'ACC g'));
        if (g[3] < 0) throw new Error('合成幅度不可為負數');
        acc.push({t_read_us: time, x_raw: raw[0], y_raw: raw[1], z_raw: raw[2], x_g: g[0], y_g: g[1], z_g: g[2], mag_g: g[3], source_line: lineNumber});
      } else {
        if (!cells.slice(2, 9).every(value => value === 'NA')) throw new Error('HX列混入ACC欄位');
        const raw = finite(cells[9], 'HX RAW', true);
        if (raw < -8388608 || raw > 8388607) throw new Error('HX RAW超出signed24範圍');
        if (!['OK', 'SAT'].includes(cells[10])) throw new Error('HX狀態須為OK或SAT');
        const saturated = cells[10] === 'SAT' || RAILS.has(raw);
        if (RAILS.has(raw) && cells[10] !== 'SAT') issue('HX_STATUS_MISMATCH', 'error', 'HX滿量程RAW未標SAT', lineNumber);
        const recordedMass = cells[11] === 'NA' ? null : finite(cells[11], 'mass_g_static');
        if (saturated && recordedMass !== null) issue('SAT_MASS_PRESENT', 'error', 'SAT列的重量應為NA', lineNumber);
        if (begin.CAL === 'UNCAL' && recordedMass !== null) issue('UNCAL_MASS_PRESENT', 'error', 'UNCAL列的重量應為NA', lineNumber);
        const recalculated = profile && !saturated ? (raw - profile.offset_counts) / profile.counts_per_g : null;
        if (recalculated !== null && !Number.isFinite(recalculated)) throw new Error('校正重算溢位');
        hx.push({t_read_us: time, hx_raw: raw, hx_status: cells[10], saturated, recorded_mass_g_static: recordedMass, recalculated_mass_g: recalculated, source_line: lineNumber});
      }
    } catch (error) { issue('INVALID_ROW', 'error', error.message, lineNumber); }
  }

  if (!beginCount) issue('MISSING_BEGIN', 'error', '缺少BEGIN，靜態STATUS不能作為試驗DUMP');
  if (!headerCount) issue('MISSING_HEADER', 'error', '缺少完整CSV表頭');
  if (!endCount) issue('MISSING_END', 'error', '缺少END，可能截斷');
  const expected = {acc: null, hx: null};
  const matches = summaries.filter(summary => begin && summary.id === begin.trial);
  if (matches.length > 1) issue('DUPLICATE_TRIAL_SUMMARY', 'error', '同trial有多個TRIAL摘要，預期筆數不明');
  if (!matches.length) {
    issue(summaries.length ? 'TRIAL_ID_MISMATCH' : 'MISSING_EXPECTED_COUNTS', summaries.length ? 'error' : 'warning', '沒有相符TRIAL的A/H預期筆數，不能確認匯出筆數完整');
  } else {
    const summary = matches[0];
    if (summary.state !== begin.state || summary.values.reason !== begin.reason) issue('TRIAL_METADATA_MISMATCH', 'error', 'TRIAL與BEGIN的狀態／原因不符');
    for (const [name, key] of [['acc', 'A'], ['hx', 'H']]) {
      try {
        expected[name] = finite(summary.values[key], key, true);
        if (expected[name] < 0) throw new Error('預期筆數不可為負數');
        if (expected[name] !== observed[name]) issue('COUNT_MISMATCH', 'error', `${name}預期${expected[name]}筆，觀察到${observed[name]}筆`);
      } catch (error) { issue('INVALID_EXPECTED_COUNTS', 'error', error.message); }
    }
  }
  if (!qualitySeen) issue('QUALITY_UNKNOWN', 'warning', '缺少品質中繼資料，品質計數保持null而不是0');
  if (Object.values(quality).some(value => typeof value === 'number' && value > 0)) issue('QUALITY_FLAGS_PRESENT', 'warning', '原始品質旗標非零；須逐指標評估，不刪除故障試驗');
  if (begin && begin.state === 'FAULT') issue('TRIAL_FAULT', 'warning', '原試驗為FAULT，匯出可完整但不代表試驗有效');
  if (hx.some(row => row.saturated)) issue('HX_SAT_PRESENT', 'warning', '存在HX飽和框，保留RAW但不拿來作有效極值或重量');
  issue('READOUT_TIMESTAMPS', 'info', '時間是ESP32相對ARM讀出時間；感測器分組，未重建精準採樣或接觸事件時間');
  const rawRange = range(hx.filter(row => !row.saturated).map(row => row.hx_raw));
  const recorded = range(hx.filter(row => !row.saturated && row.recorded_mass_g_static !== null).map(row => row.recorded_mass_g_static));
  const recalculated = range(hx.filter(row => row.recalculated_mass_g !== null).map(row => row.recalculated_mass_g));
  const structureValid = !issues.some(item => item.severity === 'error');
  const countsVerified = matches.length === 1 && expected.acc !== null && expected.hx !== null && expected.acc >= 0 && expected.hx >= 0 && expected.acc === observed.acc && expected.hx === observed.hx;
  return {
    schema_version: 1, source_kind: options.sourceKind, source_sha256: hash,
    trial: {id: begin?.trial ?? null, state: begin?.state ?? null, reason: begin?.reason ?? null, recorded_calibration: begin?.CAL ?? null},
    integrity: {structure_valid: structureValid, export_count_verified: countsVerified, complete: structureValid && countsVerified, expected, observed, issues},
    calibration: {mode: profile ? 'external_unvalidated' : 'none', profile_id: profile?.profile_id ?? null},
    quality,
    recorded_metadata: recordedMetadata,
    measurements: {
      acc: {count: observed.acc, valid_rows: acc.length, sampled_magnitude_peak_g: range(acc.map(row => row.mag_g)).max, sampled_axis_abs_peak_g: range(acc.map(row => Math.max(Math.abs(row.x_g), Math.abs(row.y_g), Math.abs(row.z_g)))).max},
      hx: {count: observed.hx, valid_rows: hx.length, non_sat_count: hx.filter(row => !row.saturated).length, raw_min: rawRange.min, raw_max: rawRange.max, recorded_mass_g_static_min: recorded.min, recorded_mass_g_static_max: recorded.max, recalculated_mass_g_min: recalculated.min, recalculated_mass_g_max: recalculated.max},
    },
    records: {acc, hx},
    validation: {stage: 'offline-dataflow-only', hardware_verified: false, measurement_accepted: false, material_comparison_accepted: false},
    limitations: ['sampled峰值僅限所保存的整段ARM樣本，不是已驗證撞擊主峰或真實峰值力', 'recorded_mass僅保留原輸出；外部重算也未代表校正或量測驗收合格', '筆數核對只檢查匯出，不保證ADC零漏樣'],
  };
}

function main(args) {
  const help = '離線第五版DUMP檢查（不開埠）\nnode tools/preliminary-measurement.cjs (--input FILE | --stdin) --source-kind synthetic|historical|new [--calibration PROFILE.json] [--output REPORT.json]\n預設輸出JSON至stdout；output不得覆寫既有檔。退出0=匯出結構/筆數符合，2=不完整/資料錯誤，1=參數或檔案錯誤；均不代表量測驗收。';
  if (args.includes('--help') || args.includes('-h')) { process.stdout.write(help + '\n'); return; }
  const options = {};
  for (let index = 0; index < args.length; index++) {
    const name = args[index];
    if (name === '--stdin') { if (options.stdin) throw new Error('重複--stdin'); options.stdin = true; continue; }
    if (!['--input', '--source-kind', '--calibration', '--output'].includes(name) || index + 1 >= args.length || args[index + 1].startsWith('--')) throw new Error('無效參數；請用--help');
    if (name in options) throw new Error(`重複${name}`);
    options[name] = args[++index];
  }
  if (Boolean(options.stdin) === Boolean(options['--input'])) throw new Error('--input與--stdin必須擇一');
  if (!SOURCE_KINDS.has(options['--source-kind'])) throw new Error('必須明確指定--source-kind synthetic|historical|new');
  const text = fs.readFileSync(options.stdin ? 0 : filePathOnly(options['--input'], true), 'utf8');
  const profile = options['--calibration'] ? JSON.parse(fs.readFileSync(filePathOnly(options['--calibration'], true), 'utf8').replace(/^\uFEFF/, '')) : undefined;
  const result = analyzeDump(text, {sourceKind: options['--source-kind'], calibrationProfile: profile});
  if (options['--input']) result.input_path = options['--input'];
  const output = JSON.stringify(result, null, 2) + '\n';
  if (options['--output']) fs.writeFileSync(filePathOnly(options['--output']), output, {encoding: 'utf8', flag: 'wx'});
  else process.stdout.write(output);
  if (!result.integrity.complete) process.exitCode = 2;
}

if (require.main === module) {
  try { main(process.argv.slice(2)); }
  catch (error) { process.stderr.write(`ERROR: ${error.message}\n`); process.exitCode = 1; }
}

module.exports = {analyzeDump};
