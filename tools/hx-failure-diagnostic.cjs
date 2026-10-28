// Offline retained SCK-LOW profile validation. No COM, network, credentials or RAW repair.
'use strict';
const fs=require('node:fs');
const {WORDS,crc32,parseHxDiagnostics,summarize:readSummary}=require('./hx-read-diagnostic.cjs');
const pcnt=require('./hx-pcnt-diagnostic.cjs');
const TOOL_VERSION='hx-failure-diagnostic-v4.0.0';
const TARGETS=[2,10,50,200,1000,2000,5000,20000,50000,100000,150000,250000];
const COLUMNS=['kind','index','target_us','begin_us','end_us','dout','valid','missed'];
const META=['mode_id','stop_reason','captured','seq','read_start_us','read_end_us','probe_start_us','probe_end_us','resume_start_us','resume_seq','resume_valid','rx','err_before','err_after','bits24','raw','phase_before','cal_before','next'];
function need(ok,message){if(!ok)throw new Error(message);}
function fields(line){return Object.fromEntries([...line.matchAll(/([A-Za-z][A-Za-z0-9_]*)=([^\s;]+)/g)].map(x=>[x[1],x[2]]));}
function uint(text,key,max=0xffffffff){need(typeof text==='string'&&/^\d+$/.test(text)&&Number(text)<=max,`Missing/invalid ${key}`);return Number(text);}
function delta(a,b){return(a-b)>>>0;}
function canonical(c){return [...META.map(k=>c.values[k]>>>0),...c.points.flatMap(p=>[p.index,p.target_us,p.begin_us??0,p.end_us??0,p.dout??0,p.valid,p.missed])];}
function parseHxFailures(text){
  const result=[];let c=null;
  for(const line of text.split(/\r?\n/)){
    if(line.startsWith('# BEGIN_HX_FAIL ')){
      need(!c,'Nested HX_FAIL capture');const begin=fields(line);
      need(begin.schema==='1'&&['SCK_LOW_250MS','SCK_LOW_250MS_PCNT','SCK_LOW_250MS_FRAME_PCNT'].includes(begin.profile)&&/^[0-9A-Fa-f]{8}$/.test(begin.session||'')&&begin.version,'Unsupported HX_FAIL identity/profile');
      need(begin.points==='12'&&begin.crc==='CRC32_ISO_HDLC_META19_POINTS7_LE_U32','Unsupported HX_FAIL CRC profile');
      c={begin,status:null,meta:null,resume:null,points:[],columns:false,values:{}};
    }else if(c&&line.startsWith('# HX_FAIL '))c.status=fields(line);
    else if(c&&line.startsWith('# HX_FAIL_META '))c.meta=line.includes('HX_FAIL_META NA;')?null:fields(line);
    else if(c&&line.startsWith('# HX_FAIL_RESUME '))c.resume=fields(line);
    else if(c&&line.startsWith('# HX_FAIL_LIMIT '))c.limit=fields(line);
    else if(c&&line===COLUMNS.join(','))c.columns=true;
    else if(c&&line.startsWith('HXFP,')){
      const x=line.split(',');need(x.length===8,'Truncated HXFP row');const p={};
      ['index','target_us','valid','missed'].forEach(k=>p[k]=uint(x[COLUMNS.indexOf(k)],k));
      need(p.index===c.points.length&&p.index<12&&p.target_us===TARGETS[p.index],'Missing/reordered/profile-invalid HXFP');
      need(p.valid<=1&&p.missed<=1&&!(p.valid&&p.missed),'Invalid valid/missed flags');
      for(const k of ['begin_us','end_us','dout']){const v=x[COLUMNS.indexOf(k)];if(!p.valid){need(v==='NA','Invalid point must use NA');p[k]=null;}else p[k]=uint(v,k,k==='dout'?1:0xffffffff);}
      c.points.push(p);
    }else if(c&&line.startsWith('# END_HX_FAIL ')){
      const end=fields(line),s=c.status,v=c.values;need(s&&c.resume&&c.columns&&c.points.length===12&&end.points==='12','Incomplete HX_FAIL fields/points');
      need(s.schema==='1'&&s.session===c.begin.session&&s.profile===c.begin.profile,'Mixed HX_FAIL epoch/profile');
      need(end.retained==='1'&&end.read_only==='1'&&/^0x[0-9A-Fa-f]{8}$/.test(end.crc32||''),'Invalid HX_FAIL end');
      v.mode_id=uint(s.mode_id,'mode_id',5);need(v.mode_id>=2,'Active/unsealed HX_FAIL dump');
      need(['WAITING','HOLD','COMPLETE','SKIPPED_BUSY','CLOSED_NO_LINK','TRUNCATED'][v.mode_id]===s.mode,'HX_FAIL mode mismatch');
      for(const k of ['stop_reason','captured','next'])v[k]=uint(s[k],k,k==='stop_reason'?4:k==='captured'?1:12);
      need(s.points==='12'&&s.early_target_us==='2000'&&s.hold_target_us==='250000','HX_FAIL profile limits missing');
      for(const k of META)if(!(k in v))v[k]=0;
      if(v.captured){
        need(c.meta,'Missing failure metadata');
        for(const k of ['seq','read_start_us','read_end_us','probe_start_us','probe_end_us','rx','err_before','err_after','phase_before','cal_before'])v[k]=uint(c.meta[k],k,k==='phase_before'?5:k==='cal_before'?2:0xffffffff);
        need(/^0x[0-9a-fA-F]{6}$/.test(c.meta.bits24||'')&&/^-?\d+$/.test(c.meta.raw||''),'Invalid failure bits/raw');v.bits24=Number(c.meta.bits24);v.raw=Number(c.meta.raw);
        need(v.raw===v.bits24-((v.bits24&0x800000)?16777216:0)&&c.meta.postcheck==='0','Failure RAW/postcheck mismatch');
        need(delta(v.err_after,v.err_before)===1,'Failure ERR delta mismatch');
        need(v.mode_id!==4,'Closed/no-LINK capture has failure');
      }else need(v.mode_id===4&&!c.meta,'No-LINK capture must be closed and metadata NA');
      v.resume_valid=uint(c.resume.valid,'resume_valid',1);
      if(v.resume_valid){v.resume_start_us=uint(c.resume.start_us,'resume_start');v.resume_seq=uint(c.resume.seq,'resume_seq');need(v.mode_id===2||v.mode_id===5,'Unexpected resume state');need(delta(v.resume_seq,v.seq)>=1,'Resume is not a later attempt');}
      else need(c.resume.start_us==='NA'&&c.resume.seq==='NA','Invalid resume must use NA');
      const valid=c.points.filter(p=>p.valid),missed=c.points.filter(p=>p.missed);
      need(uint(s.valid,'valid',12)===valid.length&&uint(s.missed,'missed',12)===missed.length,'HX_FAIL point count mismatch');
      need(c.points.every((p,i)=>i<v.next?(p.valid||p.missed):(!p.valid&&!p.missed)),'HX_FAIL next/validity mismatch');
      for(const p of valid){need(v.captured,'GPIO value without LINK');need(delta(p.begin_us,v.read_end_us)>=p.target_us,'Sample before target');need(delta(p.end_us,p.begin_us)<0x80000000,'Reversed CPU read bracket');if(p.index<11)need(delta(p.begin_us,v.read_end_us)<TARGETS[p.index+1],'Late sample incorrectly bound to crossed target');}
      if(v.mode_id===2)need(v.next===12&&c.points[11].valid&&v.stop_reason===1,'Incomplete COMPLETE trace');
      if(v.mode_id===3)need(!valid.length&&!missed.length&&!v.next&&v.stop_reason===2,'Busy path sampled DATA');
      need(c.limit?.anchor==='original_postcheck_end_cpu_us'&&c.limit?.original_reject_retained==='1'&&c.limit?.discrete_points_only==='1','Missing HX_FAIL observation limits');
      need(crc32(canonical(c))===Number(end.crc32),'HX_FAIL CRC mismatch');
      result.push(c);c=null;
    }else if(c&&line.trim())throw new Error('Unexpected/interleaved HX_FAIL line');
  }
  need(!c,'Incomplete HX_FAIL block; END missing');need(result.length>0,'No complete HX_FAIL block');return result;
}
function summarize(c){const v=c.values,valid=c.points.filter(p=>p.valid),applied=!!v.captured&&(v.mode_id===2||v.mode_id===5);return {schema:1,analysis_tool_version:TOOL_VERSION,session:c.begin.session,version:c.begin.version,profile:c.begin.profile,mode:c.status.mode,captured:!!v.captured,failure_seq:v.captured?v.seq:null,failure_bits:v.captured?'0x'+v.bits24.toString(16).padStart(6,'0'):null,failure_raw:v.captured?v.raw:null,valid_points:valid.length,missing_points:c.points.filter(p=>p.missed).length,unsampled_points:c.points.filter(p=>!p.valid&&!p.missed).length,points:c.points.map(p=>({...p,elapsed_begin_us:p.valid?delta(p.begin_us,v.read_end_us):null,elapsed_end_us:p.valid?delta(p.end_us,v.read_end_us):null})),observed_high_points:valid.filter(p=>p.dout===1).map(p=>p.index),all_valid_discrete_points_low:valid.length>0&&valid.every(p=>p.dout===0),hold_elapsed_us:applied?delta(v.probe_end_us,v.read_end_us):null,resume_start_us:v.resume_valid?v.resume_start_us:null,resume_seq:v.resume_valid?v.resume_seq:null,continuous_low_proven:false,adc_ready_proven:false,profile_can_delay_next_attempt:true,low_hold_intervention_applied:applied,changed_next_attempt_timing:applied,crc_scope:'export_only',host_receive_utc_not_sample_time:true};}
function matchFailure(f,r){need(f.begin.session===r.meta.session&&f.begin.version===r.meta.version,'Mixed DUMP/HX_FAIL epoch');if(f.values.captured){const x=r.rows.find(x=>x.seq===f.values.seq);need(x&&!x.accepted&&x.bits24===f.values.bits24&&x.raw===f.values.raw&&x.start_us===f.values.read_start_us&&x.end_us===f.values.read_end_us&&x.rx_before===f.values.rx&&x.rx_after===f.values.rx&&x.err_before===f.values.err_before&&x.err_after===f.values.err_after,'HX_FAIL does not match retained rejection');}}
function analyzeDual(text){return require('./hx-capture-diagnostic.cjs').analyzeCapture(text);}
function analyze(text){if(text.includes('# BEGIN_HX_CAPTURE ')||text.includes('# BEGIN_HX_DIAG schema=2 '))return analyzeDual(text);const reads=parseHxDiagnostics(text),failures=parseHxFailures(text);need(reads.length===failures.length,'DUMP/HX_FAIL block count mismatch');return failures.map((f,i)=>{const r=reads[i];matchFailure(f,r);return {read:readSummary(r),failure:summarize(f)};});}
module.exports={TOOL_VERSION,TARGETS,COLUMNS,META,canonical,parseHxFailures,summarize,analyze,analyzeDual};
if(require.main===module){try{need(process.argv.length===3,'Usage: node tools/hx-failure-diagnostic.cjs public-serial.log');console.log(JSON.stringify(analyze(fs.readFileSync(process.argv[2],'utf8')),null,2));}catch(e){console.error(e.message);process.exitCode=1;}}
