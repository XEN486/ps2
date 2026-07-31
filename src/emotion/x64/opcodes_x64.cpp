#include "jit_x64.hpp"
#include "../../hle/bios.hpp"

using namespace EmotionEngine::MIPS;
using namespace asmjit;

void JitX64::LUI(InstructionData& data) {
	u32 shifted = data.imm << 16;
	EmitStoreRegister(R64, data.rt, shifted, true);
}

void JitX64::ADDIU(InstructionData& data) {
	EmitLoadRegister(s1, R32, data.rs);					// s1 <- rs
	if (data.imm) cc.add(s1.r32(), (i16)data.imm);		// s1 += imm (signed)
	EmitStoreRegister(R64, data.rt, s1.r32(), true);	// rt <- s1
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
	EmitLoadRegister(s1, R32, data.rs);				// vaddr <- base
	if (data.imm) cc.add(s1.r32(), (i16)data.imm);	// vaddr += (i16)imm
	cc.and_(s1.r32(), 0xfffffff0);					// vaddr &= 0xfffffff0
	
	EmitLoadRegister128(v1, data.rt);				// v1 <- gpr[rt].128
	EmitWriteVirtualMemory128(s1, v1);				// vaddr <- v1
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