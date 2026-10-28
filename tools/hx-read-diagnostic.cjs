// 離線驗證公開HX_DIAG DUMP，不開埠、不讀秘密、不修正RAW。
'use strict';
const fs = require('node:fs');
const WORDS = ['seq','start_us','end_us','attempt_gap_us','last_accept_ms_before','bits24','raw','rx_before','rx_after','err_before','err_after','flags','cal_count_after','phase_after','cal_mode_after','accepted','dout_after25','pulses_issued','context_flags'];
const COLUMNS = ['kind',...WORDS,'crc32'];
function crc32(words) {
  let crc=0xffffffff;
  for(const word of words) for(let byte=0;byte<4;byte++) {
    crc^=(word>>>(8*byte))&255;
    for(let bit=0;bit<8;bit++) crc=(crc>>>1)^((crc&1)?0xedb88320:0);
  }
  return (crc^0xffffffff)>>>0;
}
function fields(line) { return Object.fromEntries([...line.matchAll(/([A-Za-z][A-Za-z0-9_]*)=([^\s;]+)/g)].map(x=>[x[1],x[2]])); }
function need(ok,message) { if(!ok) throw new Error(message); }
function uintField(object,key,max=0xffffffff) { const text=object[key];need(typeof text==='string'&&/^\d+$/.test(text)&&Number(text)<=max,`Missing/invalid metadata ${key}`);return Number(text); }
function decode(word) { return word-((word&0x800000)?16777216:0); }
function parseRecord(line) {
  const cols=line.split(','); need(cols.length===COLUMNS.length && cols[0]==='HXRD','Invalid/truncated HXRD row');
  const r={};
  WORDS.forEach((key,i)=>{
    const text=cols[i+1]; need(key==='raw'?/^-?\d+$/.test(text):key==='bits24'?/^0x[0-9a-fA-F]{6}$/.test(text):/^\d+$/.test(text),`Invalid ${key}`);
    const value=Number(text);need(Number.isInteger(value),'Invalid integer');
    need(key==='raw'?(value>=-8388608&&value<=8388607):(value>=0&&value<=0xffffffff),`Out of range ${key}`); r[key]=value;
  });
  need(/^0x[0-9a-fA-F]{8}$/.test(cols.at(-1)),'Invalid CRC field');
  r.crc32=Number(cols.at(-1)); need(crc32(WORDS.map(k=>r[k]>>>0))===r.crc32,`CRC mismatch seq=${r.seq}`);
  need(r.bits24<=0xffffff && decode(r.bits24)===r.raw,'RAW does not decode bits24');
  need((r.accepted===0||r.accepted===1)&&r.dout_after25===r.accepted,'Invalid acceptance/postcheck');
  need(r.pulses_issued===25,'Unexpected issued pulse count');
  need((r.flags&~7)===0 && Boolean(r.flags&1)===!r.accepted,'Invalid link classification');
  need(Boolean(r.flags&2)===Boolean(r.accepted&&(r.raw===-8388608||r.raw===8388607)),'Invalid SAT classification');
  need(r.cal_count_after<=65535&&r.phase_after<=5&&r.cal_mode_after<=2&&r.context_flags<=3,'Invalid context');
  need(((r.rx_after-r.rx_before)>>>0)===r.accepted && ((r.err_after-r.err_before)>>>0)===(r.accepted?0:1),'Counter contract mismatch');
  r.read_cpu_us=(r.end_us-r.start_us)>>>0;
  return r;
}
function parseHxDiagnostics(text) {
  const captures=[];let block=null;
  for(const line of text.split(/\r?\n/)) {
    if(line.startsWith('# BEGIN_HX_DIAG ')) {
      need(!block,'Nested BEGIN_HX_DIAG');const meta=fields(line);
      need(['1','2'].includes(meta.schema)&&typeof meta.version==='string'&&meta.version.length>0&&/^[0-9a-fA-F]{8}$/.test(meta.session||''),'Unsupported schema/session/version');
      if(meta.schema==='2')need((meta.slot==='ANY'&&meta.trigger_mask==='7')||(meta.slot==='LINK'&&meta.trigger_mask==='1'),'Invalid dual slot/policy');
      need(meta.crc==='CRC32_ISO_HDLC_19_LE_U32'&&meta.computed_on_export==='1','Unsupported CRC contract');
      block={meta,rows:[],status:{},time:{},counts:{},columns:false};
    } else if(block&&line.startsWith('# END_HX_DIAG ')) {
      const end=fields(line);need(block.columns,'Missing column header');
      need(block.status.mode==='FROZEN','DUMP is not a frozen capture');
      need(block.status.schema===block.meta.schema&&block.status.session===block.meta.session&&block.status.version===block.meta.version,'Mixed epoch/version');
      const mask=block.meta.schema==='2'?Number(block.meta.trigger_mask):7;
      if(block.meta.schema==='2')need(block.status.slot===block.meta.slot&&block.status.trigger_mask===block.meta.trigger_mask&&block.status.export_sealed==='1'&&block.counts.scope==='frozen'&&end.slot===block.meta.slot,'Unsealed/mixed dual slot');
      need(block.time.clock==='boot_micros_u32','Unsupported/missing device clock');
      need(uintField(block.status,'capacity')===256&&uintField(block.status,'record_bytes')===56&&uintField(block.status,'ring_bytes')===14336,'Unsupported record layout');
      const reason=uintField(block.status,'freeze_reason',4);need(reason>=1,'Invalid freeze reason');
      for(const key of ['start_us','window_us','trigger_us','freeze_us','trigger_seq','trigger_flags','pre','post','post_target','post_complete'])uintField(block.time,key);
      for(const key of ['attempts','pre_overwritten','omitted_after_freeze','overflow','seq_wrapped'])uintField(block.counts,key);
      need(Number(block.time.window_us)===600000000&&Number(block.time.post_target)===128,'Unsupported observation profile');
      need(Number(block.time.pre)<=127&&Number(block.time.post)<=128&&Number(block.time.post_complete)<=1,'Invalid retained window');
      need(Number(block.counts.overflow)<=1&&Number(block.counts.seq_wrapped)<=1,'Invalid validity flag');
      need(Number(block.meta.rows)===block.rows.length&&Number(end.rows)===block.rows.length&&Number(block.status.records)===block.rows.length,'Row count mismatch');
      need(end.retained==='1'&&end.read_only==='1','Unexpected retention contract');
      if(block.meta.schema==='2')need(Number(block.counts.attempts)===((block.rows.length+Number(block.counts.pre_overwritten)+Number(block.counts.omitted_after_freeze))>>>0),'Frozen count balance mismatch');
      if(Number(block.time.trigger_flags)) {
        need((Number(block.time.trigger_flags)&mask)!==0,'Trigger does not match slot policy');
        const pre=Number(block.time.pre),post=Number(block.time.post),trigger=block.rows[pre];
        need(pre+1+post===block.rows.length&&trigger&&trigger.seq===Number(block.time.trigger_seq)&&trigger.flags===Number(block.time.trigger_flags)&&trigger.end_us===Number(block.time.trigger_us),'Trigger/window metadata mismatch');
      } else need(block.rows.length<=127&&block.rows.every(r=>(r.flags&mask)===0),'Invalid untriggered window');
      let previous=null,lastAccepted=null;
      for(const r of block.rows) {
        if(previous) {
          need(((r.seq-previous.seq)>>>0)===1,'Missing/reordered/duplicate attempt sequence');
          need(r.rx_before===previous.rx_after&&r.err_before===previous.err_after,'Counter continuity mismatch');
          if(r.context_flags&2)need(r.attempt_gap_us===((r.start_us-previous.start_us)>>>0),'Attempt gap mismatch');
        }
        if(r.accepted&&lastAccepted)need(Boolean(r.flags&4)===(Math.abs(r.raw-lastAccepted.raw)>=1000),'Jump flag mismatch');
        if(!r.accepted)need(!(r.flags&4),'Rejected word classified as accepted jump');
        if(r.accepted)lastAccepted=r;
        previous=r;
      }
      captures.push(block);block=null;
    } else if(block&&line===COLUMNS.join(',')) block.columns=true;
    else if(block&&line.startsWith('HXRD,')) block.rows.push(parseRecord(line));
    else if(block&&line.startsWith('# HX_DIAG '))block.status=fields(line);
    else if(block&&line.startsWith('# HX_DIAG_TIME '))block.time=fields(line);
    else if(block&&line.startsWith('# HX_DIAG_COUNTS '))block.counts=fields(line);
    else if(block&&line.startsWith('# HX_DIAG_LIMIT ')) { /* 只留界線文字，不能把CRC當ADC真值。 */ }
    else if(block&&line.trim())throw new Error('Unexpected/interleaved DUMP line');
  }
  need(!block,'Incomplete HX_DIAG DUMP; END missing');need(captures.length>0,'No complete HX_DIAG DUMP');
  return captures;
}
function summarize(c) {
  const accepted=c.rows.filter(r=>r.accepted),rejected=c.rows.filter(r=>!r.accepted),candidates=[];
  for(let i=1;i<accepted.length-1;i++)for(let k=1;k<=23;k++)if((accepted[i-1].raw>>k)===accepted[i].raw&&(accepted[i+1].raw>>k)===accepted[i].raw)candidates.push({seq:accepted[i].seq,k,raw:accepted[i].raw,candidate_only:true});
  return {schema:Number(c.meta.schema),slot:c.meta.slot||'LEGACY',trigger_mask:Number(c.meta.trigger_mask||7),version:c.meta.version,session:c.meta.session,rows:c.rows.length,accepted:accepted.length,rejected:rejected.length,freeze_reason:Number(c.status.freeze_reason),trigger_flags:Number(c.time.trigger_flags),pre:Number(c.time.pre),post:Number(c.time.post),post_complete:c.time.post_complete==='1',overflow:c.counts.overflow==='1',first_attempt_us:c.rows[0]?.start_us??null,last_attempt_us:c.rows.at(-1)?.end_us??null,window_start_us:Number(c.time.start_us),trigger_us:Number(c.time.trigger_us),freeze_us:Number(c.time.freeze_us),candidates,crc_scope:'export_only',cpu_times_not_pin_waveform:true,host_receive_utc_not_read_time:true,independent_adc_truth:false};
}
module.exports={WORDS,COLUMNS,crc32,parseRecord,parseHxDiagnostics,summarize};
if(require.main===module) {
  try { need(process.argv.length===3,'Usage: node tools/hx-read-diagnostic.cjs public-serial.log');console.log(JSON.stringify(parseHxDiagnostics(fs.readFileSync(process.argv[2],'utf8')).map(summarize),null,2)); }
  catch(e){console.error(e.message);process.exitCode=1;}
}
