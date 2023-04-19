#pragma once

namespace hh
{
  class memory_manager;

  namespace globals
  {
    inline bool boot_state = true;
    inline memory_manager* mem_manager = {};
    extern "C" unsigned char __ImageBase;
  }
}
