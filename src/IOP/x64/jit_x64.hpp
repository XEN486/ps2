#ifndef IOP_JIT_X64_HPP
#define IOP_JIT_X64_HPP

#include "../iop.hpp"
#include <asmjit/x86.h>

namespace IOProcessor {
	class JitX64 : public JitBackend {
	public:
		bool InitJit(R3000A* cpu, Memory* memory) override;

	protected:
		void EmitBeginBlock() override;
		void EmitEndBlock() override;

	protected:
		void LUI(InstructionData& data) override;
		void ORI(InstructionData& data) override;
		void SW(InstructionData& data) override;
		void SLL(InstructionData& data) override;
		void ADDIU(InstructionData& data) override;
		void J(InstructionData& data) override;
		void LHU(InstructionData& data) override;
		void OR(InstructionData& data) override;
		void MTC0(InstructionData& data) override;
		void BNE(InstructionData& data) override;
		void ADDI(InstructionData& data) override;
		void LW(InstructionData& data) override;
		void SLTU(InstructionData& data) override;
		void ADDU(InstructionData& data) override;
		void SH(InstructionData& data) override;
		void JAL(InstructionData& data) override;
		void ANDI(InstructionData& data) override;
		void SB(InstructionData& data) override;
		void JR(InstructionData& data) override;
		void LB(InstructionData& data) override;
		void SLLV(InstructionData& data) override;
		void BEQ(InstructionData& data) override;
		void MFC0(InstructionData& data) override;
		void AND(InstructionData& data) override;
		void ADD(InstructionData& data) override;
		void BGTZ(InstructionData& data) override;
		void BLEZ(InstructionData& data) override;
		void LBU(InstructionData& data) override;
		void JALR(InstructionData& data) override;
		void BLTZ(InstructionData& data) override;
		void SLTI(InstructionData& data) override;
		void SUBU(InstructionData& data) override;
		void SRA(InstructionData& data) override;
		void DIV(InstructionData& data) override;
		void MFLO(InstructionData& data) override;
		void BGEZ(InstructionData& data) override;
		void SRL(InstructionData& data) override;
		void SLTIU(InstructionData& data) override;
		void DIVU(InstructionData& data) override;
		void MFHI(InstructionData& data) override;
		void SLT(InstructionData& data) override;
		void SYSCALL(InstructionData& data) override;
		void MTLO(InstructionData& data) override;
		void MTHI(InstructionData& data) override;
		void RFE(InstructionData& data) override;
		void LH(InstructionData& data) override;
		void NOR(InstructionData& data) override;
		void SRAV(InstructionData& data) override;
		void SRLV(InstructionData& data) override;
		void MULTU(InstructionData& data) override;
		void XOR(InstructionData& data) override;
		void XORI(InstructionData& data) override;
		void MULT(InstructionData& data) override;
		void SUB(InstructionData& data) override;
		void LWL(InstructionData& data) override;
		void LWR(InstructionData& data) override;
		void SWL(InstructionData& data) override;
		void SWR(InstructionData& data) override;
		void BLTZAL(InstructionData& data) override;
		void BGEZAL(InstructionData& data) override;
		void BREAK(InstructionData& data) override;

	protected:
		void MoveImm(InstructionData& data) override;
		void Illegal(InstructionData& data) override;

	private:
		void EmitFlushRegisters();
		void EmitLoadRegisters();
		void EmitException(InstructionData& data, ExceptionCause cause);
		void EmitEpilogue();

		// checks for overflow in last instruction
		// if true, moves the value in `value` to the register `result`.
		// if false, generates an exception and does not affect `result`.
		void CheckOverflow(InstructionData& data, asmjit::x86::Gp& result, asmjit::x86::Gp& value);

		template <typename T>
		void EmitJump(T address);

		template <typename Size>
		void EmitReadVirtualMemory(InstructionData& data, u8 ret_idx, asmjit::x86::Gp& address, bool sign_extend);
		
		template <typename Size>
		void EmitWriteVirtualMemory(InstructionData& data, asmjit::x86::Gp& address, u8 value_idx);

	private:
		asmjit::x86::Compiler cc;
		asmjit::x86::Gp r3000a;
		asmjit::x86::Gp temp;
		asmjit::x86::Gp temp64;
		asmjit::x86::Gp r[32];
	};
}

#include "jit_x64.inl"
#endif