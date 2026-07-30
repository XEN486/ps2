#ifndef RECOMPILER_X64_HPP
#define RECOMPILER_X64_HPP

#include "../emotion.hpp"

#include <asmjit/x86.h>

namespace EmotionEngine::MIPS {
	enum RegisterSize {
		R64	= 8, // lo 64-bit
		R32	= 4, // lo 32-bit
		R16	= 2, // lo 16-bit
		R8	= 1, // lo 8-bit
	};

	class JitX64 : public JitBackend {
	public:
		bool InitJit(R5900* cpu) override;

	protected:
		void EmitBeginBlock() override;
		void EmitEndBlock() override;

		void test() override;
		
	private:
		constexpr void EmitLoadRegister(asmjit::x86::Gp& reg, RegisterSize size, u8 index) {
			// always read zero for r0
			if (index == 0) {
				cc.xor_(reg, reg);
				return;
			}

			// TODO: support big endian
			// reg <- size [r5900 + (index * 16)]
			cc.mov(reg, asmjit::x86::ptr(r5900, index * sizeof(GPR), size));
		}

		constexpr void EmitStoreRegister(RegisterSize size, u8 index, asmjit::x86::Gp& reg) {
			// never write to r0
			if (index == 0) {
				return;
			}

			// TODO: support big endian
			// size [r5900 + (index * 16)] <- reg
			cc.mov(asmjit::x86::ptr(r5900, index * sizeof(GPR), size), reg);
		}

	private:
		asmjit::InvokeNode* EmitExternalCall(uintptr_t address, const asmjit::FuncSignature& sig);

		template <typename Dst, typename Src> void EmitReadVirtualMemory32(Dst ret, Src address);
		template <typename Dst, typename Src> void EmitWriteVirtualMemory32(Dst address, Src value);
		template <typename T> void EmitJump(T address);

	private:
		asmjit::x86::Compiler cc;
		asmjit::x86::Gp r5900;
	};
}

#include "jit_x64.inl"

#endif