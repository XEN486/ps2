#ifndef JIT_X64_HPP
#define JIT_X64_HPP

#include "../emotion.hpp"

#include <cassert>
#include <type_traits>
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

	protected:
		void LUI(InstructionData& data) override;
		void ADDIU(InstructionData& data) override;
		void SLL(InstructionData& data) override;


	private:
		asmjit::InvokeNode* EmitExternalCall(uintptr_t address, const asmjit::FuncSignature& sig);

		template <typename T> constexpr void EmitLoadRegister(T reg, RegisterSize size, u8 index);
		template <typename T> constexpr void EmitStoreRegister(RegisterSize size, u8 index, T reg, bool sign_extend = true);
		template <typename Dst, typename Src> void EmitReadVirtualMemory32(Dst ret, Src address);
		template <typename Dst, typename Src> void EmitWriteVirtualMemory32(Dst address, Src value);
		template <typename Dst, typename Src> void EmitReadVirtualMemory16(Dst ret, Src address);
		template <typename Dst, typename Src> void EmitWriteVirtualMemory16(Dst address, Src value);
		template <typename T> void EmitJump(T address);

	private:
		asmjit::x86::Compiler cc;
		asmjit::x86::Gp r5900;
		asmjit::x86::Gp scratch1;
		asmjit::x86::Gp scratch2;
		asmjit::x86::Gp scratch3;
		asmjit::x86::Gp scratch4;
		asmjit::x86::Gp temp;
	};
}

#include "jit_x64.inl"

#endif