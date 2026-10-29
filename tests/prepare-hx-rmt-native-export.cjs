// Prepare a temporary, public-source-only C++ exporter. Does not compile, access COM or ADC.
'use strict';
const fs=require('node:fs'),path=require('node:path'),os=require('node:os'),crypto=require('node:crypto');
const root=path.resolve(__dirname,'..'),source=fs.readFileSync(path.join(root,'05-test/05-test.ino'),'utf8');
const names=['hxRmtSealed','dumpHxRmt','hxDiagModeName','printHxDiagSlot','hxFailureModeName','hxFailureHolding','printHxFailureStatus','dumpHxFailure','dumpHxPcntUnit','dumpHxPcnt','dumpHxDiagSlot','hxAttemptPcntAligned','dumpHxAttemptPcnt','dumpHxDiagnostics'];
function extract(name){const match=new RegExp('^(?:void|bool|const char \\*) ?'+name+'\\(','m').exec(source);if(!match)throw Error('Missing actual exporter '+name);const a=source.indexOf('{',match.index);let depth=1,i=a+1;for(;i<source.length&&depth;i++){if(source[i]==='{')depth++;else if(source[i]==='}')depth--;}if(depth)throw Error('Incomplete public function '+name);return source.slice(match.index,i);}
const functions=names.map(extract).join('\n'),hash=crypto.createHash('sha256').update(functions).digest('hex').toUpperCase();
const cpp=`// SYNTHETIC public C++ exporter; no device, GPIO API, WiFi or credentials.
#include "hx_attempt_storage.h"
#include "hx_rmt_diag.h"
#include "hx_failure_diag.h"
#include <cstdio>
#include <cassert>
#include <cstdlib>
struct SerialMock{template<class...A>void printf(const char*f,A...a){std::printf(f,a...);}void print(const char*s){std::fputs(s,stdout);}void println(){std::puts("");}void println(const char*s){std::puts(s);}};
SerialMock Serial;HxDiagState hx_diag,hx_link_diag;HxAttemptStorage hx_attempt_storage;HxFailureDiag hx_fail;HxPcntDiag hx_pcnt;
struct RmtPublic {HxrState s;uint32_t closed=1;}rmt_public;RmtPublic*hx_rmt=&rmt_public;bool hx_rmt_setup_tried=true;void serviceHxRmt(bool close){if(close)rmt_public.closed=1;}
uint32_t hx_diag_session=0x89abcdef;const char FIRMWARE_VERSION[]="SYNTHETIC_native_frame_export";bool cal_mode=false;
bool isRecording(){return false;}bool opticalMode(){return false;}
void sealHxPcnt(bool frozen,bool hold){if(frozen&&!hold)hxpSeal(hx_pcnt);}
${functions}
static HxPcntUnit unit(uint32_t start,uint32_t stop,int count,bool high=false){HxPcntUnit u;u.start_begin=start;u.start_end=start+1;u.level_start_begin=start+2;u.level_start_end=start+3;u.level_start=0;u.level_end_begin=stop-3;u.level_end_end=stop-2;u.level_end=high;u.stop_begin=stop;u.stop_end=stop+1;u.read_begin=stop+2;u.read_end=stop+3;u.api_mask=15;u.attempt_mask=7;u.count=count;u.valid=hxpUnitValid(u);return u;}
int main(int argc,char**argv){std::puts("# SYNTHETIC_NATIVE_FRAME_EXPORT no_device=1 no_adc=1 export_functions_sha256=${hash}");const int requested=argc>1?std::atoi(argv[1]):0;const int failure=requested>=2&&requested<=4?requested:0;if(requested==5)hx_rmt=nullptr;unsigned allocations=0;hxasAllocate(hx_attempt_storage,[&](size_t bytes)->void*{++allocations;if((failure==2&&allocations==1)||(failure==3&&allocations==2))return nullptr;return std::malloc(bytes);},[](void*p){std::free(p);},[&]()->size_t{return failure==4?65535:65536;});assert(hx_attempt_storage.ready()==!failure);
rmt_public.s.setup=1;rmt_public.s.stage_mask=31;rmt_public.s.allocated_bytes=4080;rmt_public.s.resolution[0]=rmt_public.s.resolution[1]=10000000;rmt_public.s.channel_id[1]=1;for(auto&stage:rmt_public.s.pins){stage[0]={2,256,2,28};stage[1]={2,256,2,5};}
hxdiagBegin(hx_diag,0);hx_link_diag.trigger_mask=HXD_LINK;hxdiagBegin(hx_link_diag,0);hx_pcnt.gpio_function=2;hx_pcnt.gpio_signal=256;hx_pcnt.setup_code=1;hx_pcnt.stage_mask=15;
for(unsigned s=0;s<4;s++){hx_pcnt.pins[s][0]={2,256,2,s?28U:24U};hx_pcnt.pins[s][1]={2,256,2,5};}hx_pcnt.pins[1][0].flags=29;
uint32_t rx=0,err=0;for(uint32_t seq=1;seq<=400;seq++){HxDiagRecord r={};r.seq=seq;r.start_us=seq*10000+(seq>250?250000:0);r.end_us=r.start_us+200;r.accepted=seq!=250;r.dout_after25=r.accepted;r.raw=seq==130?-80:r.accepted?-163000:-317;r.bits24=static_cast<uint32_t>(r.raw)&0xffffff;r.rx_before=rx;r.rx_after=rx+(r.accepted?1:0);r.err_before=err;r.err_after=err+(r.accepted?0:1);r.pulses_issued=25;
HxAttemptPcnt p;p.seq=seq;p.policy=159;p.prepare_begin=r.start_us-120;p.finish_end=r.end_us+80;HxPcntUnit units[]={unit(r.start_us-100,r.end_us+60,50),unit(r.start_us-80,r.end_us+40,hxpExpectedData(r.bits24,r.dout_after25),r.dout_after25)};p.unit[0]=hxapPack(units[0]);p.unit[1]=hxapPack(units[1]);
if(seq==200){p.unit[1].count+=2;}
if(seq<=4&&hx_attempt_storage.ready()){p.policy=(159&~4)|32|(HXP_NO_PROOF<<16);HxPcntProof proof;proof.seq=seq;proof.bits24=r.bits24;proof.accepted=proof.postcheck=1;proof.unit[0]=units[0];proof.unit[1]=units[1];hxpAddProof(hx_pcnt,proof);}
if(!r.accepted){hxfStart(hx_fail,seq,r.start_us,r.end_us,r.bits24,rx,r.err_after,0,0,true,r.end_us+130);hxfDue(hx_fail,r.end_us+130);for(unsigned k=0;k<HXF_POINTS;k++)if(HXF_TARGET_US[k]>=130)hxfSample(hx_fail,r.end_us+HXF_TARGET_US[k],r.end_us+HXF_TARGET_US[k]+1,false);assert(hx_fail.mode==HXF_COMPLETE);bool running=hxpLink(hx_pcnt,seq,r.end_us,true,r.end_us+90);hx_pcnt.event_end=r.end_us+130;if(running){hx_pcnt.unit[0]=unit(r.end_us+100,r.end_us+250020,0);hx_pcnt.unit[1]=unit(r.end_us+110,r.end_us+250010,4);hx_pcnt.finish_begin=r.end_us+250005;hxpComplete(hx_pcnt,r.end_us+250030);}}
if(r.accepted)hxfResume(hx_fail,r.start_us,seq,hxdiagCanRetain(hx_link_diag,r.start_us));hxasObserve(hx_diag,hx_attempt_storage.any,r,p);hxasObserve(hx_link_diag,hx_attempt_storage.link,r,p);hxdiagSeal(hx_diag);hxdiagSeal(hx_link_diag);if(hx_rmt&&hx_attempt_storage.ready()&&hx_link_diag.count){const auto&stored=hx_link_diag.records[(hx_link_diag.next+255)%256];if(stored.seq==seq){auto&s=rmt_public.s;auto&w=s.work[0];w=HxrTrace();w.seq=w.token=seq;w.arm_mask=w.done_mask=w.safe_copy_mask=3;w.prepare_begin=r.start_us-260;w.prepare_end=r.start_us-150;w.hook_begin=r.end_us+90;w.hook_end=r.end_us+92;for(unsigned k=0;k<2;k++){w.unit[k].request_begin=r.start_us-230+20*k;w.unit[k].request_end=r.start_us-220+20*k;w.unit[k].callback_us=r.end_us+1100+k;w.unit[k].callback_flags=3;}w.unit[0].count=26;for(unsigned i=0;i<25;i++)w.unit[0].symbols[i]=20U|(32788U<<16);w.unit[0].symbols[25]=10000;w.unit[1].count=2;w.unit[1].symbols[0]=20U|(32788U<<16);w.unit[1].symbols[1]=10000;s.current=0;s.pending=1;hxrSelect(s,stored,p);hxrRetain(s,w);s.pending=0;s.attempts=seq;s.last_seq=seq;}}
rx=r.rx_after;err=r.err_after;}
assert(hx_diag.mode==HXD_FROZEN&&hx_link_diag.mode==HXD_FROZEN);dumpHxDiagnostics();std::fprintf(stderr,"allocation_state=%u allocated_sidecars=%u original_record=%zu sidecar_record=%zu\\n",hx_attempt_storage.state,hx_attempt_storage.ready()?HXAS_BYTES:0,sizeof(HxDiagRecord),sizeof(HxAttemptPcnt));}
`;
function prepare(){if(source.includes('hxfail5-pcnt2m1-rmt2'))return require('./prepare-hx-rmt2-native-export.cjs').prepare();
  const dir=fs.mkdtempSync(path.join(os.tmpdir(),'irt-rmt-export-')),file=path.join(dir,'export.cpp');fs.writeFileSync(file,cpp);
  const batch=['@echo off','call "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul',`cl /nologo /utf-8 /std:c++17 /EHsc /W4 /I"${path.join(root,'05-test')}" "${file}" /Fo"${path.join(dir,'export.obj')}" /Fe"${path.join(dir,'export.exe')}"`,'if errorlevel 1 exit /b %errorlevel%',`"${path.join(dir,'export.exe')}" > "${path.join(dir,'rmt-fixture.serial.log')}"`,'if errorlevel 1 exit /b %errorlevel%',...[2,3,4].flatMap(state=>[`"${path.join(dir,'export.exe')}" ${state} ${state===2?'>':'>>'} "${path.join(dir,'rmt-degraded.serial.log')}"`,'if errorlevel 1 exit /b %errorlevel%']),`"${path.join(dir,'export.exe')}" 5 > "${path.join(dir,'rmt-unavailable.serial.log')}"`,'if errorlevel 1 exit /b %errorlevel%'];
  fs.writeFileSync(path.join(dir,'run.cmd'),batch.join('\r\n')+'\r\n');return{dir,run:path.join(dir,'run.cmd'),fixture:path.join(dir,'rmt-fixture.serial.log'),degraded_fixture:path.join(dir,'rmt-degraded.serial.log'),unavailable_fixture:path.join(dir,'rmt-unavailable.serial.log'),functions_sha256:hash};
}
module.exports={names,extract,functions,hash,prepare};if(require.main===module)console.log(JSON.stringify(prepare()));
