#include "efi_stub.hpp"
#include <cstdint>
#include <intrin.h>

namespace hh
{
  void bug_check(const bug_check_codes code, const uint64_t arg0, const uint64_t arg1,
    const uint64_t arg2, const uint64_t arg3) noexcept
  {
    // add your own panic handler here
    __halt();
  }
}
