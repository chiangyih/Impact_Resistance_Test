# RMT1 public candidate and device verification boundary

Version: `v1-layout2-fifo-diag1-wifi1-d220-right-optical1-hxread1-hxfail5-pcnt2m1-rmt1`.

The original 25-pulse source is unchanged. Its LF-normalized source SHA256 is
`b00c0e5bbe74c37ca753ff248a1d03dba79a0c01d6e21f547a0864412eb55bb5`, independently compared with the saved PCNT2m1 source. RAW/trial/calibration capacities and the 40960-byte PCNT sidecars are unchanged. RMT adds one setup-only INTERNAL/8BIT heap allocation, at most 4096 bytes. Native 64-bit sizes: state4016/runtime4080; these are host sizes, not measured device allocation.

Two IDF RX channels observe GPIO25/33 at requested10MHz/APB, no DMA, no input filter, 1ms idle target. Driver-computed resolution and channel IDs are read through the reviewed IDF5.5.5 exported C ABI. The adapter comment naming `rmt_private.h` is a **source-location erratum**: the actual definitions are in [rmt_common.c, lines249–260](https://raw.githubusercontent.com/espressif/esp-idf/v5.5.5/components/esp_driver_rmt/src/rmt_common.c). The two symbols were confirmed with `xtensa-esp-elf-nm -g` in the installed platform archive. Neither this readback nor a10MHz request measures oscillator accuracy. The exact IDF guard and final Arduino link remain mandatory.

The reviewed [rmt_rx.c](https://raw.githubusercontent.com/espressif/esp-idf/v5.5.5/components/esp_driver_rmt/src/rmt_rx.c) enables GPIO input/pullup; loopback is false and no GPIO output route is acquired. BASE/DRIVER/READY plus first-LINK/first-DATA-event snapshots preserve output enable, route, drive and pull. Both pulls are restored independently. An unknown or changed output configuration sets the existing sticky HX clock guard.

The ISR copies only bounded callback metadata under a critical section. It records software context, buffer identity, count, `is_last`, callback CPU time and duplicate count. These are **not hardware sequence tags**. A job is reused only after both callbacks and loop-side binding/copy finish. Two alternating working buffers detect callbacks pointing to the previous buffer. Timeout or uncertain API/callback state prevents rearming until reboot; the buffers and callback contexts remain alive. Terminal means no further RMT arm, not proof that classic ESP32 RX physically stopped. No asynchronous-stop assumption is used.

A pending job never delays an original HX attempt. A following attempt marks overlap and skips new RMT instrumentation. A frame not appended to the RAW ring is explicitly NOT_BOUND and settles when callbacks complete or the timeout expires. It cannot hold the export open forever or invent a RAW row.

The first LINK and first compatible-boundary PCNT DATA-model difference are retained even when their RMT trace is missing/invalid. An independent accepted/counter-consistent reference may retain an uncertain finite waveform; it does not certify ADC truth. Embedded RAW comes from the actual appended LINK-ring canonical row, including flags/gap/context. All intersections with retained RAW/PCNT rows are checked by the parser. Two incomplete units' active user buffers are never copied. Late callbacks cannot rewrite frozen events or metadata.

Schema5 requires the complete old RAW/PCNT bundle plus one RMT block:64 metadata words and3×190 trace words,634 words total, row and whole-block CRC32. A missing RMT heap allocation still exports explicit setup2/empty events. Failed PCNT sidecar allocation preserves the existing explicit schema3 RAW fallback. Current and historical synthetic fixtures are kept separate.

The IDF callback does not expose RX_ERROR; the driver clears it internally. A successful callback and less than64 symbols therefore do not prove that every physical edge was observed. Initial LOW can be omitted and an idle terminal can be represented by a zero-period marker; no1ms duration is invented for that marker. Endpoint runs are censored. All positive encoded HIGH segments remain visible, including uncertain/capacity-limited prefixes. The two receivers have separate start brackets and unknown precise offset: no cross-channel100ns phase claim is made. MCU digital observations cannot establish HX-end voltage, supply quality or valid clocks at the far end.

## Required device checks before using the evidence

- Confirm the exact final source hashes, original FQBN/core, target identity, four-section write/hash verification and natural boot/version; keep USB connected. The early credential-free public SDK probe was successful while the candidate was still being edited and is not a final deployment build.
- Read setup1, allocated bytes≤4096, distinct channel IDs, driver resolution10000000 for both, GPIO snapshots and PCNT positive controls. Record post-allocation and WiFi-CONNECTED current/largest/minimum heap. Host sizes and startup reserve are not runtime or WiFi validation.
- Collect ordinary read-only QC, RAW/RX/AGE/error/FIFO counters and OLED/RGB evidence. Root alone owns COM3. No extra reset merely to recover a missed banner.
- Observe receiver callback/count/flags, safe-copy masks, preparation/binding/API brackets and any terminal reason. A shape flag is a prefix limitation, not proof of an HX cause. Confirm the actual ESP32 endpoint encoding against the accepted/counter reference.
- Wait for natural freeze and settled RMT state, then export through the schema5 overall END. Verify all RAW, sidecar, HXF, PCNT, RMT row/block CRCs and canonical intersections. DUMP is synchronous and can produce stale/FIFO observations; keep those separate from pre-DUMP HX events.
- Keep missing, timeout, overflow-unknown and endpoint/cross-channel blind intervals explicit. Do not repair RAW or declare calibration eligible because a later window has no errors. CAL366 still requires the actual HX anomaly and measurement-quality prerequisites to be resolved.

No COM operation, credential read, wiring/power change or flash was performed by Gibbs while preparing this candidate.
