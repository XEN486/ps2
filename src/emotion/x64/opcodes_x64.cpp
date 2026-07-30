#include "jit_x64.hpp"

using namespace EmotionEngine::MIPS;
using namespace asmjit;

void JitX64::LUI(InstructionData& data) {
	u32 shifted = data.imm << 16;
	EmitStoreRegister(R64, data.rt, shifted);
}

void JitX64::ADDIU(InstructionData& data) {
	EmitLoadRegister(scratch1, R32, data.rs);				// scratch1 <- rs
	cc.add(scratch1.r32(), (i16)data.imm);					// scratch1 += imm (signed)
	EmitStoreRegister(R64, data.rt, scratch1.r32());	// rt <- scratch1
}

void JitX64::SLL(InstructionData& data) {
	// optimization: rd == rt && sa == 0 -> NOP
	if (data.rd == data.rt && data.sa == 0) {
		return;
	}

	debug_log("{}, {}", data.rt, data.rd);
	EmitLoadRegister(scratch1, R32, data.rt);
	cc.shl(scratch1, data.sa);
	EmitStoreRegister(R64, data.rd, scratch1.r32());
}