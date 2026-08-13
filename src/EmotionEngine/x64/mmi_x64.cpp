#include "jit_x64.hpp"

using namespace EmotionEngine::Core;
using namespace asmjit;

static void IMPL_lq(EmotionEngine::Core::R5900* r5900, EmotionEngine::Memory* memory, u32 addr, u8 reg) {
	addr &= ~0xf;
	u64 lo = memory->ReadVirtualMemory64(addr);
	u64 hi = memory->ReadVirtualMemory64(addr + 8);

	if (reg) {
		r5900->gpr[reg].reg_u64[0] = lo;
		r5900->gpr[reg].reg_u64[1] = hi;
	}
}

static void IMPL_sq(EmotionEngine::Core::R5900* r5900, EmotionEngine::Memory* memory, u32 addr, u8 reg) {
	addr &= ~0xf;
	memory->WriteVirtualMemory64(addr, r5900->gpr[reg].reg_u64[0]);
	memory->WriteVirtualMemory64(addr + 8, r5900->gpr[reg].reg_u64[1]);
}

void JitX64::LQ(InstructionData& data) {
	cc.lea(temp.r32(), x86::ptr(r[data.rs].r32(), (i32)(i16)data.imm));

	FlushRegisters();
	InvokeNode* node;
	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_lq), FuncSignature::build<void, R5900*, Memory*, u32, u8>());
	node->set_arg(0, m_R5900);
	node->set_arg(1, m_Memory);
	node->set_arg(2, temp.r32());
	node->set_arg(3, data.rt);
	LoadRegisters();
}

void JitX64::SQ(InstructionData& data) {
	cc.lea(temp.r32(), x86::ptr(r[data.rs].r32(), (i32)(i16)data.imm));
	
	FlushRegisters();
	InvokeNode* node;
	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_sq), FuncSignature::build<void, R5900*, Memory*, u32, u8>());
	node->set_arg(0, m_R5900);
	node->set_arg(1, m_Memory);
	node->set_arg(2, temp.r32());
	node->set_arg(3, data.rt);
	LoadRegisters();
}

void JitX64::POR(InstructionData& data) {
	x86::Vec rs = cc.new_vec128();
	x86::Vec rt = cc.new_vec128();

	FlushRegisters();
	EmitLoad128(rs, data.rs);
	EmitLoad128(rt, data.rt);
	cc.por(rs, rt);
	EmitStore128(data.rd, rs);
	LoadRegisters();
}