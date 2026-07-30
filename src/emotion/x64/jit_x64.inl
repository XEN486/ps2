#include "jit_x64.hpp"
#include "../../memory.hpp"

namespace EmotionEngine::MIPS {
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

	template <typename T>
	void JitX64::EmitJump(T address) {
		cc.mov(asmjit::x86::dword_ptr(r5900, offsetof(R5900, next_pc)), address);
		cc.ret();
	}
}