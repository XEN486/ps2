#include "jit_x64.hpp"
#include "../../hle/bios.hpp"

using namespace EmotionEngine::Core;
using namespace asmjit;

void JitX64::LUI(InstructionData& data) {
	u32 shifted = data.imm << 16;
	EmitStoreRegister(R64, data.rt, shifted, true);
}

void JitX64::ADDIU(InstructionData& data) {
	EmitLoadRegister(s1, R32, data.rs);						// s1 <- rs
	if (data.imm) cc.add(s1.r32(), (i32)(i16)data.imm);		// s1 += imm (signed)
	EmitStoreRegister(R64, data.rt, s1.r32(), true);		// rt <- s1
}

void JitX64::SLL(InstructionData& data) {
	// optimization: rd == rt && sa == 0 -> NOP
	if (data.rd == data.rt && data.sa == 0) {
		return;
	}

	EmitLoadRegister(s1, R32, data.rt);
	cc.shl(s1.r32(), data.sa);
	EmitStoreRegister(R64, data.rd, s1.r32(), true);
}

void JitX64::SQ(InstructionData& data) {
	// vaddr = s1
	// base = rs
	EmitLoadRegister(s1, R32, data.rs);					// vaddr <- base
	if (data.imm) cc.add(s1.r32(), (i32)(i16)data.imm);	// vaddr += (i16)imm
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
	Label end = cc.new_label();

    cc.cmp(s1, s2);
	cc.j(x86::CondCode::kEqual, exit_bne);

	EmitBranchDelay();

	// PC is after the branch delay slot so we have to go back 1 instruction to use it as a base for the branch
	EmitJump((m_CompilePC - 4) + (i32)((i16)data.imm << 2));

	cc.bind(exit_bne);
	if (!data.likely) {
		EmitBranchDelay();
	}

	cc.bind(end);
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

	// TODO: Exception(AddressError);
}

void JitX64::EI(InstructionData& data) {
	// TODO: implement interrupts
}

void JitX64::LW(InstructionData& data) {
	// base = rs
	EmitLoadRegister(s1, R32, data.rs);						// vaddr <- base
	if (data.imm) cc.add(s1.r32(), (i32)(i16)data.imm);		// vaddr += (i16)imm
	
	EmitReadVirtualMemory32(s2.r32(), s1);					// s2 <- [vaddr]
	EmitStoreRegister(R64, data.rt, s2.r32(), true);		// rt <- data
}

void JitX64::SD(InstructionData& data) {
	// base = rs
	EmitLoadRegister(s1, R32, data.rs);						// vaddr <- base
	if (data.imm) cc.add(s1.r32(), (i32)(i16)data.imm);		// vaddr += (i16)imm
	
	EmitLoadRegister(s2, R64, data.rt);						// s2 <- rt
	EmitWriteVirtualMemory64(s1, s2);						// [vaddr] <- rt
}

void JitX64::SW(InstructionData& data) {
	// base = rs
	EmitLoadRegister(s1, R32, data.rs);						// vaddr <- base
	if (data.imm) cc.add(s1.r32(), (i32)(i16)data.imm);		// vaddr += (i16)imm
	
	EmitLoadRegister(s2, R32, data.rt);						// s2 <- rt
	EmitWriteVirtualMemory32(s1, s2.r32());					// [vaddr] <- rt
}

void JitX64::MULT(InstructionData& data) {
	EmitLoadRegister(s1, R32, data.rs);						// s1 <- rs
	EmitLoadRegister(s2, R32, data.rt);						// s2 <- rt

	cc.push(x86::rax);										// rs and low result
	cc.push(x86::rbx);										// r5900 could be overwritten so we save it here
	cc.push(x86::rcx);										// rt
	cc.push(x86::rdx);										// high result

	cc.mov(x86::rbx, r5900);								// save r5900 as if it was in rax, rcx or rdx it would get corrupted

	cc.mov(x86::eax, s1.r32());								// eax <- s1
	cc.mov(x86::ecx, s2.r32());								// ecx <- s2

	// cc.imul() AND cc.emit() dont support implicit form
	cc.embed_uint16(0xe9f7);								// imul ecx (edx:eax = eax * ecx)

	cc.movsxd(x86::rax, x86::eax);							// rax <- sign_extend(eax)
	cc.movsxd(x86::rdx, x86::edx);							// rdx <- sign_extend(edx)

	// store result
	cc.mov(x86::qword_ptr(x86::rbx, offsetof(R5900, hi)), x86::rdx);
	cc.mov(x86::qword_ptr(x86::rbx, offsetof(R5900, lo)), x86::rax);

	// restore registers
	cc.pop(x86::rdx);
	cc.pop(x86::rcx);
	cc.pop(x86::rbx);
	cc.pop(x86::rax);
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
	if (data.imm) cc.add(s1.r32(), (i32)(i16)data.imm);		// vaddr += (i16)imm
	
	EmitReadVirtualMemory16(s2.r16(), s1);					// s2 <- [vaddr]
	EmitStoreRegister(R64, data.rt, s2, false);				// rt <- data
}

void JitX64::SH(InstructionData& data) {
	// base = rs
	EmitLoadRegister(s1, R32, data.rs);						// vaddr <- base
	if (data.imm) cc.add(s1.r32(), (i32)(i16)data.imm);		// vaddr += (i16)imm
	
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
	if (data.imm) cc.add(s1.r32(), (i32)(i16)data.imm);		// vaddr += (i16)imm
	
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
	if (data.imm) cc.add(s1.r32(), (i32)(i16)data.imm);		// vaddr += (i16)imm
	
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
		EmitBranchDelay();
		EmitJump((m_CompilePC - 4) + (i32)((i16)data.imm << 2));
		return;
	}

	// compare rs and rt
    EmitLoadRegister(s1, R64, data.rs);
    EmitLoadRegister(s2, R64, data.rt);

	Label exit_beq = cc.new_label();
	Label end = cc.new_label();

    cc.cmp(s1, s2);
	cc.j(x86::CondCode::kNotEqual, exit_beq);

	EmitBranchDelay();

	// PC is after the branch delay slot so we have to go back 1 instruction to use it as a base for the branch
	EmitJump((m_CompilePC - 4) + (i32)((i16)data.imm << 2));

	cc.bind(exit_beq);
	if (!data.likely) {
		EmitBranchDelay();
	}

	cc.bind(end);
}

void JitX64::SB(InstructionData& data) {
	// base = rs
	EmitLoadRegister(s1, R32, data.rs);						// vaddr <- base
	if (data.imm) cc.add(s1.r32(), (i32)(i16)data.imm);		// vaddr += (i16)imm
	
	EmitLoadRegister(s2, R8, data.rt);						// s2 <- rt
	EmitWriteVirtualMemory8(s1, s2.r8());					// [vaddr] <- rt
}

void JitX64::SWC1(InstructionData& data) {
	// base = rs
	EmitLoadRegister(s1, R32, data.rs);						// vaddr <- base
	if (data.imm) cc.add(s1.r32(), (i16)data.imm);		// vaddr += (i16)imm

	EmitLoadFPR(s2.r32(), data.rt);							// s2 <- ft
	EmitWriteVirtualMemory32(s1, s2.r32());					// [vaddr] <- rt
}

void JitX64::SLTIU(InstructionData& data) {
	// compare rs and imm
    EmitLoadRegister(s1, R64, data.rs);
    cc.cmp(s1, (i32)(i16)data.imm);

    // s1 < imm (unsigned) -> s1 = 1, else s1 = 0
    cc.setb(s1.r8());
    cc.movzx(s1, s1.r8()); // zero extend to 64-bit

	// rt <- s1
    EmitStoreRegister(R64, data.rt, s1, false);
}

void JitX64::DIVU(InstructionData& data) {
	EmitLoadRegister(s1, R32, data.rs);
	EmitLoadRegister(s2, R32, data.rt);

	Label div = cc.new_label();
	Label done = cc.new_label();

	// check if we are dividing by zero
	cc.test(s2.r32(), s2.r32());
	cc.jnz(div);

	// set division by zero results
	EmitStoreSpecialRegister(SpecialRegName::HI, s1.r32()); // HI <- numerator
	cc.mov(s1.r32(), 0xffffffff);
	EmitStoreSpecialRegister(SpecialRegName::LO, s1.r32());	// LO <- 0xffffffff
	cc.jmp(done);

	// proper division
	cc.bind(div);
	cc.push(x86::rax);										// rs and quotient result
	cc.push(x86::rbx);										// r5900 could be overwritten so we save it here
	cc.push(x86::rcx);										// rt
	cc.push(x86::rdx);										// remainder result

	cc.mov(x86::rbx, r5900);								// save r5900 as if it was in rax, rcx or rdx it would get corrupted

	cc.xor_(x86::edx, x86::edx);							// edx <- 0
	cc.mov(x86::eax, s1.r32());								// eax <- s1
	cc.mov(x86::ecx, s2.r32());								// ecx <- s2

	// cc.div() doesnt support implicit form
	cc.emit(x86::Inst::kIdDiv, x86::ecx);					// eax <- (eax / ecx), edx <- (eax % ecx)

	cc.movsxd(x86::rax, x86::eax);							// rax <- sign_extend(eax)
	cc.movsxd(x86::rdx, x86::edx);							// rdx <- sign_extend(edx)

	// store result
	cc.mov(x86::qword_ptr(x86::rbx, offsetof(R5900, hi)), x86::rdx);
	cc.mov(x86::qword_ptr(x86::rbx, offsetof(R5900, lo)), x86::rax);

	// restore registers
	cc.pop(x86::rdx);
	cc.pop(x86::rcx);
	cc.pop(x86::rbx);
	cc.pop(x86::rax);

	cc.bind(done);
}

void JitX64::MFHI(InstructionData& data) {
	cc.mov(t1, x86::qword_ptr(r5900, offsetof(R5900, hi)));
	EmitStoreRegister(R64, data.rd, t1, false);
}

void JitX64::BREAK(InstructionData& data) {
	cc.int3();
}