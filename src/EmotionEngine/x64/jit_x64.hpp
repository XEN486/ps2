#ifndef EMOTIONENGINE_X64_JIT_HPP
#define EMOTIONENGINE_X64_JIT_HPP

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
		void SLL(InstructionData& data) override;
		void SLTI(InstructionData& data) override;
		void BNE(InstructionData& data) override;
		void LUI(InstructionData& data) override;
		void ORI(InstructionData& data) override;
		void JR(InstructionData& data) override;
		void ADDIU(InstructionData& data) override;
		void SW(InstructionData& data) override;
		void JALR(InstructionData& data) override;
		void SD(InstructionData& data) override;
		void JAL(InstructionData& data) override;
		void SRA(InstructionData& data) override;
		void BGEZ(InstructionData& data) override;
		void LBU(InstructionData& data) override;
		void ANDI(InstructionData& data) override;
		void BEQ(InstructionData& data) override;
		void LD(InstructionData& data) override;
		void DSRL(InstructionData& data) override;
		void DSLL(InstructionData& data) override;
		void DSLL32(InstructionData& data) override;
		void DSRA32(InstructionData& data) override;
		void OR(InstructionData& data) override;
		void DADDU(InstructionData& data) override;
		void LW(InstructionData& data) override;
		void MULT(InstructionData& data) override;
		void MULTU(InstructionData& data) override;
		void DIV(InstructionData& data) override;
		void DIVU(InstructionData& data) override;
		void BREAK(InstructionData& data) override;
		void MFLO(InstructionData& data) override;
		void ADDU(InstructionData& data) override;
		void SLT(InstructionData& data) override;
		void MOVN(InstructionData& data) override;
		void SLTIU(InstructionData& data) override;
		void LB(InstructionData& data) override;
		void SWC1(InstructionData& data) override;
		void J(InstructionData& data) override;
		void SB(InstructionData& data) override;
		void MFHI(InstructionData& data) override;
		void SLTU(InstructionData& data) override;
		void BLEZ(InstructionData& data) override;
		void SUBU(InstructionData& data) override;
		void BGTZ(InstructionData& data) override;
		void AND(InstructionData& data) override;
		void SRL(InstructionData& data) override;
		void DSRL32(InstructionData& data) override;
		void LHU(InstructionData& data) override;
		void BLTZ(InstructionData& data) override;
		void SH(InstructionData& data) override;
		void DSRAV(InstructionData& data) override;
		void XORI(InstructionData& data) override;
		void LWU(InstructionData& data) override;
		void MOVZ(InstructionData& data) override;
		void DSLLV(InstructionData& data) override;
		void DADDIU(InstructionData& data) override;
		void LH(InstructionData& data) override;
		void SYSCALL(InstructionData& data) override;
		void MFSA(InstructionData& data) override;
		void SLLV(InstructionData& data) override;
		void SRAV(InstructionData& data) override;
		void NOR(InstructionData& data) override;

	// COP0
	protected:
		void MFC0(InstructionData& data) override;
		void MTC0(InstructionData& data) override;

	// COP1
	protected:
		void MTC1(InstructionData& data) override;
		void CTC1(InstructionData& data) override;

	// COP2
	protected:
		void CFC2(InstructionData& data) override;
		void CTC2(InstructionData& data) override;
		void QMFC2(InstructionData& data) override;
		void QMTC2(InstructionData& data) override;

	// MMI
	protected:
		void LQ(InstructionData& data) override;
		void SQ(InstructionData& data) override;
		void POR(InstructionData& data) override;
		void PMFHI(InstructionData& data) override;
		void PMFLO(InstructionData& data) override;
		void PCPYLD(InstructionData& data) override;
		void PCPYHD(InstructionData& data) override;
		void PEXTLW(InstructionData& data) override;

	private:
		template <typename T> void EmitJump(T address);
		template <typename Size, typename T> void EmitReadVirtualMemory(const asmjit::x86::Gp& ret, T address);
		template <typename Size, typename T> void EmitWriteVirtualMemory(T address, const asmjit::x86::Gp& value);
		void FlushRegisters();
		void LoadRegisters();

		void FlushRegisters(std::initializer_list<u8> args);
		void LoadRegisters(std::initializer_list<u8> args);

		// call FlushRegisters() before this function
		void EmitLoad128(asmjit::x86::Vec& v, u8 idx);

		// call LoadRegisters() after this function
		void EmitStore128(u8 idx, asmjit::x86::Vec& v);

	private:
		asmjit::x86::Compiler cc; 
		asmjit::x86::Gp r5900;
		asmjit::x86::Gp r[32];
		asmjit::x86::Gp temp;
	};
}

#include "jit_x64.inl"
#endif