#include "telemetry_buffer.hpp"

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

using p0_scheduler_fixture::TelemetryBuffer;

namespace {
void test_concurrent_coherent_capture() {
    constexpr std::size_t workers = 8;
    constexpr std::size_t each = 512;
    constexpr std::size_t total = workers * each;
    TelemetryBuffer<total> buffer;
    std::array<std::thread, workers> threads;
    std::atomic<bool> start_writers{};
    std::atomic<bool> writers_done{};
    std::atomic<std::size_t> observed_during_writes{};
    std::atomic<std::size_t> reader_scans{};
    std::thread reader([&] {
        do {
            for (std::size_t i = 0; i < total; ++i) {
                TelemetryBuffer<total>::Observation observation;
                if (buffer.read(i, observation)) {
                    const auto worker = static_cast<std::size_t>(observation.iterator >> 32);
                    const auto sequence = static_cast<std::size_t>(observation.iterator & 0xffffffffu);
                    assert(worker >= 1 && worker <= workers);
                    assert(observation.thread_id == worker);
                    assert(sequence < each);
                    observed_during_writes.fetch_add(1, std::memory_order_relaxed);
                }
            }
            reader_scans.fetch_add(1, std::memory_order_release);
        } while (!writers_done.load(std::memory_order_acquire));
    });
    while (reader_scans.load(std::memory_order_acquire) == 0) std::this_thread::yield();

    for (std::size_t worker = 0; worker < workers; ++worker) {
        threads[worker] = std::thread([&, worker] {
            while (!start_writers.load(std::memory_order_acquire)) {}
            for (std::size_t sequence = 0; sequence < each; ++sequence) {
                const auto iterator = (static_cast<std::uintptr_t>(worker + 1) << 32) |
                    static_cast<std::uintptr_t>(sequence);
                const auto thread_id = static_cast<std::uint64_t>(worker + 1);
                (void)buffer.capture(iterator, thread_id);
            }
        });
    }
    start_writers.store(true, std::memory_order_release);
    for (auto& thread : threads) thread.join();
    writers_done.store(true, std::memory_order_release);
    reader.join();

    const auto saved = buffer.reserved();
    assert(saved + buffer.dropped() == total);
    assert(observed_during_writes.load(std::memory_order_relaxed) > 0);
    std::array<std::array<bool, each>, workers> seen{};
    for (std::size_t i = 0; i < saved; ++i) {
        TelemetryBuffer<total>::Observation observation;
        assert(buffer.read(i, observation));
        const auto worker = static_cast<std::size_t>(observation.iterator >> 32);
        const auto sequence = static_cast<std::size_t>(observation.iterator & 0xffffffffu);
        assert(worker >= 1 && worker <= workers);
        assert(observation.thread_id == worker);
        assert(sequence < each);
        assert(!seen[worker - 1][sequence]);
        seen[worker - 1][sequence] = true;
    }
    std::size_t unique_seen = 0;
    for (const auto& worker_seen : seen)
        for (const auto value : worker_seen) unique_seen += value;
    assert(unique_seen == saved);
    std::cout << "PASS concurrent coherent capture: " << saved << " saved, "
              << buffer.dropped() << " dropped (" << total << " attempts)\n";
}

void test_saturation_drops_without_reuse() {
    TelemetryBuffer<4> buffer;
    for (std::uintptr_t i = 1; i <= 4; ++i) assert(buffer.capture(i, i + 100));
    assert(!buffer.capture(5, 105));
    assert(!buffer.capture(6, 106));
    assert(buffer.reserved() == 4);
    assert(buffer.dropped() == 2);
    for (std::size_t i = 0; i < 4; ++i) {
        TelemetryBuffer<4>::Observation observation;
        assert(buffer.read(i, observation));
        assert(observation.iterator == i + 1);
        assert(observation.thread_id == i + 101);
    }
    TelemetryBuffer<4>::Observation out_of_range;
    assert(!buffer.read(4, out_of_range));
    std::cout << "PASS saturation drops and preserves original slots\n";
}

void test_concurrent_saturation() {
    constexpr std::size_t workers = 8;
    constexpr std::size_t each = 32;
    constexpr std::size_t total = workers * each;
    constexpr std::size_t capacity = 64;
    TelemetryBuffer<capacity> buffer;
    std::array<std::thread, workers> threads;
    for (std::size_t worker = 0; worker < workers; ++worker) {
        threads[worker] = std::thread([&, worker] {
            for (std::size_t sequence = 0; sequence < each; ++sequence) {
                const auto value = (static_cast<std::uintptr_t>(worker + 1) << 32) |
                    static_cast<std::uintptr_t>(sequence);
                (void)buffer.capture(value, worker + 1);
            }
        });
    }
    for (auto& thread : threads) thread.join();
    const auto saved = buffer.reserved();
    assert(saved + buffer.dropped() == total);
    for (std::size_t i = 0; i < saved; ++i) {
        TelemetryBuffer<capacity>::Observation observation;
        assert(buffer.read(i, observation));
        assert(static_cast<std::uint64_t>(observation.iterator >> 32) == observation.thread_id);
    }
    std::cout << "PASS concurrent saturation: " << saved << " saved, "
              << buffer.dropped() << " dropped (" << total << " attempts)\n";
}

void test_disabled_capture() {
    TelemetryBuffer<2> buffer;
    assert(buffer.capture(0x1234, 42));
    buffer.disable();
    assert(!buffer.enabled());
    assert(!buffer.capture(0x5678, 43));
    assert(buffer.reserved() == 1);
    assert(buffer.dropped() == 0);
    TelemetryBuffer<2>::Observation observation;
    assert(buffer.read(0, observation));
    assert(observation.iterator == 0x1234);
    assert(observation.thread_id == 42);
    assert(!buffer.read(1, observation));
    std::cout << "PASS disabled capture rejects subsequent calls\n";
}

void test_disable_during_writers() {
    constexpr std::size_t workers = 8;
    constexpr std::size_t each = 20000;
    constexpr std::size_t capacity = workers * each + 1;
    auto buffer = std::make_unique<TelemetryBuffer<capacity>>();
    std::atomic<bool> start{};
    std::atomic<std::size_t> ready_workers{};
    std::atomic<bool> release_workers{};
    std::array<std::thread, workers> threads;
    for (auto& thread : threads) {
        thread = std::thread([&] {
            while (!start.load(std::memory_order_acquire)) {}
            (void)buffer->capture(1, 7);
            ready_workers.fetch_add(1, std::memory_order_release);
            while (!release_workers.load(std::memory_order_acquire)) {}
            for (std::size_t i = 1; i < each; ++i) (void)buffer->capture(i + 1, 7);
        });
    }
    start.store(true, std::memory_order_release);
    while (ready_workers.load(std::memory_order_acquire) != workers) std::this_thread::yield();
    assert(buffer->reserved() + buffer->dropped() == workers);
    buffer->disable();
    release_workers.store(true, std::memory_order_release);
    for (auto& thread : threads) thread.join();

    const auto reserved_after_join = buffer->reserved();
    const auto dropped_after_join = buffer->dropped();
    assert(reserved_after_join + dropped_after_join <= workers * each);
    for (std::size_t i = 0; i < 100; ++i) assert(!buffer->capture(i + 1, 8));
    assert(buffer->reserved() == reserved_after_join);
    assert(buffer->dropped() == dropped_after_join);
    std::cout << "PASS disable during active writers: " << reserved_after_join
              << " saved, " << dropped_after_join << " dropped before/at disable\n";
}
} // namespace

int main() {
    test_concurrent_coherent_capture();
    test_saturation_drops_without_reuse();
    test_concurrent_saturation();
    test_disabled_capture();
    test_disable_during_writers();
}
