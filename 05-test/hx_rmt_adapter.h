#ifndef IRT_HX_RMT_ADAPTER_H
#define IRT_HX_RMT_ADAPTER_H
#include "hx_pcnt_adapter.h"
#include "hx_rmt_diag.h"
#include <driver/rmt_rx.h>
#include <esp_heap_caps.h>
#include <new>
// Reviewed, read-only IDF 5.5.5 ABI defined in rmt_common.c; symbols verified in installed libesp_driver_rmt.a.
// These are not declared in public rmt_common.h; the pinned SDK assertion and formal link are mandatory.
extern "C" esp_err_t rmt_get_channel_resolution(rmt_channel_handle_t,uint32_t*);
extern "C" esp_err_t rmt_get_channel_id(rmt_channel_handle_t,int*);
struct HxrRuntime;
struct HxrContext { HxrRuntime *owner=nullptr; uint32_t unit=0; };
struct HxrRuntime { HxrState s; rmt_channel_handle_t channel[2]={}; bool enabled[2]={}; HxrContext context[2]={}; uint32_t closed=0; };
static_assert(sizeof(HxrRuntime)<=4096,"RMT heap reservation exceeds 4KiB");
static HxrRuntime *hx_rmt=nullptr;
static bool hx_rmt_setup_tried=false;
static portMUX_TYPE hx_rmt_mux=portMUX_INITIALIZER_UNLOCKED;
static bool IRAM_ATTR hxRmtDone(rmt_channel_handle_t,const rmt_rx_done_event_data_t *data,void *arg) {
  HxrContext *ctx=static_cast<HxrContext*>(arg); HxrState &s=ctx->owner->s;
  portENTER_CRITICAL_ISR(&hx_rmt_mux);HxrCallback &c=s.cb[ctx->unit];++c.calls;
  c.count=static_cast<uint32_t>(data->num_symbols);c.at_us=micros();c.flags=(data->flags.is_last?1U:0U)|(data->received_symbols==reinterpret_cast<rmt_symbol_word_t*>(c.buffer)?2U:0U);
  if(c.calls!=1)c.flags|=4;c.done=1;portEXIT_CRITICAL_ISR(&hx_rmt_mux);return false;
}
static bool hxRmtPins(uint8_t stage) {
  gpio_io_config_t a={},b={};if(!hx_rmt||stage>=5||gpio_get_io_config(static_cast<gpio_num_t>(25),&a)!=ESP_OK||gpio_get_io_config(static_cast<gpio_num_t>(33),&b)!=ESP_OK)return false;
  hx_rmt->s.pins[stage][0]=hxPcntPin(a);hx_rmt->s.pins[stage][1]=hxPcntPin(b);hx_rmt->s.stage_mask|=1U<<stage;return true;
}
static bool hxRmtSafe(uint8_t stage) {
  return hx_rmt&&(hx_rmt->s.stage_mask&(1U<<stage))&&hxpSamePin(hx_rmt->s.pins[0][0],hx_rmt->s.pins[stage][0],true)&&hxpSamePin(hx_rmt->s.pins[0][1],hx_rmt->s.pins[stage][1]);
}
static bool hxRmtCurrentPinsSafe() {
  gpio_io_config_t a={},b={};if(gpio_get_io_config(static_cast<gpio_num_t>(25),&a)!=ESP_OK||gpio_get_io_config(static_cast<gpio_num_t>(33),&b)!=ESP_OK)return false;
  return hxpSamePin(hx_rmt->s.pins[0][0],hxPcntPin(a),true)&&hxpSamePin(hx_rmt->s.pins[0][1],hxPcntPin(b));
}
static void hxRmtCleanupSetup() { // Only before any receive; never free a buffer/context visible to an armed RX.
  for(int i=1;i>=0;i--)if(hx_rmt->channel[i]){
    bool stopped=!hx_rmt->enabled[i];if(hx_rmt->enabled[i]&&rmt_disable(hx_rmt->channel[i])==ESP_OK){stopped=true;hx_rmt->enabled[i]=false;hx_rmt->s.cleanup_mask|=1U<<i;}
    if(stopped&&rmt_del_channel(hx_rmt->channel[i])==ESP_OK){hx_rmt->channel[i]=nullptr;hx_rmt->s.cleanup_mask|=1U<<(i+2);}
  }
}
static void initializeHxRmt() {
  if(hx_rmt_setup_tried)return;hx_rmt_setup_tried=true;
  const uint32_t before=heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
  void *memory=heap_caps_malloc(sizeof(HxrRuntime),MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);if(!memory)return;
  hx_rmt=new(memory)HxrRuntime();HxrState &s=hx_rmt->s;s.setup_attempts=1;s.allocation_before=before;s.allocated_bytes=sizeof(HxrRuntime);
  if(!hxRmtPins(0)){s.setup=4;s.terminal=HXR_GPIO;hx_pcnt.clock_blocked=1;return;}
  const HxPcntPin&a=s.pins[0][0],&b=s.pins[0][1];
  if(a.function!=PIN_FUNC_GPIO||a.signal!=SIG_GPIO_OUT_IDX||(a.flags&~(4U|16U))!=8U||b.function!=PIN_FUNC_GPIO||b.flags!=5U){s.setup=4;s.terminal=HXR_GPIO;hx_pcnt.clock_blocked=1;return;}
  bool ok=true;
  for(uint8_t i=0;i<2&&ok;i++){
    rmt_rx_channel_config_t c={};c.gpio_num=static_cast<gpio_num_t>(i?33:25);c.clk_src=RMT_CLK_SRC_APB;c.resolution_hz=HXR_RESOLUTION;c.mem_block_symbols=HXR_SYMBOLS;c.intr_priority=0;c.flags.with_dma=false;c.flags.io_loop_back=false;
    if(rmt_new_rx_channel(&c,&hx_rmt->channel[i])!=ESP_OK){ok=false;break;}
    hx_rmt->context[i].owner=hx_rmt;hx_rmt->context[i].unit=i;rmt_rx_event_callbacks_t callbacks={};callbacks.on_recv_done=hxRmtDone;
    if(rmt_rx_register_event_callbacks(hx_rmt->channel[i],&callbacks,&hx_rmt->context[i])!=ESP_OK||rmt_enable(hx_rmt->channel[i])!=ESP_OK){ok=false;break;}hx_rmt->enabled[i]=true;
    int id=-1;if(rmt_get_channel_resolution(hx_rmt->channel[i],&s.resolution[i])!=ESP_OK||rmt_get_channel_id(hx_rmt->channel[i],&id)!=ESP_OK||id<0||s.resolution[i]!=HXR_RESOLUTION){ok=false;break;}s.channel_id[i]=static_cast<uint32_t>(id);
  }
  const bool driver=hxRmtPins(1);if(!ok)hxRmtCleanupSetup();
  const bool sck_pull=gpio_set_pull_mode(static_cast<gpio_num_t>(25),GPIO_FLOATING)==ESP_OK;
  const bool data_pull=gpio_set_pull_mode(static_cast<gpio_num_t>(33),GPIO_PULLUP_ONLY)==ESP_OK;const bool restored=sck_pull&&data_pull;
  const bool ready=hxRmtPins(2);if(!restored||!ready||!hxRmtSafe(2)){s.terminal|=HXR_GPIO;hx_pcnt.clock_blocked=1;}
  s.setup=ok&&driver&&ready&&!s.terminal?1:3;if(s.setup!=1)s.terminal|=HXR_API;
  s.allocation_after=heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
}
static void serviceHxRmt(bool close_window=false) {
  if(!hx_rmt)return;HxrState&s=hx_rmt->s;if(close_window)hx_rmt->closed=1;
  if(!s.pending)return;HxrTrace&w=s.work[s.current];HxrCallback cb[2];portENTER_CRITICAL(&hx_rmt_mux);cb[0]=s.cb[0];cb[1]=s.cb[1];portEXIT_CRITICAL(&hx_rmt_mux);
  bool waiting=false;for(uint8_t i=0;i<2;i++)if(w.arm_mask&(1U<<i)){
    if(!cb[i].done){waiting=true;continue;}
    w.done_mask|=1U<<i;w.unit[i].callback_us=cb[i].at_us;w.unit[i].count=cb[i].count;w.unit[i].callback_flags=cb[i].flags;
    const bool safe=cb[i].token==w.token&&cb[i].calls==1&&cb[i].flags==3&&cb[i].count<=HXR_SYMBOLS;
    if(!safe){w.safe_copy_mask&=~(1U<<i);w.reason|=HXR_CALLBACK;s.terminal|=HXR_CALLBACK;continue;}w.safe_copy_mask|=1U<<i;
    if(hxdiagDelta(cb[i].at_us,w.unit[i].request_begin)>HXR_TIMEOUT_US){w.reason|=HXR_LATE;s.terminal|=HXR_LATE;}
    uint32_t bad=i?hxrSymbolReason(w.unit[i]):hxrSckReason(w.unit[i]);
    if(w.read_bound&&(hxdiagDelta(w.raw.start_us,w.unit[i].request_end)>=HXP_MAX_SPAN_US||hxdiagDelta(cb[i].at_us,w.raw.end_us)>=HXP_MAX_SPAN_US))bad|=HXR_LATE;
    w.reason|=bad;s.terminal|=bad&~HXR_SYMBOL; // Unknown endpoint/shape must not erase all future diagnostic opportunities.
  }
  if(waiting&&hxdiagDelta(micros(),w.unit[0].request_begin)<HXR_TIMEOUT_US)return;
  if(waiting){w.reason|=HXR_TIMEOUT;s.terminal|=HXR_TIMEOUT;}
  if(!w.read_bound){if(!hx_rmt->closed&&!(w.reason&HXR_NOT_BOUND))return;w.reason|=HXR_NOT_BOUND;s.terminal|=HXR_NOT_BOUND;}
  hxrRetain(s,w);s.pending=0; // Missing RAW binding closes without inventing a RAW/event; timeout retains all buffer lifetimes.
}
static void prepareHxRmt(uint32_t seq,bool eligible) {
  if(!hx_rmt||hx_rmt->closed)return;const uint32_t prepare_begin=micros();serviceHxRmt();HxrState&s=hx_rmt->s;++s.attempts;s.last_seq=seq;
  if(!eligible||s.setup!=1||s.terminal||hx_rmt->closed){++s.skipped;return;}
  if(s.pending){s.work[s.current].reason|=HXR_OVERLAP;++s.skipped;return;}
  if(!hxRmtCurrentPinsSafe()){s.terminal|=HXR_GPIO;hx_pcnt.clock_blocked=1;++s.skipped;return;}
  s.current^=1;HxrTrace&w=s.work[s.current];w=HxrTrace();w.seq=seq;w.token=++s.next_token;w.prepare_begin=prepare_begin;s.pending=1;
  rmt_receive_config_t receive={};receive.signal_range_min_ns=0;receive.signal_range_max_ns=HXR_IDLE_NS;receive.flags.en_partial_rx=false;
  for(uint8_t i=0;i<2;i++){
    portENTER_CRITICAL(&hx_rmt_mux);s.cb[i]=HxrCallback();s.cb[i].buffer=w.unit[i].symbols;s.cb[i].token=w.token;portEXIT_CRITICAL(&hx_rmt_mux);
    w.unit[i].request_begin=micros();const esp_err_t result=rmt_receive(hx_rmt->channel[i],w.unit[i].symbols,sizeof(w.unit[i].symbols),&receive);w.unit[i].request_end=micros();
    if(result!=ESP_OK){w.reason|=HXR_API;s.terminal|=HXR_API;break;}w.arm_mask|=1U<<i;
  }
  w.prepare_end=micros();
}
static void recordHxRmt(const HxDiagRecord&r,const HxAttemptPcnt&p) {
  if(!hx_rmt||hx_rmt->closed)return;const uint32_t hook_begin=micros();HxrState&s=hx_rmt->s;const uint32_t prior0=s.event_selected[0],prior1=s.event_selected[1];const int role=hxrFreshProofRole(hx_pcnt,r);hxrSelect(s,r,p,role);
  if(role>=1&&!hxrProofIdentity(hx_pcnt.proof[hx_pcnt.proof_count-1],r,p)){s.terminal|=HXR_IDENTITY;if(s.pending&&s.work[s.current].seq==r.seq)s.work[s.current].reason|=HXR_IDENTITY;else for(auto&e:s.event)if(e.retained&&e.seq==r.seq)e.reason|=HXR_IDENTITY;}
  const bool bad0=!prior0&&s.event_selected[0]&&(!hxRmtPins(3)||!hxRmtSafe(3));
  const bool bad1=!prior1&&s.event_selected[1]&&(!hxRmtPins(4)||!hxRmtSafe(4));
  if(bad0||bad1){s.terminal|=HXR_GPIO;hx_pcnt.clock_blocked=1;if(s.pending&&s.work[s.current].seq==r.seq)s.work[s.current].reason|=HXR_GPIO;else for(auto&e:s.event)if(e.retained&&e.seq==r.seq)e.reason|=HXR_GPIO;} // Same-frame roles share uncertainty; no earlier different-frame event is rewritten.
  if(s.pending&&s.work[s.current].seq==r.seq){s.work[s.current].hook_begin=hook_begin;s.work[s.current].hook_end=micros();} // Loop service copies only after this binding bracket is fixed.
}
static void discardHxRmtBinding(uint32_t seq) {
  if(hx_rmt&&hx_rmt->s.pending&&hx_rmt->s.work[hx_rmt->s.current].seq==seq){hx_rmt->s.work[hx_rmt->s.current].reason|=HXR_NOT_BOUND;hx_rmt->s.terminal|=HXR_NOT_BOUND;}
}
#endif
