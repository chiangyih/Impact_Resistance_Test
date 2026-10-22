'use strict';

// Offline recalculation only. Emit an apply_patch patch, or verify saved outputs.
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const assert = require('node:assert/strict');
const root = __dirname;
const read = name => fs.readFileSync(path.join(root, name), 'utf8').replace(/^\uFEFF/, '');
const hash = name => crypto.createHash('sha256').update(fs.readFileSync(path.join(root, name))).digest('hex');
const sourceHashes = {
  '2026-10-21-hx711-status.csv': '226f3c42a4cb6a8a3361c6ec57b9a16e2e757d810e15a04d5183614ba4a4e636',
  '2026-10-21-hx711-source.log': '963138d78d79b95c4e5d11976f8b48827c0ee15a3eb7bf177362a22b185f87c1',
  '2026-10-21-hx711-fifo-diag1-source.log': 'f169865bcbfd58da29cac4db6a7ab34d9edc7a748ee1404b45d163d6104cf4e2',
};
for (const [name, expected] of Object.entries(sourceHashes)) assert.equal(hash(name), expected, `Source changed: ${name}`);

function parseCsv(text) {
  const rows = [];
  let row = [], cell = '', quoted = false;
  for (let i = 0; i < text.length; i++) {
    const c = text[i];
    if (c === '"') {
      if (quoted && text[i + 1] === '"') { cell += '"'; i++; }
      else quoted = !quoted;
    } else if (c === ',' && !quoted) { row.push(cell); cell = ''; }
    else if (c === '\n' && !quoted) { row.push(cell.replace(/\r$/, '')); rows.push(row); row = []; cell = ''; }
    else cell += c;
  }
  assert.equal(quoted, false, 'Unclosed CSV quote');
  if (row.length || cell) { row.push(cell); rows.push(row); }
  const headers = rows.shift();
  return rows.map(values => {
    assert.equal(values.length, headers.length, 'CSV column count');
    return Object.fromEntries(headers.map((key, i) => [key, values[i]]));
  });
}
function csv(rows) {
  const headers = Object.keys(rows[0]);
  const escape = value => /[",\r\n]/.test(String(value)) ? '"' + String(value).replace(/"/g, '""') + '"' : String(value);
  return '\uFEFF' + [headers, ...rows.map(row => headers.map(key => row[key] ?? ''))].map(row => row.map(escape).join(',')).join('\n') + '\n';
}
const close = (a, b) => assert.ok(Math.abs(a - b) < 1e-9, `${a} != ${b}`);
function correctionFields(expected, coefficient, mass) {
  return { archived_expected_added_mass_g: expected, archived_counts_per_g: coefficient,
    archived_derived_mass_g: mass, entered_reference_mass_g: 400, corrected_reference_mass_g: 200,
    correction_date_local: '2026-10-22' };
}

const oldSource = parseCsv(read('2026-10-21-hx711-status.csv'));
const oldRows = oldSource.map(source => {
  const row = { ...source, ...correctionFields(source.expected_added_mass_g, source.counts_per_g, source.derived_mass_g) };
  if (row.expected_added_mass_g === '400') row.expected_added_mass_g = '200';
  if (row.cal_state === 'CAL_USER') {
    close(Number(source.derived_mass_g), (Number(source.raw_counts) - Number(source.tare_offset_counts)) / Number(source.counts_per_g));
    row.counts_per_g = Number(source.counts_per_g) * 2;
    row.derived_mass_g = (Number(row.raw_counts) - Number(row.tare_offset_counts)) / row.counts_per_g;
    close(row.derived_mass_g, Number(source.derived_mass_g) / 2);
  }
  return row;
});
assert.equal(oldRows.length, 200);
assert.equal(oldRows.filter(r => r.derived_mass_g !== '').length, 176);
assert.equal(oldRows.filter(r => r.duplicate_rx_in_stage === '1').length, 4);
assert.equal(oldRows.filter(r => r.expected_added_mass_g === '200').length, 24);

const newLogName = '2026-10-21-hx711-fifo-diag1-source.log';
const lines = read(newLogName).split(/\r?\n/);
const newRows = [], photos = [], seen = new Map(), observations = new Map();
let stage = 'precheck', stageOrder = 1, offset = '', calibrated = false;
for (const [index, line] of lines.entries()) {
  if (line.startsWith('# Stage: TARE;')) { stage = 'tare_pre'; stageOrder = 2; }
  if (line.startsWith('# Stage: CAL 400;')) { stage = 'cal400_pre'; stageOrder = 3; }
  if (line.startsWith('# Stage: first unload')) { stage = 'unload1'; stageOrder = 4; }
  if (line.startsWith('# Stage: repeated 400g')) { stage = 'repeat400'; stageOrder = 5; }
  if (line.startsWith('# Stage: second unload')) { stage = 'unload2'; stageOrder = 6; }
  if (line.startsWith('# Stage: extended empty-tray')) { stage = 'extended_unloaded'; stageOrder = 7; }
  if (line.startsWith('# TARE_OK ')) { offset = Number(line.match(/offset=([-\d.]+)/)[1]); stage = 'tare_post'; }
  if (line.startsWith('# CAL_USER counts_per_g=')) {
    assert.equal(Number(line.match(/counts_per_g=([\d.]+)/)[1]), 10.234609375);
    calibrated = true; stage = 'cal400_post';
  }
  if (line.includes('# STATUS ')) {
    const fields = Object.fromEntries([...line.matchAll(/\b([A-Za-z_]+)=([^\s]+)/g)].map(m => [m[1], m[2]]));
    const n = (observations.get(stage) || 0) + 1;
    observations.set(stage, n);
    const rxKey = `${stage}:${fields.RX}`, duplicate = seen.has(rxKey);
    if (duplicate) assert.equal(seen.get(rxKey), fields.RAW, 'Same RX, changed RAW');
    seen.set(rxKey, fields.RAW);
    assert.equal(fields.CAL === 'CAL_USER', calibrated);
    const expected = stage === 'precheck' ? '' : stage.startsWith('cal400') || stage === 'repeat400' ? 200 : 0;
    const oldMass = calibrated ? (Number(fields.RAW) - offset) / 10.234609375 : '';
    newRows.push({ observation_id: newRows.length + 1, session_date_local: '2026-10-21', stage_order: stageOrder,
      stage_id: stage, stage_observation: n, expected_added_mass_g: expected,
      raw_counts: fields.RAW, rx_count: fields.RX, hx_error_count: fields.ERR, hx_status: fields.HX,
      cal_state: fields.CAL, tare_offset_counts: offset, counts_per_g: calibrated ? 20.46921875 : '',
      derived_mass_g: calibrated ? (Number(fields.RAW) - offset) / 20.46921875 : '',
      duplicate_rx_in_stage: Number(duplicate), host_elapsed_ms: fields.elapsed_ms ?? '',
      phase: fields.phase, reason: fields.reason, age_ms: fields.AGE_ms, adxl_status: fields.ADXL,
      adxl_error_count: fields.AERR, fifo_full_count: fields.FIFO_FULL, firmware: 'v1-layout2-fifo-diag1',
      source_log: newLogName, source_line_number: index + 1, raw_status_line: line,
      ...correctionFields(expected === 200 ? 400 : expected, calibrated ? 10.234609375 : '', oldMass) });
  }
  if (line.startsWith('# PHOTO_OBSERVATION ')) {
    const fields = Object.fromEntries([...line.matchAll(/\b([A-Za-z_]+)=([^\s]+)/g)].map(m => [m[1], m[2]]));
    const original = (Number(fields.RAW) + 164024.688) / 10.234609375;
    close(Math.round(original * 1e4) / 1e4, Number(fields.grams_approx));
    photos.push({ observation_id: photos.length + 1, source_kind: 'PHOTO_OBSERVATION', photo_file: fields.file,
      approx_elapsed_s: photos.length * 10, expected_added_mass_g: 0, raw_counts: fields.RAW,
      tare_offset_counts: -164024.688, counts_per_g: 20.46921875,
      derived_mass_g: (Number(fields.RAW) + 164024.688) / 20.46921875,
      cal_visible: fields.CAL_visible, phase: fields.PHASE, hx_status: fields.HX,
      source_log: newLogName, source_line_number: index + 1, raw_observation_line: line,
      ...correctionFields(0, 10.234609375, original) });
  }
}
assert.equal(newRows.length, 134);
assert.equal(newRows.filter(r => r.derived_mass_g !== '').length, 115);
assert.equal(newRows.filter(r => r.duplicate_rx_in_stage).length, 3);
assert.equal(photos.length, 7);
assert.deepEqual([...observations.values()], [7, 4, 4, 4, 5, 16, 16, 17, 61]);
for (const row of [...newRows, ...photos]) if (row.derived_mass_g !== '') close(row.derived_mass_g, row.archived_derived_mass_g / 2);

function stats(rows) {
  const values = rows.map(r => Number(r.derived_mass_g));
  const original = rows.map(r => Number(r.archived_derived_mass_g));
  return { n: rows.length, mean: values.reduce((a, b) => a + b, 0) / rows.length,
    oldMean: original.reduce((a, b) => a + b, 0) / rows.length,
    min: Math.min(...values), max: Math.max(...values), pp: Math.max(...values) - Math.min(...values) };
}
function select(rows, stage, deduplicate = false) {
  return rows.filter(r => r.stage_id === stage && (!deduplicate || Number(r.duplicate_rx_in_stage) === 0));
}
const fmt = n => n.toFixed(4);
const oldStages = [['cal400_post', '校正後保留砝碼（含重複快照）'], ['unload1', '第一次卸載'],
  ['unload1_settle', '第一次卸載後追加空載'], ['repeat400', '200g重放'], ['unload2', '第二次卸載'],
  ['unloaded_drift', '45秒空載（排除同RX重複）', true], ['version_check', '空載版本查詢'], ['clearance_followup', '5mm間隙確認後45秒空載']];
const newStages = [['cal400_post', '校正後保留砝碼（含重複快照）'], ['unload1', '第一次卸載'],
  ['repeat400', '200g重放'], ['unload2', '第二次卸載（排除同RX重複）', true], ['extended_unloaded', '約60秒空載']];
const table = (rows, stages) => [
  '| 窗口 | 採用快照數 | 原換算平均(g) | 修正平均(g) | 修正範圍(g) | 修正峰對峰(g) |',
  '|---|---:|---:|---:|---|---:|',
  ...stages.map(([stage, label, dedup]) => {
    const s = stats(select(rows, stage, dedup));
    return `| ${label} | ${s.n} | ${fmt(s.oldMean)} | ${fmt(s.mean)} | ${fmt(s.min)}～${fmt(s.max)} | ${fmt(s.pp)} |`;
  })].join('\n');
const oldRepeat = stats(select(oldRows, 'repeat400')), newRepeat = stats(select(newRows, 'repeat400'));
const p = stats(photos), last5 = stats(photos.slice(2));
const extended = select(newRows, 'extended_unloaded');
const bins = [0, 20000, 40000].map(start => stats(extended.filter(r => Number(r.host_elapsed_ms) >= start && Number(r.host_elapsed_ms) < start + 20000)));
const x = extended.map(r => Number(r.host_elapsed_ms) / 60000), y = extended.map(r => r.derived_mass_g);
const xmean = x.reduce((a, b) => a + b) / x.length, ymean = y.reduce((a, b) => a + b) / y.length;
const slope = x.reduce((s, v, i) => s + (v - xmean) * (y[i] - ymean), 0) / x.reduce((s, v) => s + (v - xmean) ** 2, 0);
const r2 = 1 - y.reduce((s, v, i) => s + (v - (ymean + slope * (x[i] - xmean))) ** 2, 0) / y.reduce((s, v) => s + (v - ymean) ** 2, 0);
const unloadDifference = stats(select(newRows, 'unload2', true)).mean - stats(select(newRows, 'unload1')).mean;
const report = `# 2026-10-21 HX711量測重算：校正砝碼更正為200g

2026-10-22使用者更正：昨天使用的是20g砝碼10個，合計200g。昨天舊版與新版兩輪均沿用同一批砝碼，因此兩輪皆依200g修正。這是依使用者重量回報的離線重算，沒有另行秤量砝碼。

## 修正方法與適用範圍

實際執行的命令仍為\`CAL 400\`，原始成功回應照原文保存。校正公式為\`counts_per_g = (校正框平均RAW - offset) / 已知重量\`，故修正係數＝原係數×400/200＝原係數×2；\`修正克數 = (RAW - offset) / 修正係數 = 原換算克數 / 2\`。RAW、零點、RX、ERR、時間及照片判讀值均保持原紀錄。

| 校正輪次／版本 | offset列印值(counts) | 原係數(counts/g) | 修正係數(counts/g) |
|---|---:|---:|---:|
| 舊版v1-layout2 | -162747.625 | 9.977421875 | 19.95484375 |
| 新版v1-layout2-fifo-diag1 | -164024.688 | 10.234609375 | 20.46921875 |

所有依這兩輪係數換算的克數、絕對誤差、標準差、峰對峰及g/min斜率均減半；相對200g的百分比誤差及R²不變。這裡的g是荷重靜態等效公克；ADXL375加速度的g值、電壓、電阻、180kg銘牌與RAW計數不適用此倍率。新版offset只列印三位小數，重算仍是依列印值的近似值，不補造內部精度。

## 舊版200筆STATUS修正結果

${table(oldRows, oldStages)}

重放平均${fmt(oldRepeat.mean)}g，對200g偏差${fmt(oldRepeat.mean - 200)}g（${fmt((oldRepeat.mean / 200 - 1) * 100)}%）。原CAL後5筆保留重複回報；45秒unloaded_drift採45個不同RX，其餘同原紀錄。全部200筆（176筆可換算、24筆UNCAL）均保存於[修正CSV](2026-10-21-hx711-status-corrected-200g.csv)。

## 新版134筆STATUS修正結果

${table(newRows, newStages)}

重放平均${fmt(newRepeat.mean)}g，對200g偏差+${fmt(newRepeat.mean - 200)}g（+${fmt((newRepeat.mean / 200 - 1) * 100)}%）。兩次卸載平均差修正為+${fmt(unloadDifference)}g。約60秒空載三段（0～20、20～40、40～60秒）的採用快照數為21／20／20，修正均值依序為${bins.map(s => '+' + fmt(s.mean) + 'g').join('、')}；線性擬合斜率約+${fmt(slope)}g/min，R²約${fmt(r2)}，只描述該窗口，不能外推長期漂移率。

第二次卸載17筆中有1筆重複，主表採16個不同RX；若沿用全部17筆，修正平均為${fmt(stats(select(newRows, 'unload2')).mean)}g。CAL後5筆含1筆重複，若排除該筆則4個不同RX修正平均為${fmt(stats(select(newRows, 'cal400_post', true)).mean)}g。全部134筆（115筆可換算、19筆UNCAL）均保存於[新版修正CSV](2026-10-21-hx711-fifo-diag1-status-corrected-200g.csv)。

## 七張OLED空托盤照片修正結果

| 照片 | 約相對時間(s) | RAW(counts) | 原近似換算(g) | 修正近似換算(g) |
|---|---:|---:|---:|---:|
${photos.map(r => `| ${path.posix.basename(r.photo_file)} | ${r.approx_elapsed_s} | ${r.raw_counts} | ${fmt(r.archived_derived_mass_g)} | ${fmt(r.derived_mass_g)} |`).join('\n')}

七張修正平均+${fmt(p.mean)}g，範圍+${fmt(p.min)}～+${fmt(p.max)}g，峰對峰${fmt(p.pp)}g；最後五張平均+${fmt(last5.mean)}g、範圍+${fmt(last5.min)}～+${fmt(last5.max)}g。首末差${fmt(photos.at(-1).derived_mass_g - photos[0].derived_mass_g)}g，仍是先下降後波動。時間僅依使用者回報約10秒間隔，沒有精確拍攝時間、RX或ERR，不能判定獨立ADC樣本或熱穩定。逐張資料見[照片重算CSV](2026-10-21-hx711-oled-corrected-200g.csv)。

## 資料來源、追溯與核對

- 三份修正CSV均另存，原始200筆CSV及兩份log的內容與SHA256保持不變。原\`CAL 400\`命令、\`cal400_pre/post\`與\`repeat400\`階段識別保留供追溯，階段名中的400不再代表實際砝碼重量。
- 舊版CSV保留35個原欄位，僅更新\`expected_added_mass_g\`、\`counts_per_g\`、\`derived_mass_g\`；另加6欄保留原值、輸入400g、更正200g及更正日期。首次外加荷重未確認及UNCAL克數仍留空。
- 新版CSV由log逐行解析；\`source_line_number\`為完整新版log的1-based行號。舊版CSV該欄仍沿用原封存的工具輸出段內行號。照片CSV列\`source_kind=PHOTO_OBSERVATION\`，沒有補造RX／ERR。
- 驗證筆數200／134／7、可換算筆數176／115／7、STATUS重複回報4／3；逐筆核對修正公式及原換算值÷2。原始STATUS行、RAW、RX、ERR與缺值不變。
- 本次沒有操作COM3、修改或燒錄韌體、重置、重新TARE／CAL，亦沒有把修正係數寫入裝置。硬體排查維持暫停；最後已知RAM若仍保留，內部仍可能使用錯誤的400g比例，恢復時需先確認現況。
- 重算修正了重量尺度，空載RAW偏移、回零波動及串列無回應仍待排查，校正尚未驗證合格。小數位是計算輸出，並非量測精度認證。

| 原始檔案 | SHA256 |
|---|---|
${Object.entries(sourceHashes).map(([name, value]) => `| ${name} | \`${value}\` |`).join('\n')}

重算／核對工具為\`recalculate-200g.cjs\`：\`node docs/calibration/recalculate-200g.cjs --check\`可檢查保存結果，\`--patch\`僅輸出修正檔案的apply_patch內容，不操作裝置。
`;
const outputs = {
  '2026-10-21-hx711-status-corrected-200g.csv': csv(oldRows),
  '2026-10-21-hx711-fifo-diag1-status-corrected-200g.csv': csv(newRows),
  '2026-10-21-hx711-oled-corrected-200g.csv': csv(photos),
  '2026-10-21-hx711-correction-200g.md': report,
};
for (const [name, content] of Object.entries(outputs)) {
  if (name.endsWith('.csv')) assert.equal(parseCsv(content.replace(/^\uFEFF/, '')).length, name.includes('oled') ? 7 : name.includes('fifo') ? 134 : 200);
}
if (process.argv.includes('--patch')) {
  const patch = ['*** Begin Patch'];
  for (const [name, content] of Object.entries(outputs)) {
    assert.equal(fs.existsSync(path.join(root, name)), false, `Output already exists: ${name}; use --check`);
    patch.push(`*** Add File: docs/calibration/${name}`, ...content.trimEnd().split('\n').map(line => '+' + line));
  }
  patch.push('*** End Patch');
  process.stdout.write(patch.join('\n'));
} else if (process.argv.includes('--check')) {
  for (const [name, expected] of Object.entries(outputs)) {
    const actual = fs.readFileSync(path.join(root, name), 'utf8').replace(/\r\n/g, '\n');
    assert.equal(actual, expected, `Output mismatch: ${name}`);
  }
  console.log('PASS: 341 observations; 298 recalculated masses; duplicate flags and missing values preserved; 3 source SHA256 unchanged.');
} else {
  console.log(JSON.stringify({ oldRepeat, newRepeat, photos: p, newUnloaded: stats(extended), sourceHashes }, null, 2));
}
