#include "exc_common.hpp"
#include <Windows.h>

namespace exc
{
  struct gs_handler_data
  {
    static constexpr uint32_t COOKIE_OFFSET_MASK = ~7u;

    uint32_t cookie_offset;
    int32_t aligned_base_offset;
    int32_t alignment;
  };

  struct handler_entry
  {
    uint32_t try_rva_low;
    uint32_t try_rva_high;

    union
    {
      relative_virtual_address<int32_t(void* ptrs, uint64_t frame_ptr)> filter;
      relative_virtual_address<void(bool, uint64_t frame_ptr)> unwinder;
    };

    uint32_t target_rva;
  };

  struct handler_data
  {
    uint32_t entry_count;
    handler_entry entries[1];
  };

  struct eh_node_catch
  {
    byte* primary_frame_ptr;
  };

  struct catch_handler
  {
    catch_flag adjectives;
    relative_virtual_address<const type_info> type_desc;
    relative_virtual_address<void> catch_object_offset;
    relative_virtual_address<const byte> handler;
    relative_virtual_address<const eh_node_catch> node_offset;
  };

  struct try_block
  {
    /*0x00*/ int32_t try_low;
    /*0x04*/ int32_t try_high;
    /*0x08*/ int32_t catch_high;
    /*0x0c*/ int32_t catch_count;
    /*0x10*/ relative_virtual_address<const catch_handler> catch_handlers;
  };

  struct eh_region
  {
    relative_virtual_address<const byte> first_ip;
    int32_t state;
  };

  struct alignas(8) eh_node
  {
    int32_t state;
    int32_t reserved;  // unused now
  };

  struct unwind_graph_edge
  {
    int32_t next;
    relative_virtual_address<const byte> cleanup_handler;
  };

  union eh_flag
  {
    struct
    {
      uint8_t compiled_with_ehs : 1;
      uint8_t reserved1 : 1;
      uint8_t is_noexcept : 1;
    };

    uint8_t all;
  };

  struct function_eh_info
  {
    /*0x00*/ uint32_t magic;  // MSVC's magic number
    /*0x04*/ uint32_t state_count;
    /*0x08*/ relative_virtual_address<const unwind_graph_edge> unwind_graph;
    /*0x0c*/ int32_t try_block_count;
    /*0x10*/ relative_virtual_address<const try_block> try_blocks;
    /*0x14*/ uint32_t region_count;
    /*0x18*/ relative_virtual_address<const eh_region> regions;
    /*0x1c*/ relative_virtual_address<eh_node> eh_node_offset;

    // `magic_num >= 0x19930521`
    /*0x20*/ uint32_t es_types;

    // `magic_num >= 0x19930522`
    /*0x24*/ eh_flag eh_flags;
  };

  struct eh_handler_data
  {
    relative_virtual_address<const function_eh_info> eh_info;
  };

  struct function_context
  {
    int32_t state;
    int32_t home_block_index;
  };

  static exception_disposition frame_handler(exception_record*, byte* frame_ptr,
    x64_cpu_context*, dispatcher_context* dispatcher_context) noexcept
  {
    if (dispatcher_context->cookie == &rethrow_probe_cookie)
    {
      return exception_disposition::cxx_handler;
    }

    if (dispatcher_context->cookie != &unwind_cookie)
    {
      return exception_disposition::continue_search;
    }

    auto* throw_frame = dispatcher_context->throw_frame;
    auto& catch_info = throw_frame->catch_info;
    auto* ctx = reinterpret_cast<function_context*>(catch_info.unwind_context);

    const byte* image_base = dispatcher_context->pdata->image_base();
    const auto eh_info_rva = *static_cast<const relative_virtual_address<const function_eh_info>*>(dispatcher_context->extra_data);

    const auto* eh_info = image_base + eh_info_rva;
    const auto try_blocks = image_base + eh_info->try_blocks;

    byte* primary_frame_ptr = catch_info.primary_frame_ptr;
    int32_t state;
    int32_t home_block_index;

    if (primary_frame_ptr < frame_ptr)
    {
      const auto pc_rva = make_rva(throw_frame->mach.rip, image_base);

      const auto regions = image_base + eh_info->regions;
      state = regions[eh_info->region_count - 1].state;

      for (uint32_t i = 1; i != eh_info->region_count; ++i)
      {
        if (pc_rva < regions[i].first_ip)
        {
          state = regions[i - 1].state;
          break;
        }
      }

      home_block_index = eh_info->try_block_count - 1;
    }
    else
    {
      state = ctx->state;
      home_block_index = ctx->home_block_index;
    }

    int32_t funclet_low_state = -1;

    if (primary_frame_ptr == frame_ptr)
    {
      home_block_index = -1;
    }
    else
    {
      // Identify the funclet we're currently in. If we're in a catch
      // funclet, locate the primary frame ptr and cache everything.
      for (; home_block_index > -1; --home_block_index)
      {
        if (const auto& try_block = try_blocks[home_block_index]; try_block.try_high < state && state <= try_block.catch_high)
        {
          if (primary_frame_ptr < frame_ptr)
          {
            const auto* catch_handlers = image_base + try_block.catch_handlers;

            for (int32_t idx = 0; idx < try_block.catch_count; ++idx)
            {
              if (const auto& catch_handler = catch_handlers[idx]; catch_handler.handler == dispatcher_context->fn->begin)
              {
                const auto* node = frame_ptr + catch_handler.node_offset;
                primary_frame_ptr = node->primary_frame_ptr;

                if (primary_frame_ptr < frame_ptr)
                {
                  terminate({hh::bug_check_codes::corrupted_exception_handler,
                    reinterpret_cast<int64_t>(primary_frame_ptr), reinterpret_cast<int64_t>(frame_ptr) });
                }

                break;
              }
            }
          }

          funclet_low_state = try_block.try_low;
          break;
        }
      }

      if (primary_frame_ptr < frame_ptr)
      {
        primary_frame_ptr = frame_ptr;
      }
    }

    int32_t target_state = funclet_low_state;
    const auto* throw_info = catch_info.get_throw_info();
    const catch_handler* target_catch_handler = nullptr;

    for (int32_t idx = home_block_index + 1; !target_catch_handler && idx < eh_info->try_block_count; ++idx)
    {
      if (const auto& try_block = try_blocks[idx]; try_block.try_low <= state && state <= try_block.try_high)
      {
        if (try_block.try_low < funclet_low_state)
        {
          continue;
        }

        if (!throw_info)
        {
          probe_for_exception(*dispatcher_context->pdata, *throw_frame);
          throw_info = catch_info.get_throw_info();
        }

        const auto* handlers = image_base + try_block.catch_handlers;
        for (int32_t handler_idx = 0; handler_idx < try_block.catch_count; ++handler_idx)
        {
          const auto& catch_block = handlers[handler_idx];

          if (!process_catch_block(image_base, catch_block.adjectives, image_base + catch_block.type_desc,
            primary_frame_ptr + catch_block.catch_object_offset, catch_info.get_exception_object(), *throw_info))
          {
            continue;
          }

          target_state = try_block.try_low - 1;
          target_catch_handler = &catch_block;
          dispatcher_context->handler = image_base + catch_block.handler;

          catch_info.primary_frame_ptr = primary_frame_ptr;
          ctx->home_block_index = home_block_index;
          ctx->state = try_block.try_low - 1;
          break;
        }
      }
    }

    if (home_block_index == -1 && !target_catch_handler && eh_info->eh_flags.is_noexcept)
    {
      terminate({ hh::bug_check_codes::corrupted_exception_handler, home_block_index, reinterpret_cast<int64_t>(target_catch_handler) });
    }

    if (target_state < funclet_low_state)
    {
      terminate({ hh::bug_check_codes::corrupted_eh_unwind_data, target_state, funclet_low_state });
    }

    const unwind_graph_edge* unwind_graph = image_base + eh_info->unwind_graph;
    while (state > target_state)
    {
      const auto& edge = unwind_graph[state];
      state = edge.next;

      if (edge.cleanup_handler)
      {
        const auto raw_funclet = const_cast<byte*>(image_base + edge.cleanup_handler);
        using cleanup_handler_t = uintptr_t(const byte*, const byte*);
        const auto cleanup_handler = reinterpret_cast<cleanup_handler_t*>(raw_funclet);
        cleanup_handler(raw_funclet, frame_ptr);
      }
    }

    return exception_disposition::cxx_handler;
  }

  extern "C" exception_disposition __CxxFrameHandler3(exception_record* exception_rec, byte* frame_ptr,
    x64_cpu_context* cpu_ctx, dispatcher_context* dispatcher_ctx) noexcept
  {
    if (exception_rec != &exc_record_cookie)
    {
      verify_seh_in_cxx_handler(exception_rec->code, exception_rec->address,
        exception_rec->flags.all,
        dispatcher_ctx->fn->unwind_struct.value(),
        dispatcher_ctx->image_base);

      return exception_disposition::continue_search;
    }

    return frame_handler(exception_rec, frame_ptr, cpu_ctx, dispatcher_ctx);
  }

  extern "C" exception_disposition __GSHandlerCheck_EH(exception_record* exception_rec, byte* frame_ptr,
    x64_cpu_context* cpu_ctx, dispatcher_context* ctx) noexcept
  {
    return __CxxFrameHandler3(exception_rec, frame_ptr, cpu_ctx, ctx);
  }
}
