#include "jit_x64.hpp"
#include "../memory/memory.hpp"

#include <type_traits>
#include <cassert>

namespace IOProcessor {
#pragma warning(push)
#pragma warning(disable:4505)
	static void WRAP_WriteVirtualMemory32(Memory* mem, u32 addr, u32 val) { mem->WriteVirtualMemory32(addr, val); }
	static void WRAP_WriteVirtualMemory16(Memory* mem, u32 addr, u16 val) { mem->WriteVirtualMemory16(addr, val); }
	static void WRAP_WriteVirtualMemory8(Memory* mem, u32 addr, u8 val) { mem->WriteVirtualMemory8(addr, val); }
	static u32 WRAP_ReadVirtualMemory32(Memory* mem, u32 addr) { return mem->ReadVirtualMemory32(addr); }
	static u16 WRAP_ReadVirtualMemory16(Memory* mem, u32 addr) { return mem->ReadVirtualMemory16(addr); }
	static u8 WRAP_ReadVirtualMemory8(Memory* mem, u32 addr) { return mem->ReadVirtualMemory8(addr); }
#pragma warning(pop)

	template <typename T>
	void JitX64::EmitJump(T address) {
		cc.mov(asmjit::x86::dword_ptr(r3000a, offsetof(R3000A, next_pc)), address);
	}

	template <typename Size>
	void JitX64::EmitReadVirtualMemory(InstructionData& data, u8 ret_idx, asmjit::x86::Gp& address, bool sign_extend) {
		asmjit::x86::Gp& ret = r[ret_idx];

		// TODO: optimize scratchpad/ram to not have call
		uintptr_t ptr;

		// correct size temp register
		asmjit::x86::Gp temp_sized;

		asmjit::Label exception = cc.new_label();
		asmjit::Label end = cc.new_label();

		if constexpr (std::is_same_v<Size, u32>) {
			ptr = reinterpret_cast<uintptr_t>(WRAP_ReadVirtualMemory32);
			temp_sized = temp.r32();
			cc.test(address, 3);
			cc.jnz(exception);
		} else if constexpr (std::is_same_v<Size, u16>) {
			ptr = reinterpret_cast<uintptr_t>(WRAP_ReadVirtualMemory16);
			temp_sized = temp.r16();
			cc.test(address, 1);
			cc.jnz(exception);
		} else if constexpr (std::is_same_v<Size, u8>) {
			ptr = reinterpret_cast<uintptr_t>(WRAP_ReadVirtualMemory8);
			temp_sized = temp.r8();
		} else {
			static_assert(false);
		}
		
		asmjit::InvokeNode* node;
		cc.invoke(asmjit::Out(node), ptr, asmjit::FuncSignature::build<Size, Memory*, u32>());
		node->set_arg(0, m_Memory);
		node->set_arg(1, address);

		if (ret_idx == 0) {
			node->set_ret(0, temp_sized);
		} else if constexpr (std::is_same_v<Size, u32>) {
			node->set_ret(0, ret);
		} else {
			node->set_ret(0, temp_sized);
			if (sign_extend) cc.movsx(ret, temp_sized);
			else cc.movzx(ret, temp_sized);
		}

		cc.jmp(end);

		cc.bind(exception);
		EmitException(data, ExceptionCause::LoadAddressError);
		EmitEpilogue();
		cc.ret();

		cc.bind(end);
	}

	template <typename Size>
	void JitX64::EmitWriteVirtualMemory(InstructionData& data, asmjit::x86::Gp& address, u8 value_idx) {
		// dont try write when cache is isolated
		asmjit::Label end = cc.new_label();
		cc.test(asmjit::x86::dword_ptr(r3000a, offsetof(R3000A, cop0[SR])), 0x10000); // sr.16 == ISc (isolate cache)
		cc.jnz(end);

		// TODO: optimize scratchpad/ram to not have call
		uintptr_t ptr;

		// correct size value register
		asmjit::x86::Gp value_sized;
		asmjit::Label exception = cc.new_label();
		
		if constexpr (std::is_same_v<Size, u32>) {
			ptr = reinterpret_cast<uintptr_t>(WRAP_WriteVirtualMemory32);
			value_sized = r[value_idx];
			cc.test(address, 3);
			cc.jnz(exception);
		} else if constexpr (std::is_same_v<Size, u16>) {
			ptr = reinterpret_cast<uintptr_t>(WRAP_WriteVirtualMemory16);
			value_sized = r[value_idx].r16();
			cc.test(address, 1);
			cc.jnz(exception);
		} else if constexpr (std::is_same_v<Size, u8>) {
			ptr = reinterpret_cast<uintptr_t>(WRAP_WriteVirtualMemory8);
			value_sized = r[value_idx].r8();
		} else {
			static_assert(false);
		}
		
		asmjit::InvokeNode* node;
		cc.invoke(asmjit::Out(node), ptr, asmjit::FuncSignature::build<void, Memory*, u32, Size>());
		node->set_arg(0, m_Memory);
		node->set_arg(1, address);
		node->set_arg(2, value_sized);
		cc.jmp(end);

		cc.bind(exception);
		EmitException(data, ExceptionCause::StoreAddressError);
		EmitEpilogue();
		cc.ret();
		
		cc.bind(end);
	}
}