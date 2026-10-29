// Actual PCNT/RMT adapters, actual RAW Observe, actual complete .ino exporter; no hardware.
'use strict';
const fs=require('node:fs'),path=require('node:path'),os=require('node:os'),crypto=require('node:crypto');
const root=path.resolve(__dirname,'..'),{functions}=require('./prepare-hx-rmt2-native-export.cjs');
const hash=crypto.createHash('sha256').update(functions).digest('hex').toUpperCase();
const prefix=fs.readFileSync(path.join(root,'tests/05-hx-rmt-adapter.native.cpp'),'utf8').split('int main()')[0];
const cpp=prefix+`
#include "hx_attempt_storage.h"
#include "hx_failure_diag.h"
struct SerialMock{template<class...A>void printf(const char*f,A...a){std::printf(f,a...);}void print(const char*s){std::fputs(s,stdout);}void println(){std::puts("");}void println(const char*s){std::puts(s);}}Serial;
HxDiagState hx_diag,hx_link_diag;HxAttemptStorage hx_attempt_storage;HxFailureDiag hx_fail;uint32_t hx_diag_session=0xDD001122;const char FIRMWARE_VERSION[]="SYNTHETIC_RMT2_ACTUAL_DEADLINE";bool cal_mode=false;
bool isRecording(){return false;}bool opticalMode(){return false;}
${functions}
int main(int argc,char**){reset();initializeHxPcnt(25,33);assert(hx_pcnt.setup_code==1);initializeHxRmt();assert(hx_rmt&&hx_rmt->s.setup==1);
hxasAllocate(hx_attempt_storage,[](size_t n)->void*{return std::malloc(n);},[](void*p){std::free(p);},[]()->size_t{return 65536;});
hxdiagBegin(hx_diag,0);hx_link_diag.trigger_mask=HXD_LINK;hxdiagBegin(hx_link_diag,0);
fakeNow=599999500;prepareHxRmt(1,true);fakeNow=599999994;fakeLevels[33]=0;prepareHxAttemptPcnt(true,hx_link_diag);
HxDiagRecord r={};r.seq=1;r.start_us=micros();fakeNow+=200;r.end_us=micros();r.bits24=0xFD7D56;r.raw=-164522;r.accepted=r.dout_after25=1;r.rx_after=1;r.pulses_issued=25;
fakeLevels[33]=1;fakeUnits[0]->count=50;fakeUnits[1]->count=hxpExpectedData(r.bits24,1)+(argc>1?2:0);finishHxAttemptPcnt(1,r.bits24,true);
assert(r.start_us>=HXD_WINDOW_US&&hx_pcnt.proof_count==1&&hx_pcnt.proof[0].valid==(argc>1?0U:1U));
hxasObserve(hx_diag,hx_attempt_storage.any,r,hx_attempt_pcnt_pending);hxasObserve(hx_link_diag,hx_attempt_storage.link,r,hx_attempt_pcnt_pending);hxdiagSeal(hx_diag);hxdiagSeal(hx_link_diag);assert(!hx_link_diag.count&&hx_link_diag.export_omitted==1);
discardHxRmtBinding(1);fill();fakeNow+=1200;rmtFire(0);rmtFire(1,2);serviceHxRmt(true);assert(!hx_rmt->s.pending&&(hx_rmt->s.terminal&HXR_NOT_BOUND));
std::printf("# SYNTHETIC_RMT2_DEADLINE no_device=1 no_adc=1 proof_valid=%u read_start=%lu raw_retained=0 export_functions_sha256=${hash}\\n",hx_pcnt.proof[0].valid,static_cast<unsigned long>(r.start_us));dumpHxDiagnostics();}
`;
function prepare(){const dir=fs.mkdtempSync(path.join(os.tmpdir(),'irt-rmt2-deadline-export-')),file=path.join(dir,'export.cpp');fs.writeFileSync(file,cpp);const fixture=path.join(dir,'fixture.serial.log');const lines=['@echo off','call "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul',`cl /nologo /utf-8 /std:c++17 /EHsc /W4 /wd4505 /I"${path.join(root,'05-test')}" /I"${path.join(root,'tests')}" /I"${path.join(root,'tests/hx-pcnt-stubs')}" "${file}" /Fo"${path.join(dir,'export.obj')}" /Fe"${path.join(dir,'export.exe')}"`,'if errorlevel 1 exit /b %errorlevel%',`"${path.join(dir,'export.exe')}" > "${fixture}"`,'if errorlevel 1 exit /b %errorlevel%',`"${path.join(dir,'export.exe')}" bad >> "${fixture}"`,'if errorlevel 1 exit /b %errorlevel%'];fs.writeFileSync(path.join(dir,'run.cmd'),lines.join('\r\n')+'\r\n');return{dir,run:path.join(dir,'run.cmd'),fixture,functions_sha256:hash};}
module.exports={prepare,hash};if(require.main===module)console.log(JSON.stringify(prepare()));
