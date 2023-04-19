#pragma once
#include "delete_constructors.hpp"
#include <cstdint>

namespace hh::common
{
  constexpr uint32_t page_size = 0x1000;

  // RAII spinlock.
  class spinlock_guard : non_copyable
  {
  private:
    static constexpr uint32_t max_wait_ = 65536;
    volatile long* lock_;

  private:
    bool try_lock() noexcept;
    void lock_spinlock() noexcept;
    void unlock() noexcept;

  public:
    spinlock_guard(spinlock_guard&&) noexcept;
    spinlock_guard& operator=(spinlock_guard&&) noexcept;
    explicit spinlock_guard(volatile long* lock) noexcept;
    ~spinlock_guard() noexcept;
  };
}
