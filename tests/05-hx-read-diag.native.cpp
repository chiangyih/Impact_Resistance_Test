// Native C++ checks execute the actual public header, without Arduino/COM or credentials.
#include "../05-test/hx_read_diag.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

static HxDiagRecord rec(uint32_t i, bool accepted = true, int32_t raw = -160000) {
  HxDiagRecord r = {};
  r.start_us = i * 1000; r.end_us = r.start_us + 125; r.bits24 = static_cast<uint32_t>(raw) & 0xFFFFFF;
  r.raw = raw; r.rx_before = i - 1; r.rx_after = accepted ? i : i - 1;
  r.accepted = accepted; r.dout_after25 = accepted; r.pulses_issued = 25; r.context_flags = 1;
  r.err_after = accepted ? 0 : 1;
  return r;
}
static const HxDiagRecord &at(const HxDiagState &s, uint16_t i) { return s.records[hxdiagIndex(s, i)]; }
static uint32_t referenceCrc(const std::vector<uint8_t> &bytes) {
  uint32_t table[256];
  for (uint32_t i = 0; i < 256; ++i) { uint32_t c = i; for (unsigned b = 0; b < 8; ++b) c = c & 1 ? 0xEDB88320U ^ (c >> 1) : c >> 1; table[i] = c; }
  uint32_t crc = 0xFFFFFFFF;
  for (uint8_t b : bytes) crc = table[(crc ^ b) & 255] ^ (crc >> 8);
  return crc ^ 0xFFFFFFFF;
}
int main() {
  unsigned passed = 0;
  {
    HxDiagState s; hxdiagBegin(s, 0); for (uint32_t i = 1; i <= 400; ++i) hxdiagObserve(s, rec(i));
    assert(sizeof(HxDiagRecord) == 56 && sizeof(s.records) == 14336);
    assert(s.count == 127 && s.pre_overwritten == 273 && at(s, 0).seq == 274 && at(s, 126).seq == 400);
    ++passed;
  }
  {
    HxDiagState s; hxdiagBegin(s, 0); for (uint32_t i = 1; i <= 200; ++i) hxdiagObserve(s, rec(i));
    hxdiagObserve(s, rec(201, false, -594)); for (uint32_t i = 202; i <= 329; ++i) hxdiagObserve(s, rec(i));
    assert(s.mode == HXD_FROZEN && s.count == 256 && s.pre_count == 127 && s.post_count == 128);
    assert(s.trigger_seq == 201 && s.trigger_flags == HXD_LINK && s.freeze_reason == HXD_POST_FULL);
    assert(at(s, 127).raw == -594 && at(s, 127).accepted == 0 && at(s, 255).seq == 329);
    HxDiagRecord snapshot[256]; std::memcpy(snapshot, s.records, sizeof(snapshot));
    hxdiagObserve(s, rec(330)); assert(s.omitted_after_freeze == 1 && std::memcmp(snapshot, s.records, sizeof(snapshot)) == 0);
    ++passed;
  }
  {
    HxDiagState s; hxdiagBegin(s, 0); hxdiagObserve(s, rec(1, false)); const uint32_t first = s.trigger_us;
    hxdiagObserve(s, rec(2, false)); hxdiagTick(s, first + HXD_POST_US - 1); assert(s.mode == HXD_POST_CAPTURE);
    hxdiagTick(s, first + HXD_POST_US); assert(s.freeze_reason == HXD_POST_TIMEOUT && s.post_count == 1 && s.trigger_seq == 1);
    ++passed;
  }
  {
    HxDiagState s; hxdiagBegin(s, 100); hxdiagBegin(s, 200); assert(s.start_us == 100);
    hxdiagTick(s, 100 + HXD_WINDOW_US); assert(s.freeze_reason == HXD_WINDOW_LIMIT && s.count == 0);
    const uint32_t frozen = s.freeze_us; hxdiagTick(s, frozen + 100); assert(s.freeze_us == frozen);
    ++passed;
  }
  {
    HxDiagState s; hxdiagBegin(s, 0); HxDiagRecord r = rec(1, false); r.start_us = HXD_WINDOW_US - 1000; r.end_us = r.start_us + 125;
    hxdiagObserve(s, r); hxdiagTick(s, HXD_WINDOW_US); assert(s.freeze_reason == HXD_WINDOW_LIMIT && s.post_count == 0);
    ++passed;
  }
  {
    HxDiagState s; hxdiagBegin(s, 0); hxdiagObserve(s, rec(1)); hxdiagObserve(s, rec(2, false, -156)); hxdiagObserve(s, rec(3));
    assert(at(s, 2).flags == 0 && at(s, 1).bits24 == 0xFFFF64);
    HxDiagRecord r = rec(4, true, -594); hxdiagObserve(s, r); r.raw = 0;
    assert(at(s, 3).raw == -594 && at(s, 3).flags == HXD_JUMP && s.trigger_flags == HXD_LINK);
    ++passed;
  }
  {
    HxDiagState s; hxdiagBegin(s, 0); hxdiagObserve(s, rec(1)); hxdiagObserve(s, rec(2, true, 8388607));
    assert(s.trigger_flags == (HXD_JUMP | HXD_SAT) && at(s, 1).accepted == 1);
    ++passed;
  }
  {
    HxDiagState s; hxdiagBegin(s, 0xFFF00000); s.attempts = 0xFFFFFFFF;
    HxDiagRecord r = rec(1, true, 0); r.start_us = 0xFFF00100; r.end_us = 0xFFF00180; hxdiagObserve(s, r);
    assert(s.seq_wrapped && at(s, 0).seq == 0 && s.accepted_seen);
    hxdiagTick(s, static_cast<uint32_t>(0xFFF00000U + HXD_WINDOW_US)); assert(s.freeze_reason == HXD_WINDOW_LIMIT);
    ++passed;
  }
  {
    HxDiagState s; hxdiagBegin(s, 0); s.mode = HXD_POST_CAPTURE; s.count = HXD_CAPACITY;
    hxdiagObserve(s, rec(1)); assert(s.overflow && s.count == HXD_CAPACITY && s.freeze_reason == HXD_OVERFLOW);
    ++passed;
  }
  {
    const HxDiagRecord r = rec(3, false, -156);
    const uint32_t words[] = {r.seq,r.start_us,r.end_us,r.attempt_gap_us,r.last_accept_ms_before,r.bits24,static_cast<uint32_t>(r.raw),r.rx_before,r.rx_after,r.err_before,r.err_after,r.flags,r.cal_count_after,r.phase_after,r.cal_mode_after,r.accepted,r.dout_after25,r.pulses_issued,r.context_flags};
    std::vector<uint8_t> bytes; for (uint32_t w : words) for (unsigned i = 0; i < 4; ++i) bytes.push_back(static_cast<uint8_t>(w >> (i * 8)));
    assert(bytes.size() == 76 && hxdiagCrc(r) == referenceCrc(bytes));
    const std::vector<uint8_t> known = {'1','2','3','4','5','6','7','8','9'}; assert(referenceCrc(known) == 0xCBF43926);
    ++passed;
  }
  std::printf("%u native C++ HX diagnostic cases passed; record=%zu ring=%zu bytes. No GPIO/timing validation.\n", passed, sizeof(HxDiagRecord), sizeof(HxDiagState::records));
}
