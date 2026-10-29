// Compile actual RMT adapter and actual .ino RMT exporter against deterministic mocks.
'use strict';
const fs=require('node:fs'),path=require('node:path'),os=require('node:os'),crypto=require('node:crypto');
const root=path.resolve(__dirname,'..');
const {extract}=require('./prepare-hx-rmt2-native-export.cjs'),dump=[extract('dumpHxRmt'),extract('dumpHxPcntUnit'),extract('dumpHxPcnt')].join('\n');
const hash=crypto.createHash('sha256').update(dump).digest('hex').toUpperCase();
const helpers=fs.readFileSync(path.join(root,'tests/05-hx-rmt-adapter.native.cpp'),'utf8').split('int main()')[0];
const cpp=helpers+`
#include <string>
struct CaptureSerial { std::string text; void print(const char*s){text+=s;}void println(){text+='\\n';}void println(const char*s){text+=s;text+='\\n';}template<class...A>void printf(const char*f,A...a){char b[1024];int n=std::snprintf(b,sizeof(b),f,a...);assert(n>=0&&n<1024);text.append(b,n);} } Serial;
const char FIRMWARE_VERSION[]="SYNTHETIC_actual_rmt_adapter";uint32_t hx_diag_session=0x12345678;
${dump}
int main(int argc,char**argv){const int mode=argc>1?std::atoi(argv[1]):0;reset();
if(mode==1){failRmtHeap=true;initializeHxRmt();}
else if(mode==5||mode==6){initializeHxRmt();prepareHxRmt(1,false);bind(1,true,mode==5);prepareHxRmt(2,true);bind(2,true,mode==6);done();serviceHxRmt(true);}
else if(mode==7){initializeHxRmt();prepareHxRmt(1,true);bind(1,true,true);fill();rmtFire(0);fakeNow+=6000;serviceHxRmt();prepareHxRmt(2,true);bind(2);serviceHxRmt(true);}
else if(mode==8||mode==9){initializeHxRmt();for(unsigned i=1;i<=(mode==8?8U:4U);i++){prepareHxRmt(i,true);bind(i,true,mode==8);done();}serviceHxRmt(true);}
else{initializeHxRmt();if(mode==4){prepareHxRmt(1,true);bind();done();}prepareHxRmt(mode==4?2:1,true);
if(mode==2){fill();fakeNow+=1200;rmtFire(0);rmtFire(1,2);serviceHxRmt(true);}
else if(mode==3){serviceHxRmt(true);fakeNow+=6000;serviceHxRmt(true);}
else{bind(mode==4?2:1,false);fill();if(mode==4){fakeNow+=1200;rmtFire(0,26,true,true);rmtFire(1,2);}else{rmtFire(0);fakeNow+=6000;}serviceHxRmt(true);}}
assert(!hx_rmt||!hx_rmt->s.pending);hxpSeal(hx_pcnt);dumpHxPcnt();dumpHxRmt();const auto before=Serial.text;Serial.text.clear();
if(mode==0||mode==7){rmtFire(1,2);}if(mode==1){failRmtHeap=false;initializeHxRmt();}if(mode==3){fill();rmtFire(0);rmtFire(1,2);}prepareHxRmt(99,true);serviceHxRmt(true);dumpHxPcnt();dumpHxRmt();assert(Serial.text==before);
std::printf("# SYNTHETIC_RMT_ACTUAL_ADAPTER mode=%d no_device=1 no_adc=1 export_functions_sha256=${hash} immutable_after_late_event=1\\n",mode);std::fputs(before.c_str(),stdout);std::fprintf(stderr,"mode=%d immutable_actual_export=1 bytes=%zu\\n",mode,before.size());reset();}
`;
function prepare(){const dir=fs.mkdtempSync(path.join(os.tmpdir(),'irt-rmt2-adapter-export-')),file=path.join(dir,'export.cpp');fs.writeFileSync(file,cpp);const fixture=path.join(dir,'fixture.serial.log');const lines=['@echo off','call "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul',`cl /nologo /utf-8 /std:c++17 /EHsc /W4 /wd4505 /I"${path.join(root,'tests')}" /I"${path.join(root,'tests/hx-pcnt-stubs')}" "${file}" /Fo"${path.join(dir,'export.obj')}" /Fe"${path.join(dir,'export.exe')}"`,'if errorlevel 1 exit /b %errorlevel%',...[0,1,2,3,4,5,6,7,8,9].flatMap(i=>[`"${path.join(dir,'export.exe')}" ${i} ${i?'>>':'>'} "${fixture}"`,'if errorlevel 1 exit /b %errorlevel%'])];fs.writeFileSync(path.join(dir,'run.cmd'),lines.join('\r\n')+'\r\n');return{dir,run:path.join(dir,'run.cmd'),fixture,functions_sha256:hash};}
module.exports={prepare,hash};if(require.main===module)console.log(JSON.stringify(prepare()));
