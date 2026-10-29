// Actual public C++ exporters run under three allocator failures, never device data.
'use strict';
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const {analyzeCapture}=require('../tools/hx-capture-diagnostic.cjs');
const {parseHxDiagnostics}=require('../tools/hx-read-diagnostic.cjs');
const {hash}=require('./prepare-hx-attempt-native-export.cjs');
const text=fs.readFileSync(path.join(__dirname,'fixtures/hx-attempt-degraded-native-synthetic.log'),'utf8');
let n=0;function test(name,f){f();console.log(`PASS ${++n}: ${name}`);}
test('實際C++三種配置失敗都保留完整RAW bundle並明示逐框診斷不完整',()=>{
  assert.equal((text.match(/SYNTHETIC_NATIVE_FRAME_EXPORT no_device=1 no_adc=1/g)||[]).length,3);
  assert.equal((text.match(/export_functions_sha256=[A-F0-9]{64}/g)||[]).length,3); // Historical PCNT2m1 exporter fixture; current RMT exporter tested separately.
  const captures=analyzeCapture(text);assert.equal(captures.length,3);
  for(const [i,c] of captures.entries()){
    assert.equal(c.schema,3);assert.equal(c.bundle_complete,true);assert.equal(c.attempt_pcnt_available,false);assert.equal(c.attempt_pcnt_allocation_state,i+2);assert.equal(c.requested_frame_diagnostic_complete,false);
    assert.equal(c.read.rows,256);assert.equal(c.link.rows,256);assert.equal(c.failure.failure_seq,250);assert.equal(c.failure.profile,'SCK_LOW_250MS_PCNT');assert.equal(c.pcnt.proof_count,0);assert.equal(c.attempt_any,undefined);assert.equal(c.attempt_link,undefined);
  }
  const reads=parseHxDiagnostics(text);assert.equal(reads.length,6);for(let i=2;i<6;i+=2){assert.deepEqual(reads[0].rows,reads[i].rows);assert.deepEqual(reads[1].rows,reads[i+1].rows);}
});
test('降級profile需要真正失敗狀態與sidecar schema0，不能默默冒稱配置成功',()=>{
  for(const replacement of ['allocation_state=0','allocation_state=1','allocation_state=5'])assert.throws(()=>analyzeCapture(text.replace('attempt_pcnt_schema=0 allocation_state=2','attempt_pcnt_schema=0 '+replacement)),/allocation identity/);
  assert.throws(()=>analyzeCapture(text.replace('attempt_pcnt_schema=0','attempt_pcnt_schema=1')),/allocation identity/);
  assert.throws(()=>analyzeCapture(text.replace('schema=3 version=','schema=4 version=')),/identity/);
  assert.doesNotMatch(text,/# BEGIN_HX_ATTEMPT_PCNT/);
});
console.log(`${n} allocator-failure actual-export cases passed; synthetic only.`);
