#include "jit_x64.hpp"
#include "../../hle/bios.hpp"

using namespace EmotionEngine::MIPS;
using namespace asmjit;

void JitX64::LUI(InstructionData& data) {
	u32 shifted = data.imm << 16;
	EmitStoreRegister(R64, data.rt, shifted, true);
}

void JitX64::ADDIU(InstructionData& data) {
	EmitLoadRegister(s1, R32, data.rs);						// s1 <- rs
	if (data.imm) cc.add(s1.r32(), (u32)(i16)data.imm);		// s1 += imm (signed)
	EmitStoreRegister(R64, data.rt, s1.r32(), true);		// rt <- s1
}

void JitX64::SLL(InstructionData& data) {
	// optimization: rd == rt && sa == 0 -> NOP
	if (data.rd == data.rt && data.sa == 0) {
		return;
	}

	EmitLoadRegister(s1, R32, data.rt);
	cc.shl(s1, data.sa);
	EmitStoreRegister(R64, data.rd, s1.r32(), true);
}

void JitX64::SQ(InstructionData& data) {
	// vaddr = s1
	// base = rs
	EmitLoadRegister(s1, R32, data.rs);					// vaddr <- base
	if (data.imm) cc.add(s1.r32(), (u32)(i16)data.imm);	// vaddr += (i16)imm
	cc.and_(s1.r32(), 0xfffffff0);						// vaddr &= 0xfffffff0
	
	EmitLoadRegister128(v1, data.rt);					// v1 <- gpr[rt].128
	EmitWriteVirtualMemory128(s1, v1);					// vaddr <- v1
}

void JitX64::SLTU(InstructionData& data) {
	// compare rs and rt
    EmitLoadRegister(s1, R64, data.rs);
    EmitLoadRegister(s2, R64, data.rt);
    cc.cmp(s1, s2);

    // s1 < s2 (unsigned) -> s1 = 1, else s1 = 0
    cc.setb(s1.r8());
    cc.movzx(s1, s1.r8()); // zero extend to 64-bit

	// rd <- s1
    EmitStoreRegister(R64, data.rd, s1, false);
}

void JitX64::JAL(InstructionData& data) {
	// PC at this point is AFTER the branch delay slot because the JIT reorders the instructions
	// so we can store the current PC to GPR[31]
	EmitStoreRegister(R64, 31, m_CompilePC, false);

	// we have to go back 2 instructions (8 bytes) to go back to the address of the JAL instruction
	// for the same reason as before
	EmitJump(((m_CompilePC - 8) & 0xf0000000) | (data.addr << 2));
}

void JitX64::BNE(InstructionData& data) {
	// compare rs and rt
    EmitLoadRegister(s1, R64, data.rs);
    EmitLoadRegister(s2, R64, data.rt);

	Label exit_bne = cc.new_label();
    cc.cmp(s1, s2);
	cc.j(x86::CondCode::kEqual, exit_bne);

	// PC is after the branch delay slot so we have to go back 1 instruction to use it as a base for the branch
	EmitJump((m_CompilePC - 4) + (i32)((i16)data.imm << 2));

	cc.bind(exit_bne);
}

void JitX64::DADDU(InstructionData& data) {
	EmitLoadRegister(s1, R64, data.rs);					// s1 <- rs
	EmitLoadRegister(s2, R64, data.rt);					// s2 <- rt
	cc.add(s1, s2);										// s1 += s2
	EmitStoreRegister(R64, data.rd, s1.r64(), false);	// rd <- s1
}

void JitX64::SYSCALL(InstructionData& data) {
	// TODO: LLE emulation of BIOS
	InvokeNode* node = EmitExternalCall(
		reinterpret_cast<uintptr_t>(&HLE::Bios::EESysCall),
		FuncSignature::build<void, R5900*>()
	);

	node->set_arg(0, m_R5900);
}

void JitX64::JR(InstructionData& data) {
	EmitLoadRegister(s1, R32, data.rs);	// s1 <- rs
	EmitJump(s1);						// pc <- s1

	// TODO: use the register pre delay-slot
	// TODO: Exception(AddressError);
}

void JitX64::EI(InstructionData& data) {
	// TODO: implement interrupts
}

void JitX64::LW(InstructionData& data) {
	// base = rs
	EmitLoadRegister(s1, R32, data.rs);						// vaddr <- base
	if (data.imm) cc.add(s1.r32(), (u32)(i16)data.imm);		// vaddr += (i16)imm
	
	EmitReadVirtualMemory32(s2.r32(), s1);					// s2 <- [vaddr]
	EmitStoreRegister(R64, data.rt, s2.r32(), true);		// rt <- data
}

void JitX64::SD(InstructionData& data) {
	// base = rs
	EmitLoadRegister(s1, R32, data.rs);						// vaddr <- base
	if (data.imm) cc.add(s1.r32(), (u32)(i16)data.imm);		// vaddr += (i16)imm
	
	EmitLoadRegister(s2, R64, data.rt);						// s2 <- rt
	EmitWriteVirtualMemory64(s1, s2);						// [vaddr] <- rt
}

void JitX64::SW(InstructionData& data) {
	// base = rs
	EmitLoadRegister(s1, R32, data.rs);						// vaddr <- base
	if (data.imm) cc.add(s1.r32(), (u32)(i16)data.imm);		// vaddr += (i16)imm
	
	EmitLoadRegister(s2, R32, data.rt);						// s2 <- rt
	EmitWriteVirtualMemory32(s1, s2.r32());					// [vaddr] <- rt
}

void JitX64::MULT(InstructionData& data) {
	EmitLoadRegister(s1, R32, data.rs);						// s1 <- rs
	EmitLoadRegister(s2, R32, data.rt);						// s2 <- rt

	cc.movsxd(s1, s1.r32());								// s1 <- sign_extend(s1)
	cc.movsxd(s2, s2.r32());								// s2 <- sign_extend(s2)
	cc.imul(s1, s2);										// s1 *= s2

	EmitStoreSpecialRegister(SpecialRegName::LO, s1.r32());	// LO <- s1.r32
	cc.sar(s1, 32);											// s1 >> 32
	EmitStoreSpecialRegister(SpecialRegName::HI, s1.r32());	// HI <- s1.r32
}

void JitX64::ADDU(InstructionData& data) {
	EmitLoadRegister(s1, R32, data.rs);						// s1 <- rs
	EmitLoadRegister(s2, R32, data.rt);						// s2 <- rt
	cc.add(s1.r32(), s2.r32());								// s1 += s2
	EmitStoreRegister(R64, data.rd, s1.r32(), true);		// rd <- s1
}

void JitX64::LHU(InstructionData& data) {
	// base = rs
	EmitLoadRegister(s1, R32, data.rs);						// vaddr <- base
	if (data.imm) cc.add(s1.r32(), (u32)(i16)data.imm);		// vaddr += (i16)imm
	
	EmitReadVirtualMemory16(s2.r16(), s1);					// s2 <- [vaddr]
	EmitStoreRegister(R64, data.rt, s2, false);				// rt <- data
}

void JitX64::SH(InstructionData& data) {
	// base = rs
	EmitLoadRegister(s1, R32, data.rs);						// vaddr <- base
	if (data.imm) cc.add(s1.r32(), (u32)(i16)data.imm);		// vaddr += (i16)imm
	
	EmitLoadRegister(s2.r16(), R16, data.rt);				// s2 <- rt
	EmitWriteVirtualMemory16(s1, s2);						// [vaddr] <- rt
}

void JitX64::ORI(InstructionData& data) {
	EmitLoadRegister(s1, R64, data.rs);						// s1 <- rs
	if (data.imm) cc.or_(s1, (u16)data.imm);				// s1 |= imm
	EmitStoreRegister(R64, data.rt, s1, false);				// rt <- s1
}

void JitX64::AND(InstructionData& data) {
	EmitLoadRegister(s1, R64, data.rs);						// s1 <- rs
	EmitLoadRegister(s2, R64, data.rt);						// s2 <- rt
	cc.and_(s1, s2);										// s1 &= s2
	EmitStoreRegister(R64, data.rd, s1, false);				// rd <- s1
}

void JitX64::SYNC(InstructionData& data) {
	// this instruction is handled by the backend
}

void JitX64::LD(InstructionData& data) {
	// base = rs
	EmitLoadRegister(s1, R32, data.rs);						// vaddr <- base
	if (data.imm) cc.add(s1.r32(), (u32)(i16)data.imm);		// vaddr += (i16)imm
	
	EmitReadVirtualMemory64(s2, s1);						// s2 <- [vaddr]
	EmitStoreRegister(R64, data.rt, s2, false);				// rt <- data
}

void JitX64::DSRL(InstructionData& data) {
	EmitLoadRegister(s1, R64, data.rt);						// s1 <- rt
	cc.shr(s1, data.sa);									// s1 >> sa (logical)
	EmitStoreRegister(R64, data.rd, s1, false);				// rd <- s1
}

void JitX64::ANDI(InstructionData& data) {
	EmitLoadRegister(s1, R64, data.rs);						// s1 <- rs
	if (data.imm) cc.and_(s1, (u16)data.imm);				// s1 &= imm
	EmitStoreRegister(R64, data.rt, s1, false);				// rt <- s1
}

void JitX64::LBU(InstructionData& data) {
	// base = rs
	EmitLoadRegister(s1, R32, data.rs);						// vaddr <- base
	if (data.imm) cc.add(s1.r32(), (u32)(i16)data.imm);		// vaddr += (i16)imm
	
	EmitReadVirtualMemory8(s2.r8(), s1);					// s2 <- [vaddr]
	EmitStoreRegister(R64, data.rt, s2, false);				// rt <- data
}

void JitX64::SRL(InstructionData& data) {
	EmitLoadRegister(s1, R32, data.rt);						// s1 <- rt
	cc.shr(s1.r32(), data.sa);								// s1 >> sa (logical)
	EmitStoreRegister(R64, data.rd, s1.r32(), true);		// rd <- s1
}

void JitX64::DSLL(InstructionData& data) {
	EmitLoadRegister(s1, R64, data.rt);						// s1 <- rt
	cc.shl(s1, data.sa);									// s1 << sa (logical)
	EmitStoreRegister(R64, data.rd, s1, false);				// rd <- s1
}

void JitX64::OR(InstructionData& data) {
	EmitLoadRegister(s1, R64, data.rs);						// s1 <- rs
	EmitLoadRegister(s2, R64, data.rt);						// s2 <- rt
	cc.or_(s1, s2);											// s1 |= s2
	EmitStoreRegister(R64, data.rd, s1, false);				// rd <- s1
}

void JitX64::DSLL32(InstructionData& data) {
	EmitLoadRegister(s1, R64, data.rt);						// s1 <- rt
	cc.shl(s1, data.sa + 32);								// s1 << (sa + 32) (logical)
	EmitStoreRegister(R64, data.rd, s1, false);				// rd <- s1
}

void JitX64::BEQ(InstructionData& data) {
	// optimization: rs == rt -> forced branch
	if (data.rs == data.rt) {
		EmitJump((m_CompilePC - 4) + (i32)((i16)data.imm << 2));
		return;
	}

	// compare rs and rt
    EmitLoadRegister(s1, R64, data.rs);
    EmitLoadRegister(s2, R64, data.rt);

	Label exit_beq = cc.new_label();
    cc.cmp(s1, s2);
	cc.j(x86::CondCode::kNotEqual, exit_beq);

	// PC is after the branch delay slot so we have to go back 1 instruction to use it as a base for the branch
	EmitJump((m_CompilePC - 4) + (i32)((i16)data.imm << 2));

	cc.bind(exit_beq);
}

void JitX64::SB(InstructionData& data) {
	// base = rs
	EmitLoadRegister(s1, R32, data.rs);						// vaddr <- base
	if (data.imm) cc.add(s1.r32(), (u32)(i16)data.imm);		// vaddr += (i16)imm
	
	EmitLoadRegister(s2, R8, data.rt);						// s2 <- rt
	EmitWriteVirtualMemory8(s1.r32(), s2.r8());					// [vaddr] <- rt
}