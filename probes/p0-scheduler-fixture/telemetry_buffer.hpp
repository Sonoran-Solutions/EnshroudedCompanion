#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace p0_scheduler_fixture {

// Host-only fixture primitive. This is not wired into ShroudForge or a game
// loader. Each successful reservation permanently owns one slot; there is no
// wraparound, reuse, lock, allocation, logging, or pointer dereference.
template <std::size_t Capacity>
class TelemetryBuffer final {
    static_assert(Capacity > 0);
    static_assert(std::atomic<std::uintptr_t>::is_always_lock_free);
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
    static_assert(std::atomic<std::size_t>::is_always_lock_free);
    static_assert(std::atomic<bool>::is_always_lock_free);

public:
    struct Observation {
        std::uintptr_t iterator{};
        std::uint64_t thread_id{};
    };

    TelemetryBuffer() = default;
    TelemetryBuffer(const TelemetryBuffer&) = delete;
    TelemetryBuffer& operator=(const TelemetryBuffer&) = delete;

    // The caller supplies the ID. The capture path stores the two raw values
    // only; it does not inspect the iterator or query any OS/game API.
    bool capture(std::uintptr_t iterator, std::uint64_t thread_id) noexcept {
        if (!enabled_.load(std::memory_order_acquire)) return false;

        std::size_t index = reserved_.load(std::memory_order_relaxed);
        bool claimed = false;
        // Keep callback work bounded even under heavy contention. A failed
        // strong CAS may drop this observation; slots are never reused.
        for (unsigned attempt = 0; attempt < kReservationAttempts; ++attempt) {
            if (index >= Capacity) break;
            if (reserved_.compare_exchange_strong(index, index + 1,
                    std::memory_order_relaxed, std::memory_order_relaxed)) {
                claimed = true;
                break;
            }
        }
        if (!claimed) {
            dropped_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }

        auto& slot = slots_[index];
        slot.iterator.store(iterator, std::memory_order_relaxed);
        slot.thread_id.store(thread_id, std::memory_order_relaxed);
        slot.ready.store(true, std::memory_order_release);
        return true;
    }

    // Disabling prevents captures that observe the false flag. A capture that
    // already passed the flag may finish; owners must retain this buffer until
    // all possible callers have stopped (or until process exit).
    void disable() noexcept { enabled_.store(false, std::memory_order_release); }

    bool enabled() const noexcept { return enabled_.load(std::memory_order_acquire); }

    // Snapshot a published slot without waiting. A reserved but unfinished
    // slot returns false and may be queried again later.
    bool read(std::size_t index, Observation& output) const noexcept {
        if (index >= Capacity) return false;
        const auto& slot = slots_[index];
        if (!slot.ready.load(std::memory_order_acquire)) return false;
        output.iterator = slot.iterator.load(std::memory_order_relaxed);
        output.thread_id = slot.thread_id.load(std::memory_order_relaxed);
        return true;
    }

    std::size_t reserved() const noexcept {
        const auto value = reserved_.load(std::memory_order_acquire);
        return value < Capacity ? value : Capacity;
    }

    std::size_t dropped() const noexcept { return dropped_.load(std::memory_order_relaxed); }

private:
    static constexpr unsigned kReservationAttempts = 8;

    struct Slot {
        std::atomic<std::uintptr_t> iterator{};
        std::atomic<std::uint64_t> thread_id{};
        std::atomic<bool> ready{};
    };

    std::array<Slot, Capacity> slots_{};
    std::atomic<bool> enabled_{true};
    std::atomic<std::size_t> reserved_{};
    std::atomic<std::size_t> dropped_{};
};

} // namespace p0_scheduler_fixture
