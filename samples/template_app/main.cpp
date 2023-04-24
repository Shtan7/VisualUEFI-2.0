extern "C"
{
#include "..\..\edk2 libs\vshacks.h"
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/ShellLib.h>
}

#include "uefi.hpp"
#include <new>
#include "memory_manager.hpp"
#include "type_info.hpp"
#include "common.hpp"
#include "globals.hpp"
#include <vector>

extern "C" EFI_GUID gEfiSampleDriverProtocolGuid = EFI_SAMPLE_DRIVER_PROTOCOL_GUID;

// We run on any UEFI Specification
extern "C" const UINT32 _gUefiDriverRevision = 0;

// Our name
extern "C" CHAR8 * gEfiCallerBaseName = const_cast<CHAR8*>("UEFI template app");

extern "C" EFI_STATUS EFIAPI UefiUnload(IN EFI_HANDLE ImageHandle)
{
  // This code should be compiled out and never called 

  return EFI_SUCCESS;
}

using namespace hh;

extern "C" EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE * SystemTable)
{
  dead_loop();

  globals::mem_manager = new tlsf_allocator{};

  {
    std::vector<int> nums;

    for (std::size_t j = 0; j < 1000; j++)
    {
      nums.push_back(j);
    }

    for (auto elem : nums)
    {
      Print(L"%d\n"_w, elem);
    }

    try
    {
      throw std::exception{ "test exception" };
    }
    catch (std::exception& e)
    {
      Print(L"%a\n"_w, e.what());
    }
  }

  delete globals::mem_manager;

  return EFI_SUCCESS;
}
