#ifndef IRT_HX_ATTEMPT_STORAGE_H
#define IRT_HX_ATTEMPT_STORAGE_H
#include "hx_attempt_pcnt.h"
#include <new>

enum HxAttemptAllocState : uint8_t { HXAS_NOT_TRIED, HXAS_READY, HXAS_FIRST_FAILED, HXAS_SECOND_FAILED, HXAS_RESERVE_FAILED };
struct HxAttemptStorage {
  HxAttemptPcntRing *any = nullptr, *link = nullptr;
  uint8_t state = HXAS_NOT_TRIED, attempts = 0;
  bool ready() const { return state == HXAS_READY && any && link; }
};
const uint32_t HXAS_BYTES = 2 * sizeof(HxAttemptPcntRing), HXAS_MIN_REMAINING = 65536;
struct HxHeapSnapshot { uint32_t free_bytes = 0, largest = 0, minimum = 0; };
static_assert(HXAS_BYTES == 40960, "Sidecar capacity must stay unchanged");

// Setup-only transaction: publish neither pointer until both blocks and the reserve exist.
// A failed attempt is sticky. RAW rings remain independent and callers must report degradation.
template<typename Allocate, typename Release, typename Available>
inline bool hxasAllocate(HxAttemptStorage &s, Allocate allocate, Release release, Available available) {
  if (s.state != HXAS_NOT_TRIED) return s.ready();
  ++s.attempts;
  void *a = allocate(sizeof(HxAttemptPcntRing));
  if (!a) { s.state = HXAS_FIRST_FAILED; return false; }
  void *b = allocate(sizeof(HxAttemptPcntRing));
  if (!b) { release(a); s.state = HXAS_SECOND_FAILED; return false; }
  if (available() < HXAS_MIN_REMAINING) { release(b); release(a); s.state = HXAS_RESERVE_FAILED; return false; }
  s.any = new (a) HxAttemptPcntRing(); s.link = new (b) HxAttemptPcntRing();
  s.state = HXAS_READY; return true;
}

inline void hxasObserve(HxDiagState &raw, HxAttemptPcntRing *side, const HxDiagRecord &r, const HxAttemptPcnt &p) {
  if (side) hxapObserveShared(raw, *side, r, p);
  else hxdiagObserveShared(raw, r); // Never lose RAW solely because optional sidecar allocation failed.
}
#endif
