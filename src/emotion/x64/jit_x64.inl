#include "jit_x64.hpp"
#include "../../memory.hpp"

namespace EmotionEngine::Core {
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
				cc.movsxd(t1, reg);
				cc.mov(asmjit::x86::ptr(r5900, index * sizeof(GPR), size), t1);
				return;
			}

			cc.mov(asmjit::x86::ptr(r5900, index * sizeof(GPR), size), reg);
		} 
		
		// reg = imm
		else if constexpr (std::is_same_v<T, u32>) {
			if (sign_extend) {
				cc.movabs(t1, (u64)(i32)reg);
				cc.mov(asmjit::x86::ptr(r5900, index * sizeof(GPR), size), t1);
				return;
			}

			cc.mov(asmjit::x86::ptr(r5900, index * sizeof(GPR), size), reg);
		}
		
		else {
			assert(!sign_extend);
			cc.mov(t1, reg);
			cc.mov(asmjit::x86::ptr(r5900, index * sizeof(GPR), size), t1);
		}
	}

	template <typename T>
	void JitX64::EmitJump(T address) {
		cc.mov(asmjit::x86::dword_ptr(r5900, offsetof(R5900, next_pc)), address);
	}
}