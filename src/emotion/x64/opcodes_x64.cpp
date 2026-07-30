#include "jit_x64.hpp"

using namespace EmotionEngine::MIPS;
using namespace asmjit;

void JitX64::LUI(InstructionData& data) {
	u32 shifted = data.imm << 16;
	EmitStoreRegister(R64, data.rt, shifted);
}

void JitX64::ADDIU(InstructionData& data) {
	EmitLoadRegister(s1, R32, data.rs);			// s1 <- rs
	cc.add(s1.r32(), (i16)data.imm);			// s1 += imm (signed)
	EmitStoreRegister(R64, data.rt, s1.r32());	// rt <- s1
}

void JitX64::SLL(InstructionData& data) {
	// optimization: rd == rt && sa == 0 -> NOP
	if (data.rd == data.rt && data.sa == 0) {
		return;
	}

	EmitLoadRegister(s1, R32, data.rt);
	cc.shl(s1, data.sa);
	EmitStoreRegister(R64, data.rd, s1.r32());
}

void JitX64::SQ(InstructionData& data) {
	// vaddr = s1
	// base = rs
	EmitLoadRegister(s1, R32, data.rs);		// vaddr <- base
	cc.add(s1.r32(), (i16)data.imm);		// vaddr += (i16)imm
	cc.and_(s1.r32(), 0xfffffff0);			// vaddr &= 0xfffffff0
	
	EmitLoadRegister128(v1, data.rt);		// v1 <- gpr[rt].128
	EmitWriteVirtualMemory128(s1, v1);		// vaddr <- v1
}