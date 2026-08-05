#include "jit_x64.hpp"
#include "../Memory/memory.hpp"

#include <type_traits>
#include <cassert>

namespace EmotionEngine::Core {
#pragma warning(push)
#pragma warning(disable:4505)
	static void WRAP_WriteVirtualMemory64(EmotionEngine::Memory* mem, u32 addr, u64 val) { mem->WriteVirtualMemory64(addr, val); }
	static void WRAP_WriteVirtualMemory32(EmotionEngine::Memory* mem, u32 addr, u32 val) { mem->WriteVirtualMemory32(addr, val); }
	static void WRAP_WriteVirtualMemory16(EmotionEngine::Memory* mem, u32 addr, u16 val) { mem->WriteVirtualMemory16(addr, val); }
	static void WRAP_WriteVirtualMemory8(EmotionEngine::Memory* mem, u32 addr, u8 val) { mem->WriteVirtualMemory8(addr, val); }
	static u64 WRAP_ReadVirtualMemory64(EmotionEngine::Memory* mem, u32 addr) { return mem->ReadVirtualMemory64(addr); }
	static u32 WRAP_ReadVirtualMemory32(EmotionEngine::Memory* mem, u32 addr) { return mem->ReadVirtualMemory32(addr); }
	static u16 WRAP_ReadVirtualMemory16(EmotionEngine::Memory* mem, u32 addr) { return mem->ReadVirtualMemory16(addr); }
	static u8 WRAP_ReadVirtualMemory8(EmotionEngine::Memory* mem, u32 addr) { return mem->ReadVirtualMemory8(addr); }
#pragma warning(pop)

	template <typename T>
	void JitX64::EmitJump(T address) {
		cc.mov(asmjit::x86::dword_ptr(r5900, offsetof(R5900, next_pc)), address);
	}

	template <typename Size, typename T>
	void JitX64::EmitReadVirtualMemory(const asmjit::x86::Gp& ret, T address) {
		// TODO: optimize scratchpad/rdram to not have call
		static_assert(std::is_same_v<T, u32> || std::is_same_v<T, asmjit::x86::Gp>);
		uintptr_t ptr;

		if constexpr (std::is_same_v<Size, u64>) {
			ptr = reinterpret_cast<uintptr_t>(WRAP_ReadVirtualMemory64);
			assert(ret.is_gp64());
		} else if constexpr (std::is_same_v<Size, u32>) {
			ptr = reinterpret_cast<uintptr_t>(WRAP_ReadVirtualMemory32);
			assert(ret.is_gp32());
		} else if constexpr (std::is_same_v<Size, u16>) {
			ptr = reinterpret_cast<uintptr_t>(WRAP_ReadVirtualMemory16);
			assert(ret.is_gp16());
		} else if constexpr (std::is_same_v<Size, u8>) {
			ptr = reinterpret_cast<uintptr_t>(WRAP_ReadVirtualMemory8);
			assert(ret.is_gp8());
		} else {
			static_assert(false);
		}
		
		asmjit::InvokeNode* node;
		cc.invoke(asmjit::Out(node), ptr, asmjit::FuncSignature::build<Size, Memory*, u32>());
		node->set_arg(0, m_Memory);
		node->set_arg(1, address);
		node->set_ret(0, ret);
	}

	template <typename Size, typename T>
	void JitX64::EmitWriteVirtualMemory(T address, const asmjit::x86::Gp& value) {
		// TODO: optimize scratchpad/rdram to not have call
		static_assert(std::is_same_v<T, u32> || std::is_same_v<T, asmjit::x86::Gp>);
		uintptr_t ptr;

		if constexpr (std::is_same_v<Size, u64>) {
			ptr = reinterpret_cast<uintptr_t>(WRAP_WriteVirtualMemory64);
			assert(value.is_gp64());
		} else if constexpr (std::is_same_v<Size, u32>) {
			ptr = reinterpret_cast<uintptr_t>(WRAP_WriteVirtualMemory32);
			assert(value.is_gp32());
		} else if constexpr (std::is_same_v<Size, u16>) {
			ptr = reinterpret_cast<uintptr_t>(WRAP_WriteVirtualMemory16);
			assert(value.is_gp16());
		} else if constexpr (std::is_same_v<Size, u8>) {
			ptr = reinterpret_cast<uintptr_t>(WRAP_WriteVirtualMemory8);
			assert(value.is_gp8());
		} else {
			static_assert(false);
		}

		asmjit::InvokeNode* node;
		cc.invoke(asmjit::Out(node), ptr, asmjit::FuncSignature::build<void, Memory*, u32, Size>());
		node->set_arg(0, m_Memory);
		node->set_arg(1, address);
		node->set_arg(2, value);
	}

}