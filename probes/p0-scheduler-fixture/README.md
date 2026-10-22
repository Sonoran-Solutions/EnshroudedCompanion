# P0 scheduler telemetry fixture

This is a **host-only fixture primitive**. It is not linked into ShroudForge,
does not load a game profile, and has no process-memory access, loader, hook,
patch, or action registration. It captures only two caller-supplied values:
an opaque `uintptr_t` iterator identity and a `uint64_t` thread ID. The thread ID
is not obtained by the fixture.

`TelemetryBuffer<Capacity>` is fixed-capacity. Each successful atomic
reservation owns one slot permanently. Reservations use at most eight strong
CAS attempts and saturate at capacity, so the reservation counter cannot wrap
or permit slot reuse. Once full, subsequent capture attempts increment a drop
counter. Contention that exhausts the eight attempts can also drop an
observation while capacity remains; this bounds callback work. Capture
performs no heap allocation, lock acquisition, pointer dereference, logging,
OS call, or wait.
Atomic fields plus a release-published ready flag ensure readers see coherent
pairs. Atomics are compile-time required to be always lock-free on the target.
`disable()` blocks subsequent calls that observe the flag; a caller that raced
past the flag may finish. The buffer therefore must outlive any in-flight
caller (a future runtime probe should pin it through process exit).

## Fixture checks

- 8 concurrent writers publish 4096 unique iterator/thread pairs while a
  reader repeatedly inspects slots. Successes plus drops account for all
  enabled capture attempts; saved pairs are coherent and unique.
- Sequential full-buffer and concurrent saturation tests verify no overwrite,
  attempt accounting, and preservation of prior records.
- Disabled capture is rejected without changing counters. A separate test
  holds all writer threads between capture calls, disables capture while those
  callers are active, then releases them and verifies later calls cannot
  change either counter. This checks disable behavior around active callers;
  it does not suspend a thread inside `capture()`.

## Reproduction and result

From the repository root, with host `g++` 15.2.0:

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -pthread \
  probes/p0-scheduler-fixture/test_telemetry_buffer.cpp \
  -o /tmp/p0-scheduler-fixture
timeout 60s /tmp/p0-scheduler-fixture

g++ -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -pthread \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  probes/p0-scheduler-fixture/test_telemetry_buffer.cpp \
  -o /tmp/p0-scheduler-fixture-asan
timeout 60s /tmp/p0-scheduler-fixture-asan

g++ -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -pthread \
  -fsanitize=thread -fno-omit-frame-pointer \
  probes/p0-scheduler-fixture/test_telemetry_buffer.cpp \
  -o /tmp/p0-scheduler-fixture-tsan
timeout 60s /tmp/p0-scheduler-fixture-tsan
```

All three checks exited 0. The observed outputs were:

```text
normal:
PASS concurrent coherent capture: 4036 saved, 60 dropped (4096 attempts)
PASS saturation drops and preserves original slots
PASS concurrent saturation: 64 saved, 192 dropped (256 attempts)
PASS disabled capture rejects subsequent calls
PASS disable during active writers: 8 saved, 0 dropped before/at disable

ASan/UBSan:
PASS concurrent coherent capture: 4096 saved, 0 dropped (4096 attempts)
PASS saturation drops and preserves original slots
PASS concurrent saturation: 64 saved, 192 dropped (256 attempts)
PASS disabled capture rejects subsequent calls
PASS disable during active writers: 8 saved, 0 dropped before/at disable

TSan:
PASS concurrent coherent capture: 3970 saved, 126 dropped (4096 attempts)
PASS saturation drops and preserves original slots
PASS concurrent saturation: 64 saved, 192 dropped (256 attempts)
PASS disabled capture rejects subsequent calls
PASS disable during active writers: 8 saved, 0 dropped before/at disable
```

The saved/drop split under contention can vary while attempt accounting stays
exact.

This validates the host fixture's data structure only. It does not prove that a
real callback has the expected ABI, register arguments, stack alignment, phase,
or thread-ID source, nor that trampoline replay is safe. Before any runtime
admission, separately establish exact post-update image identity, unique
signature and expected bytes, decoded instruction boundaries and relocation,
site-specific ABI/stack/unwind behavior, multi-thread call safety, telemetry
buffer/module lifetime, and refusal paths. A diagnostic descriptor/target must
remain separate from production profile selection and must not set
`runtimeLayoutValidated`. Runtime disable should stop telemetry only; leave
hook/trampoline memory mapped until process exit and restart for cleanup.
