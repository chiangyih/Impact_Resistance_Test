#ifndef IRT_HX_PCNT_ADAPTER_H // 僅IDF driver配置，不用會改periman歸屬的Arduino RMT。
#define IRT_HX_PCNT_ADAPTER_H // 原25pulse區塊不得包含本檔呼叫。
#include "hx_pcnt_diag.h" // 可主機回歸的固定snapshot與CRC。
#include "hx_attempt_pcnt.h" // 同seq每讀框compact sidecar，原ring另存。
#include <driver/gpio.h> // 明確讀回OE/route/drive/pull。
#include <driver/pulse_cnt.h> // MCU側硬體數位edge，不是ADC端波形。
#include <soc/gpio_sig_map.h> // SIG_GPIO_OUT_IDX只驗原GPIO輸出路由。
#include <soc/io_mux_reg.h> // PIN_FUNC_GPIO與已安裝IDF相同。
#include <esp_idf_version.h> // overflow有效性依賴此精確driver補償契約。
static_assert(ESP_IDF_VERSION == ESP_IDF_VERSION_VAL(5, 5, 5), "PCNT pending-overflow contract requires reviewed IDF 5.5.5"); // 升降core必須重新審查，不默默套用。
static HxPcntDiag hx_pcnt; // 新增固定RAM，不重用trial/cal或既有ring。
static_assert(sizeof(HxPcntDiag) <= 2048, "PCNT public snapshot must remain bounded"); // driver本身另有setup heap配置。
struct HxPcntRuntime { pcnt_unit_handle_t unit[2] = {}; pcnt_channel_handle_t channel[2] = {}; bool enabled[2] = {}, proof_active = false; uint8_t pins[2] = {25,33}; HxPcntProof pending = {}; }; // 全部資源setup一次配置。
static HxPcntRuntime hx_pcnt_rt; // runtime handles不匯出，不讀其他記憶體。
static HxAttemptPcnt hx_attempt_pcnt_pending; static bool hx_attempt_pcnt_prepared = false, hx_attempt_pcnt_active = false; // frame先深拷貝，再讓首LINK hold清硬體counter。
static portMUX_TYPE hx_pcnt_mux = portMUX_INITIALIZER_UNLOCKED; // 只保護overflow latch，不放在SCK train內。
static volatile uint32_t hx_pcnt_ever_overflow = 0; static volatile bool hx_pcnt_irq_closed = false; static uint8_t hx_pcnt_index[2] = {0,1}; // 全epoch不清overflow，seal後callback不改證據。
static bool IRAM_ATTR hxPcntOverflow(pcnt_unit_handle_t, const pcnt_watch_event_data_t *, void *arg) { // 只有limit事件，不做每edge ISR。
  const uint8_t index = *static_cast<uint8_t *>(arg); portENTER_CRITICAL_ISR(&hx_pcnt_mux); if (!hx_pcnt_irq_closed) hx_pcnt_ever_overflow = hx_pcnt_ever_overflow | (1U << index); portEXIT_CRITICAL_ISR(&hx_pcnt_mux); return false; // 不輸出、不配置heap、不碰SCK。
} // 結束overflow latch。
static uint32_t hxPcntOverflowMask() { portENTER_CRITICAL(&hx_pcnt_mux); const uint32_t value = hx_pcnt_ever_overflow; portEXIT_CRITICAL(&hx_pcnt_mux); return value; } // 晚到事件只會使觀測更保守。
static HxPcntPin hxPcntPin(const gpio_io_config_t &p) { // 僅白名單GPIO配置，沒有RAM dump。
  HxPcntPin r; r.function = p.fun_sel; r.signal = p.sig_out; r.drive = p.drv; r.flags = static_cast<uint32_t>(p.pu) | (p.pd << 1) | (p.ie << 2) | (p.oe << 3) | (p.oe_ctrl_by_periph << 4) | (p.oe_inv << 5) | (p.od << 6) | (p.slp_sel << 7); return r; // 設定與bool各欄明確編碼。
} // 結束配置編碼。
static bool hxPcntPins(uint8_t stage) { // 每次成功讀到兩腳才標stage有效。
  gpio_io_config_t a = {}, b = {}; if (gpio_get_io_config(static_cast<gpio_num_t>(hx_pcnt_rt.pins[0]), &a) != ESP_OK || gpio_get_io_config(static_cast<gpio_num_t>(hx_pcnt_rt.pins[1]), &b) != ESP_OK) return false; // 失敗不造全零配置。
  hx_pcnt.pins[stage][0] = hxPcntPin(a); hx_pcnt.pins[stage][1] = hxPcntPin(b); hx_pcnt.stage_mask |= 1U << stage; return true; // BASE/DRIVER/READY/EVENT各自保存。
} // 結束GPIO讀回。
static bool hxPcntSafe(uint8_t stage) { // 失敗清理也允許原IE或追加IE，但任何其他差異都不默默修正。
  return (hx_pcnt.stage_mask & (1U << stage)) && (hxpSamePin(hx_pcnt.pins[0][0], hx_pcnt.pins[stage][0]) || hxpSamePin(hx_pcnt.pins[0][0], hx_pcnt.pins[stage][0], true)) && hxpSamePin(hx_pcnt.pins[0][1], hx_pcnt.pins[stage][1]); // OE/route/drive/pulls必須原樣。
} // 結束配置安全檢查。
static bool hxPcntAllocate() { // 只在setup使用driver heap，正常框/hold不配置或釋放。
  for (uint8_t i = 0; i < 2; ++i) { // 兩unit各一channel，GPIO固定25與33。
    pcnt_unit_config_t config = {}; config.low_limit = -static_cast<int>(HXP_LIMIT); config.high_limit = HXP_LIMIT; config.flags.accum_count = true; // get_count對pending overflow作補償，任何limit都無效。
    if (pcnt_new_unit(&config, &hx_pcnt_rt.unit[i]) != ESP_OK) return false; // 資源不足不猜成功。
    pcnt_chan_config_t channel = {}; channel.edge_gpio_num = hx_pcnt_rt.pins[i]; channel.level_gpio_num = -1; channel.flags.virt_level_io_level = 1; channel.flags.io_loop_back = false; // DATA絕不因loopback啟output。
    if (pcnt_new_channel(hx_pcnt_rt.unit[i], &channel, &hx_pcnt_rt.channel[i]) != ESP_OK) return false; // 此API會暫開pullup，稍後必須還原並比對。
    if (pcnt_channel_set_edge_action(hx_pcnt_rt.channel[i], PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE) != ESP_OK || pcnt_channel_set_level_action(hx_pcnt_rt.channel[i], PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_KEEP) != ESP_OK || pcnt_unit_set_glitch_filter(hx_pcnt_rt.unit[i], nullptr) != ESP_OK) return false; // 不新增未明示glitch filter。
    pcnt_event_callbacks_t callbacks = {}; callbacks.on_reach = hxPcntOverflow; if (pcnt_unit_register_event_callbacks(hx_pcnt_rt.unit[i], &callbacks, &hx_pcnt_index[i]) != ESP_OK || pcnt_unit_add_watch_point(hx_pcnt_rt.unit[i], HXP_LIMIT) != ESP_OK || pcnt_unit_enable(hx_pcnt_rt.unit[i]) != ESP_OK) return false; // limit只使觀測無效。
    hx_pcnt_rt.enabled[i] = true; // enable不等於開始計數，counter此時仍停。
  } // 結束兩unit配置。
  return true; // 真正可用仍需GPIO讀回及正對照。
} // 結束setup資源。
static void initializeHxPcnt(uint8_t sck, uint8_t data) { // 在原HX reset後、共同window前呼叫。
  hx_pcnt_rt.pins[0] = sck; hx_pcnt_rt.pins[1] = data; hx_pcnt.gpio_function = PIN_FUNC_GPIO; hx_pcnt.gpio_signal = SIG_GPIO_OUT_IDX; // 只接受原GPIO輸出功能。
  if (!hxPcntPins(0) || !hxpBaseline(hx_pcnt)) { hx_pcnt.setup_code = 2; hx_pcnt.clock_blocked = 1; return; } // 未確認原配置不啟任何新driver或clock。
  const bool allocated = hxPcntAllocate(); const bool driver_read = hxPcntPins(1); // 即使partial allocation也保存driver後配置。
  const bool pull_sck = gpio_set_pull_mode(static_cast<gpio_num_t>(sck), GPIO_FLOATING) == ESP_OK; const bool pull_data = gpio_set_pull_mode(static_cast<gpio_num_t>(data), GPIO_PULLUP_ONLY) == ESP_OK; // 只還原事先驗證的pull，不重映射route/OE/drive。
  const bool ready_read = hxPcntPins(2); // 實際恢復結果必須核對，不能只報API已呼叫。
  if (!pull_sck || !pull_data || !ready_read || !hxPcntSafe(2)) hx_pcnt.clock_blocked = 1; // 無法確保復原時阻擋HX clock，保留STALE/NA。
  hx_pcnt.setup_code = allocated && driver_read && ready_read && !hx_pcnt.clock_blocked && hxpPinsMatch(hx_pcnt, 2) ? 1 : 3; // 不可用也保留已配置資源至重啟，不在背景反覆重配。
} // 結束初始化與恢復檢查。
static uint8_t hxPcntStartUnit(uint8_t i, HxPcntUnit &u, const HxDiagState *window = nullptr) { // 回傳本窗原因，不修改先前hold的reason。
  const auto pin = static_cast<gpio_num_t>(hx_pcnt_rt.pins[i]); if (!hx_pcnt_rt.enabled[i]) return HXP_SETUP; // 未配置呈NA。
  if (pcnt_unit_clear_count(hx_pcnt_rt.unit[i]) != ESP_OK) { return HXP_API; } u.api_mask |= 1; // CLEAR失敗不假稱呼叫START，不清HX/ADXL警示。
  const uint32_t before = micros(); if (window && !hxdiagCanRetain(*window, before)) return HXP_TIME; // 每個實際START前再檢共同總/後窗，晚到NA。
  u.attempt_mask |= 1; u.start_begin = before; if (pcnt_unit_start(hx_pcnt_rt.unit[i]) == ESP_OK) u.api_mask |= 2; u.start_end = micros(); // 真實API括弧及早期盲區。
  u.level_start_begin = micros(); u.level_start = gpio_get_level(pin); u.level_start_end = micros(); // start後的離散點，不冒充精確起點level。
  return (u.api_mask & 2) ? HXP_NONE : HXP_API; // START失敗仍保留實際括弧。
} // 結束unit開始。
static void hxPcntStopUnit(uint8_t i, HxPcntUnit &u) { // DATA先停，SCK後停，兩counter停止後才容許resume。
  const auto pin = static_cast<gpio_num_t>(hx_pcnt_rt.pins[i]); if (!hx_pcnt_rt.enabled[i]) return; // partial init仍誠實NA。
  u.level_end_begin = micros(); u.level_end = gpio_get_level(pin); u.level_end_end = micros(); // stop之前的離散點，間隙仍未知。
  u.attempt_mask |= 2; u.stop_begin = micros(); if (pcnt_unit_stop(hx_pcnt_rt.unit[i]) == ESP_OK) u.api_mask |= 4; else hx_pcnt.clock_blocked = 1; u.stop_end = micros(); // stop失敗不能讓合法resume污染計數。
  int count = 0; u.attempt_mask |= 4; u.read_begin = micros(); if (pcnt_unit_get_count(hx_pcnt_rt.unit[i], &count) == ESP_OK) { u.api_mask |= 8; u.count = count; } u.read_end = micros(); // 停後讀回包含尚未進ISR的limit補償。
  u.overflow = ((hxPcntOverflowMask() >> i) & 1U) || count < 0 || static_cast<uint32_t>(count) >= HXP_LIMIT; u.valid = hxpUnitValid(u); // 不忽略pending limit或用wrap零值通過。
} // 結束unit停止/讀回。
static void prepareHxPcntProof(const HxDiagState *window = nullptr) { // 最多八個既有read的外側counter正對照，train內零新增。
  if (hx_pcnt.sealed || hx_pcnt.linked || hx_pcnt.setup_code != 1 || hx_pcnt.proof_ready || hx_pcnt.proof_count >= HXP_PROOFS || hx_pcnt_rt.proof_active) return; // 不重做或無限等待。
  hx_pcnt_rt.pending = HxPcntProof(); if (hxPcntStartUnit(0, hx_pcnt_rt.pending.unit[0], window) == HXP_NONE) hxPcntStartUnit(1, hx_pcnt_rt.pending.unit[1], window); hx_pcnt_rt.proof_active = true; // SCK先成功才啟DATA，每個START均檢觀測截止。
} // 結束正對照開始。
static void finishHxPcntProof(uint32_t seq, uint32_t bits, bool accepted) { // 原postcheck/CPU read_end後先停，再進fail hook。
  if (!hx_pcnt_rt.proof_active) return; // READY之後不再對正常框加API。
  hxPcntStopUnit(1, hx_pcnt_rt.pending.unit[1]); hxPcntStopUnit(0, hx_pcnt_rt.pending.unit[0]); hx_pcnt_rt.proof_active = false; // 拒絕框也先關validation，避免與hold重疊。
  hx_pcnt_rt.pending.seq = seq; hx_pcnt_rt.pending.bits24 = bits & 0xFFFFFFU; hx_pcnt_rt.pending.accepted = accepted; hx_pcnt_rt.pending.postcheck = accepted; hxpAddProof(hx_pcnt, hx_pcnt_rt.pending); // 不影響原接受/ERR/校正。
} // 結束正對照保存。
static void startHxPcntHold(uint32_t seq, uint32_t anchor, bool eligible, const HxDiagState &window) { // 僅原HXF首LINK，額外開銷會使early點missed。
  if (hx_pcnt.sealed || hx_pcnt.linked) { return; } const uint32_t now = micros(); // 防止後續LINK覆寫。
  if (hx_pcnt.setup_code == 1 && (!hxPcntPins(3) || !hxpPinsMatch(hx_pcnt, 3))) hx_pcnt.clock_blocked = 1; // 不改初始setup記錄、不偷偷修路由，禁止後續clock。
  if (hxpLink(hx_pcnt, seq, anchor, eligible, now)) { // 只有ready且GPIO同原配置才開始。
    if (!hxdiagCanRetain(window, micros())) { hx_pcnt.mode = HXP_SKIPPED; hx_pcnt.reason = HXP_TIME; } // readback延遲若跨截止，不再啟counter。
    else { hx_pcnt.reason = hxPcntStartUnit(0, hx_pcnt.unit[0], &window); if (hx_pcnt.reason == HXP_NONE) hx_pcnt.reason = hxPcntStartUnit(1, hx_pcnt.unit[1], &window); } // DATA不能先於有效SCK啟動。
  } // 結束可用窗開始。
  hx_pcnt.event_end = micros(); // 與原anchor分開，不假裝2/10us點仍準時。
} // 結束首LINK counter開始。
static void finishHxPcntHold(bool hxf_holding) { // 必須在下一HX attempt之前呼叫。
  if (hxf_holding || hx_pcnt.mode != HXP_RUNNING) { return; } hx_pcnt.finish_begin = micros(); // 只停counter，SCK仍由原程式保持LOW。
  hxPcntStopUnit(1, hx_pcnt.unit[1]); hxPcntStopUnit(0, hx_pcnt.unit[0]); hxpComplete(hx_pcnt, micros()); // DATA先停且SCK後停，不能混入resume脈衝。
} // 結束真正hold觀察。
static void sealHxPcnt(bool link_frozen, bool hxf_holding) { // 只在既有LINK槽/profile固定時封存。
  finishHxPcntHold(hxf_holding); if (!link_frozen || hxf_holding || hx_pcnt.sealed) return; // 不匯出活動counter。
  portENTER_CRITICAL(&hx_pcnt_mux); hx_pcnt_irq_closed = true; portEXIT_CRITICAL(&hx_pcnt_mux); hxpSeal(hx_pcnt); // overflow在stop後get_count已納入，晚到callback不可改CRC。
} // 結束不可變snapshot。
static void prepareHxAttemptPcnt(bool eligible, const HxDiagState &window) { // 原DATA ready gate後、原read括弧前，包含CLEAR的開銷另存。
  if (hx_attempt_pcnt_active || hx_pcnt_rt.proof_active || hx_pcnt.mode == HXP_RUNNING) return; // 防止重入或hold中清同一硬體，正常poll另有HOLD優先守衛。
  hx_attempt_pcnt_prepared = hx_attempt_pcnt_active = false; if (window.mode == HXD_FROZEN || hx_pcnt.sealed) return; // 兩槽結束後停止每框counter介入。
  hx_attempt_pcnt_pending = HxAttemptPcnt(); hx_attempt_pcnt_pending.prepare_begin = micros(); hx_attempt_pcnt_prepared = true; // 不把舊frame複製到新seq。
  hx_attempt_pcnt_pending.policy = (hx_pcnt.setup_code == 1 ? HXAP_SETUP : 0U) | (hx_pcnt.proof_ready ? HXAP_PROOF : 0U) | (eligible ? HXAP_ELIGIBLE : 0U); // API有效與原accepted分開。
  uint8_t reason = !eligible ? HXP_BUSY : hx_pcnt.setup_code != 1 ? HXP_SETUP : hx_pcnt.clock_blocked ? HXP_API : !hxdiagCanRetain(window,hx_attempt_pcnt_pending.prepare_begin) ? HXP_TIME : HXP_NONE; // 不延長共同期限。
  if (reason == HXP_NONE && !hx_pcnt.proof_ready) { // 原四筆完整正對照只啟停一次，不疊加counter。
    prepareHxPcntProof(&window); hx_attempt_pcnt_active = hx_pcnt_rt.proof_active; if (hx_attempt_pcnt_active) hx_attempt_pcnt_pending.policy |= HXAP_PROOF_SOURCE; if (!hx_attempt_pcnt_active) reason = HXP_NO_PROOF; // 診斷來源旗標只標實際proof，不把滿八筆後的路徑誤作新對照；guard不變。
  } else if (reason == HXP_NONE) { // 已驗counter路徑後，每合法IDLE讀框都計數。
    hx_pcnt_rt.pending = HxPcntProof(); reason = hxPcntStartUnit(0,hx_pcnt_rt.pending.unit[0],&window); if (reason == HXP_NONE) reason = hxPcntStartUnit(1,hx_pcnt_rt.pending.unit[1],&window); hx_attempt_pcnt_active = true; // SCK先起，DATA後起，所有呼叫都在train外。
  } // 結束本框準備。
  if (hx_attempt_pcnt_active) { hx_attempt_pcnt_pending.policy |= HXAP_PRESENT; } hx_attempt_pcnt_pending.policy |= static_cast<uint32_t>(reason) << 16; // unknown不填valid0計數。
} // 結束每框開始。
static void finishHxAttemptPcnt(uint32_t seq, uint32_t bits, bool accepted) { // 原postcheck/read_end後，先固定frame再允許首LINK hold。
  if (!hx_attempt_pcnt_prepared) return; // frozen後不做額外API。
  if (hx_pcnt_rt.proof_active) finishHxPcntProof(seq,bits,accepted); // 原完整正對照保留，避免early LINK與validation重疊。
  else if (hx_attempt_pcnt_active) { hxPcntStopUnit(1,hx_pcnt_rt.pending.unit[1]); hxPcntStopUnit(0,hx_pcnt_rt.pending.unit[0]); } // DATA先停、SCK後停，拒絕框也保留count。
  if (hx_attempt_pcnt_active) { hx_attempt_pcnt_pending.unit[0] = hxapPack(hx_pcnt_rt.pending.unit[0]); hx_attempt_pcnt_pending.unit[1] = hxapPack(hx_pcnt_rt.pending.unit[1]); if ((hx_attempt_pcnt_pending.unit[0].flags & 6) == 6 && (hx_attempt_pcnt_pending.unit[1].flags & 6) == 6 && hxapEnclosed(hx_attempt_pcnt_pending)) hx_attempt_pcnt_pending.policy |= HXAP_ENCLOSED; } // 實際API失敗不冒充包絡。
  if (!(hx_attempt_pcnt_pending.policy >> 16)) { uint8_t reason = !(hx_attempt_pcnt_pending.policy & HXAP_PROOF) ? HXP_NO_PROOF : HXP_NONE; if (reason == HXP_NONE && (!hxapUnitValid(hx_attempt_pcnt_pending.unit[0]) || !hxapUnitValid(hx_attempt_pcnt_pending.unit[1]))) reason = ((hx_attempt_pcnt_pending.unit[0].flags | hx_attempt_pcnt_pending.unit[1].flags) & 128) ? HXP_OVERFLOW : (hx_attempt_pcnt_pending.unit[0].count > static_cast<int32_t>(HXP_VALID_MAX) || hx_attempt_pcnt_pending.unit[1].count > static_cast<int32_t>(HXP_VALID_MAX)) ? HXP_RANGE : HXP_API; if (reason == HXP_NONE && !(hx_attempt_pcnt_pending.policy & HXAP_ENCLOSED)) reason = HXP_TIME; hx_attempt_pcnt_pending.policy |= static_cast<uint32_t>(reason) << 16; } // 保留無proof/API/range等原因，不拿DATA預期或RAW接受作濾除。
  if (hx_pcnt.clock_blocked) { hx_attempt_pcnt_pending.policy |= HXAP_GUARD; } hx_attempt_pcnt_pending.policy |= HXAP_FINISHED; hx_attempt_pcnt_pending.seq = seq; hx_attempt_pcnt_pending.finish_end = micros(); hx_attempt_pcnt_active = false; // 開銷含私有counter啟停/深拷貝，不改HX accepted。
} // 結束每框固定pending。
#endif // 結束PCNT adapter。
