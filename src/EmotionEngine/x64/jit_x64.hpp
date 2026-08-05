#ifndef EMOTIONENGINE_X64_JIT_X64_HPP
#define EMOTIONENGINE_X64_JIT_X64_HPP

#include "../emotion.hpp"
#include <asmjit/x86.h>

namespace EmotionEngine::Core {
	class JitX64 : public JitBackend {
	public:
		bool InitJit(R5900* cpu, Memory* memory) override;

	protected:
		void EmitBeginBlock() override;
		void EmitEndBlock() override;

	protected:
		void MFC0(InstructionData& data) override;
		void SLL(InstructionData& data) override;
		void SLTI(InstructionData& data) override;
		void BNE(InstructionData& data) override;
		void LUI(InstructionData& data) override;
		void ORI(InstructionData& data) override;
		void JR(InstructionData& data) override;

	private:
		template <typename T> void EmitJump(T address);
		template <typename Size, typename T> void EmitReadVirtualMemory(const asmjit::x86::Gp& ret, T address);
		template <typename Size, typename T> void EmitWriteVirtualMemory(T address, const asmjit::x86::Gp& value);
		void FlushRegisters();

	private:
		asmjit::x86::Compiler cc; 
		asmjit::x86::Gp r5900;
		asmjit::x86::Gp r[32];
	};
}

#include "jit_x64.inl"
#endif