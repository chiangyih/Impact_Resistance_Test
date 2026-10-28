// IDF/Arduino API mock for actual adapter compilation; never device evidence.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#define IRAM_ATTR
using portMUX_TYPE=int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)(x))
#define portEXIT_CRITICAL(x) ((void)(x))
#define portENTER_CRITICAL_ISR(x) ((void)(x))
#define portEXIT_CRITICAL_ISR(x) ((void)(x))
#define ESP_IDF_VERSION_VAL(a,b,c) (((a)<<16)|((b)<<8)|(c))
#define ESP_IDF_VERSION ESP_IDF_VERSION_VAL(5,5,5)
#define PIN_FUNC_GPIO 2
#define SIG_GPIO_OUT_IDX 256
using gpio_num_t=int;using esp_err_t=int;constexpr int ESP_OK=0,GPIO_FLOATING=0,GPIO_PULLUP_ONLY=1;
struct gpio_io_config_t{uint32_t fun_sel=2,sig_out=256;int drv=2;bool pu=false,pd=false,ie=false,oe=false,oe_ctrl_by_periph=false,oe_inv=false,od=false,slp_sel=false;};
struct pcnt_watch_event_data_t{int watch_point_value=32767;};struct FakePcnt;using pcnt_unit_handle_t=FakePcnt*;using pcnt_channel_handle_t=FakePcnt*;
using FakeCallback=bool(*)(pcnt_unit_handle_t,const pcnt_watch_event_data_t*,void*);
struct pcnt_unit_config_t{int low_limit=0,high_limit=0,intr_priority=0;struct{bool accum_count=false;}flags;};
struct pcnt_chan_config_t{int edge_gpio_num=0,level_gpio_num=0;struct{bool virt_level_io_level=false,io_loop_back=false;}flags;};
struct pcnt_event_callbacks_t{FakeCallback on_reach=nullptr;};constexpr int PCNT_CHANNEL_EDGE_ACTION_INCREASE=1,PCNT_CHANNEL_LEVEL_ACTION_KEEP=0;
struct FakePcnt{unsigned id=0;int pin=0,count=0,accum=0;bool running=false,enabled=false,pending=false;FakeCallback callback=nullptr;void* context=nullptr;};
inline gpio_io_config_t fakePins[40];inline int fakeLevels[40];inline uint32_t fakeNow=1000;inline std::vector<FakePcnt*> fakeUnits;inline std::vector<std::string> fakeCalls;inline std::string fakeFail;inline int fakeFailUnit=-1;inline bool fakeCorruptReady=false;
inline uint32_t micros(){return fakeNow++;}
inline bool fakeError(const char*name,int id=-1){return fakeFail==name&&(fakeFailUnit<0||fakeFailUnit==id);}
inline int gpio_get_io_config(gpio_num_t p,gpio_io_config_t*v){if(fakeError("gpio"))return -1;*v=fakePins[p];return 0;}
inline int gpio_set_pull_mode(gpio_num_t p,int mode){fakePins[p].pu=mode==GPIO_PULLUP_ONLY;fakePins[p].pd=false;if(fakeCorruptReady&&p==33)fakePins[25].sig_out=7;return fakeError("pull")?-1:0;}
inline int gpio_get_level(gpio_num_t p){return fakeLevels[p];}
inline int pcnt_new_unit(const pcnt_unit_config_t*c,pcnt_unit_handle_t*u){if(fakeError("allocate",static_cast<int>(fakeUnits.size())))return -1;auto p=new FakePcnt;p->id=static_cast<unsigned>(fakeUnits.size());fakeUnits.push_back(p);*u=p;return c->flags.accum_count&&c->high_limit==32767&&c->low_limit==-32767?0:-1;}
inline int pcnt_new_channel(pcnt_unit_handle_t u,const pcnt_chan_config_t*c,pcnt_channel_handle_t*v){u->pin=c->edge_gpio_num;auto&p=fakePins[u->pin];p.pu=true;p.pd=false;p.ie=true;if(c->flags.io_loop_back)p.oe=true;*v=u;return c->level_gpio_num==-1?0:-1;}
inline int pcnt_channel_set_edge_action(pcnt_channel_handle_t,int,int){return 0;}
inline int pcnt_channel_set_level_action(pcnt_channel_handle_t,int,int){return 0;}
inline int pcnt_unit_set_glitch_filter(pcnt_unit_handle_t u,const void*p){return p||fakeError("filter",u->id)?-1:0;}
inline int pcnt_unit_register_event_callbacks(pcnt_unit_handle_t u,const pcnt_event_callbacks_t*c,void*p){u->callback=c->on_reach;u->context=p;return 0;}
inline int pcnt_unit_add_watch_point(pcnt_unit_handle_t,int v){return v==32767?0:-1;}
inline int pcnt_unit_enable(pcnt_unit_handle_t u){u->enabled=true;return 0;}
inline int pcnt_unit_clear_count(pcnt_unit_handle_t u){fakeCalls.push_back("C"+std::to_string(u->id));if(fakeError("clear",u->id))return -1;u->count=u->accum=0;return 0;}
inline int pcnt_unit_start(pcnt_unit_handle_t u){fakeCalls.push_back("S"+std::to_string(u->id));if(fakeError("start",u->id))return -1;u->running=true;return 0;}
inline int pcnt_unit_stop(pcnt_unit_handle_t u){fakeCalls.push_back("T"+std::to_string(u->id));if(fakeError("stop",u->id))return -1;u->running=false;return 0;}
inline int pcnt_unit_get_count(pcnt_unit_handle_t u,int*v){fakeCalls.push_back("R"+std::to_string(u->id));if(fakeError("read",u->id))return -1;*v=u->count+u->accum+(u->pending&&u->count<=32767/2?32767:0);return 0;}
