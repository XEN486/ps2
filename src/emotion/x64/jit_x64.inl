#include "jit_x64.hpp"
#include "../../memory.hpp"

namespace EmotionEngine::MIPS {
	template <typename T>
	constexpr void JitX64::EmitLoadRegister(T reg, RegisterSize size, u8 index) {
		// always read zero for r0
		if (index == 0) {
			cc.xor_(reg, reg);
			return;
		}

		// TODO: support big endian
		// reg <- size [r5900 + (index * 16)]
		cc.mov(reg, asmjit::x86::ptr(r5900, index * sizeof(GPR), size));
	}

	template <typename T>
	constexpr void JitX64::EmitStoreRegister(RegisterSize size, u8 index, T reg, bool sign_extend) {
		// never write to r0
		if (index == 0) {
			return;
		}

		// TODO: support big endian
		// size [r5900 + (index * 16)] <- reg

		if (sign_extend) {
			assert(size == R64);
		}

		// reg = Gp
		if constexpr (std::is_same_v<T, asmjit::x86::Gp>) {
			if (sign_extend) {
				assert(reg.is_gp32());
				cc.movsxd(temp, reg);
				cc.mov(asmjit::x86::ptr(r5900, index * sizeof(GPR), size), temp);
				return;
			}
		} 
		
		// reg = imm
		else if constexpr (std::is_same_v<T, u32>) {
			if (sign_extend) {
				cc.movabs(temp, (u64)(i32)reg);
				cc.mov(asmjit::x86::ptr(r5900, index * sizeof(GPR), size), temp);
				return;
			}
		}
		
		else {
			assert(!sign_extend);
		}

		cc.mov(asmjit::x86::ptr(r5900, index * sizeof(GPR), size), reg);
	}

	template <typename Dst, typename Src>
	void JitX64::EmitReadVirtualMemory32(Dst ret, Src address) {
		asmjit::x86::Gp value = cc.new_gp32("value");
		asmjit::InvokeNode* node = EmitExternalCall(
			reinterpret_cast<uintptr_t>(&Memory::ReadVirtualMemory32),
			asmjit::FuncSignature::build<u32, u32>()
		);

		node->set_arg(0, address);
		node->set_ret(0, value);

		if (ret.is_gp64()) {
			// writing to lo dword of a 64-bit register clears the hi dword too
			// so we have to use or instead
			asmjit::x86::Gp mask = cc.new_gp64("mask");
			cc.movabs(mask, 0xffffffff00000000);
			cc.and_(ret, mask);
			cc.or_(ret, value);
		} else {
			cc.mov(ret, value);
		}
	}

	template <typename Dst, typename Src>
	void JitX64::EmitWriteVirtualMemory32(Dst address, Src value) {
		asmjit::InvokeNode* node = EmitExternalCall(
			reinterpret_cast<uintptr_t>(&Memory::WriteVirtualMemory32),
			asmjit::FuncSignature::build<void, u32, u32>()
		);

		node->set_arg(0, address);
		node->set_arg(1, value);
	}

	template <typename Dst, typename Src>
	void JitX64::EmitReadVirtualMemory16(Dst ret, Src address) {
		asmjit::x86::Gp value = cc.new_gp16("value");
		asmjit::InvokeNode* node = EmitExternalCall(
			reinterpret_cast<uintptr_t>(&Memory::ReadVirtualMemory16),
			asmjit::FuncSignature::build<u16, u32>()
		);

		node->set_arg(0, address);
		node->set_ret(0, value);
		cc.mov(ret.r16(), value);
	}

	template <typename Dst, typename Src>
	void JitX64::EmitWriteVirtualMemory16(Dst address, Src value) {
		asmjit::InvokeNode* node = EmitExternalCall(
			reinterpret_cast<uintptr_t>(&Memory::WriteVirtualMemory16),
			asmjit::FuncSignature::build<void, u32, u16>()
		);

		node->set_arg(0, address);
		node->set_arg(1, value.r16());
	}

	template <typename T>
	void JitX64::EmitJump(T address) {
		cc.mov(asmjit::x86::dword_ptr(r5900, offsetof(R5900, next_pc)), address);
		cc.ret();
	}
}