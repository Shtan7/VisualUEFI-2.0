#include "common.hpp"
#include "uefi.hpp"
#include <intrin.h>

extern "C"
{
#include <Pi/PiMultiPhase.h>
#include <Protocol/MpService.h>
#include <Uefi/UefiBaseType.h>
#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>
}

namespace hh::common
{
  spinlock_guard::spinlock_guard(volatile long* lock) noexcept : lock_{ lock }
  {
    lock_spinlock();
  }

  spinlock_guard::~spinlock_guard() noexcept
  {
    if (lock_ != nullptr)
    {
      unlock();
    }
  }

  bool spinlock_guard::try_lock() noexcept
  {
    return (!(*lock_) && !_interlockedbittestandset(lock_, 0));
  }

  void spinlock_guard::unlock() noexcept
  {
    *lock_ = 0;
  }

  spinlock_guard::spinlock_guard(spinlock_guard&& obj) noexcept
  {
    lock_ = obj.lock_;
    obj.lock_ = nullptr;
  }

  spinlock_guard& spinlock_guard::operator=(spinlock_guard&& obj) noexcept
  {
    if (lock_ != nullptr)
    {
      unlock();
    }

    lock_ = obj.lock_;
    obj.lock_ = nullptr;

    return *this;
  }

  void spinlock_guard::lock_spinlock() noexcept
  {
    uint32_t wait = 1;

    while (!try_lock())
    {
      for (uint32_t j = 0; j < wait; j++)
      {
        _mm_pause();
      }

      if (wait * 2 > max_wait_)
      {
        wait = max_wait_;
      }
      else
      {
        wait *= 2;
      }
    }
  }
}
