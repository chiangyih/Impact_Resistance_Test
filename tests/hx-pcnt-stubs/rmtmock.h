#pragma once
#include "mock.h"
#include <cstdlib>
#include <cstring>
#include <algorithm>
constexpr uint32_t MALLOC_CAP_INTERNAL=1,MALLOC_CAP_8BIT=2,RMT_CLK_SRC_APB=1;
struct rmt_symbol_word_t { uint32_t val; };
struct FakeRmt;using rmt_channel_handle_t=FakeRmt*;
struct rmt_rx_done_event_data_t { rmt_symbol_word_t*received_symbols=nullptr;size_t num_symbols=0;struct{bool is_last=true;}flags; };
using FakeRmtCallback=bool(*)(rmt_channel_handle_t,const rmt_rx_done_event_data_t*,void*);
struct rmt_rx_event_callbacks_t { FakeRmtCallback on_recv_done=nullptr; };
struct rmt_rx_channel_config_t { gpio_num_t gpio_num=0;uint32_t clk_src=0,resolution_hz=0;size_t mem_block_symbols=0;int intr_priority=0;struct{bool invert_in=false,with_dma=false,io_loop_back=false,allow_pd=false;}flags; };
struct rmt_receive_config_t { uint32_t signal_range_min_ns=0,signal_range_max_ns=0;struct{bool en_partial_rx=false;}flags; };
struct FakeRmt { unsigned id=0;int pin=0;uint32_t resolution=0;bool enabled=false,armed=false,deleted=false;void*buffer=nullptr,*prior=nullptr,*ctx=nullptr;FakeRmtCallback callback=nullptr; };
inline std::vector<FakeRmt*> fakeRmt;inline std::string rmtFail;inline int rmtFailUnit=-1;inline bool failRmtHeap=false,corruptRmtPin=false;inline unsigned rmtAllocCalls=0,rmtReceiveCalls=0,rmtDeleteCalls=0,rmtDisableCalls=0;
inline bool rmtError(const char*n,int unit=-1){return rmtFail==n&&(rmtFailUnit<0||unit==rmtFailUnit);}
inline uint32_t heap_caps_get_free_size(uint32_t){return 115828;}
inline void*heap_caps_malloc(size_t n,uint32_t caps){++rmtAllocCalls;return failRmtHeap||caps!=(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)?nullptr:std::malloc(n);}
inline int rmt_new_rx_channel(const rmt_rx_channel_config_t*c,rmt_channel_handle_t*out){if(rmtError("allocate",static_cast<int>(fakeRmt.size())))return -1;if(c->gpio_num<0||c->flags.io_loop_back||c->flags.with_dma||c->mem_block_symbols!=64||c->resolution_hz!=10000000||c->clk_src!=RMT_CLK_SRC_APB)return -1;auto*p=new FakeRmt;p->id=static_cast<unsigned>(fakeRmt.size());p->pin=c->gpio_num;p->resolution=c->resolution_hz;fakeRmt.push_back(p);fakePins[p->pin].pu=true;fakePins[p->pin].ie=true;if(corruptRmtPin)fakePins[25].sig_out=7;*out=p;return 0;}
inline int rmt_rx_register_event_callbacks(rmt_channel_handle_t p,const rmt_rx_event_callbacks_t*c,void*arg){if(rmtError("callback",p->id))return -1;p->callback=c->on_recv_done;p->ctx=arg;return 0;}
inline int rmt_enable(rmt_channel_handle_t p){if(rmtError("enable",p->id))return -1;p->enabled=true;return 0;}
inline int rmt_disable(rmt_channel_handle_t p){++rmtDisableCalls;if(rmtError("disable",p->id))return -1;p->enabled=false;return 0;}
inline int rmt_del_channel(rmt_channel_handle_t p){++rmtDeleteCalls;if(rmtError("delete",p->id))return -1;p->deleted=true;return 0;}
extern "C" inline int rmt_get_channel_resolution(rmt_channel_handle_t p,uint32_t*out){if(rmtError("resolution",p->id))return -1;*out=p->resolution;return 0;}
extern "C" inline int rmt_get_channel_id(rmt_channel_handle_t p,int*out){*out=p->id;return 0;}
inline int rmt_receive(rmt_channel_handle_t p,void*b,size_t bytes,const rmt_receive_config_t*c){++rmtReceiveCalls;if(rmtError("receive",p->id)||!p->enabled||p->armed||bytes!=256||c->signal_range_min_ns||c->signal_range_max_ns!=1000000||c->flags.en_partial_rx)return -1;p->prior=p->buffer;p->buffer=b;p->armed=true;return 0;}
inline void rmtFire(unsigned i,unsigned count=26,bool last=true,bool old=false){auto*p=fakeRmt[i];rmt_rx_done_event_data_t e;e.received_symbols=static_cast<rmt_symbol_word_t*>(old?p->prior:p->buffer);e.num_symbols=count;e.flags.is_last=last;p->armed=false;p->callback(p,&e,p->ctx);}
