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
		void SQ(InstructionData& data) override;
		void SLTU(InstructionData& data) override;
		void JAL(InstructionData& data) override;
		void BNE(InstructionData& data) override;
		void DADDU(InstructionData& data) override;
		void SYSCALL(InstructionData& data) override;
		void JR(InstructionData& data) override;
		void EI(InstructionData& data) override;
		void LW(InstructionData& data) override;
		void SD(InstructionData& data) override;
		void SW(InstructionData& data) override;
		void MULT(InstructionData& data) override;
		void ADDU(InstructionData& data) override;
		void LHU(InstructionData& data) override;
		void SH(InstructionData& data) override;

	private:
		asmjit::InvokeNode* EmitExternalCall(uintptr_t address, const asmjit::FuncSignature& sig);

		void EmitLoadRegister128(asmjit::x86::Vec& reg, u8 index);
		void EmitStoreRegister128(u8 index, asmjit::x86::Vec& reg);

		void EmitReadVirtualMemory128(asmjit::x86::Vec& ret, const asmjit::x86::Gp& address);
		void EmitWriteVirtualMemory128(const asmjit::x86::Gp& address, asmjit::x86::Vec& val);

		void EmitReadVirtualMemory64(const asmjit::x86::Gp& ret, const asmjit::x86::Gp& address);
		void EmitWriteVirtualMemory64(const asmjit::x86::Gp& address, const asmjit::x86::Gp& value);

		void EmitReadVirtualMemory64(const asmjit::x86::Gp& ret, u32 address);
		void EmitWriteVirtualMemory64(u32 address, const asmjit::x86::Gp& value);
		
		void EmitReadVirtualMemory32(const asmjit::x86::Gp& ret, const asmjit::x86::Gp& address);
		void EmitWriteVirtualMemory32(const asmjit::x86::Gp& address, const asmjit::x86::Gp& value);

		void EmitReadVirtualMemory32(const asmjit::x86::Gp& ret, u32 address);
		void EmitWriteVirtualMemory32(u32 address, const asmjit::x86::Gp& value);

		void EmitReadVirtualMemory16(const asmjit::x86::Gp& ret, const asmjit::x86::Gp& address);
		void EmitWriteVirtualMemory16(const asmjit::x86::Gp& address, const asmjit::x86::Gp& value);

		void EmitReadVirtualMemory16(const asmjit::x86::Gp& ret, u32 address);
		void EmitWriteVirtualMemory16(u32 address, const asmjit::x86::Gp& value);

		template <typename T> constexpr void EmitLoadRegister(T reg, RegisterSize size, u8 index);
		template <typename T> constexpr void EmitStoreRegister(RegisterSize size, u8 index, T reg, bool sign_extend);
		template <typename T> void EmitJump(T address);

		void EmitVirtualToPhysical(const asmjit::x86::Gp& address);
		void EmitWrite32To64Preserved(const asmjit::x86::Gp& dst, const asmjit::x86::Gp& src);

		void EmitStoreSpecialRegister(SpecialRegName dst, const asmjit::x86::Gp& src);

	private:
		asmjit::x86::Compiler cc;
		asmjit::x86::Gp r5900;

		// scratch registers
		asmjit::x86::Gp s1;
		asmjit::x86::Gp s2;
		asmjit::x86::Gp s3;
		asmjit::x86::Gp s4;

		// temp registers (overwritable by Emit*)
		asmjit::x86::Gp t1;

		// vector registers
		asmjit::x86::Vec v1;
		asmjit::x86::Vec v2;
		asmjit::x86::Vec v3;
		asmjit::x86::Vec v4;
	};
}

#include "jit_x64.inl"

#endif