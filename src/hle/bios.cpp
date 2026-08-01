#include "bios.hpp"
using namespace EmotionEngine::Core;
using namespace HLE;

void Bios::EESysCall(R5900* r5900) {
	u8 syscall_no = r5900->gpr[(u8)AbiNames::v1].reg_u8[0];
	switch (syscall_no) {
		case SetGsCrt: {
			bool interlaced = r5900->gpr[(u8)AbiNames::a0].reg_u64[0];
			int display_mode = r5900->gpr[(u8)AbiNames::a1].reg_i32[0];
			bool frame = r5900->gpr[(u8)AbiNames::a2].reg_u64[0];

			// TODO: implement GS
			debug_log("SetGsCrt({}, 0x{:08x}, {})", interlaced, display_mode, frame);
			break;
		};

		case InitMainThread: {
			u32 stack = r5900->gpr[(u8)AbiNames::a1].reg_u32[0];
			int stack_size = r5900->gpr[(u8)AbiNames::a2].reg_i32[0];

			if (stack == 0xffffffff) {
				m_ThreadSP = (RDRAM_LAST_ADDR - stack_size);
			} else {
				m_ThreadSP = stack + stack_size;
			}

			debug_log("InitMainThread(0x{:08x}, 0x{:08x}) -> 0x{:08x}", stack, stack_size, m_ThreadSP);
			r5900->gpr[(u8)AbiNames::v0].reg_u32[0] = m_ThreadSP;
			break;
		};

		case InitHeap: {
			u32 heap = r5900->gpr[(u8)AbiNames::a0].reg_u32[0];
			int heap_size = r5900->gpr[(u8)AbiNames::a1].reg_i32[0];

			if (heap == 0xffffffff) {
				m_HeapEnd = m_ThreadSP;
			} else {
				m_HeapEnd = heap + heap_size;
			}

			debug_log("InitHeap(0x{:08x}, 0x{:08x}) -> 0x{:08x}", heap, heap_size, m_HeapEnd);
			r5900->gpr[(u8)AbiNames::v0].reg_u32[0] = m_HeapEnd;
			break;
		}

		case FlushCache: {
			int mode = r5900->gpr[(u8)AbiNames::a0].reg_i32[0];

			// TODO: invalidate JIT blocks here?
			debug_log("FlushCache({})", mode);
			break;
		}

		case GsSetIMR: {
			uint64_t imr = r5900->gpr[(u8)AbiNames::a0].reg_u64[0];

			// TODO: implement GS
			debug_log("GsSetIMR(0x{:016x})", imr);
			break;
		}

		default: {
			error_log("unknown syscall {:02X}h", syscall_no);
			exit(1);
		}
	}
}