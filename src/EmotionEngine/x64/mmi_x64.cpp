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

	InvokeNode* node;
	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_lq), FuncSignature::build<void, R5900*, Memory*, u32, u8>());
	node->set_arg(0, m_R5900);
	node->set_arg(1, m_Memory);
	node->set_arg(2, temp.r32());
	node->set_arg(3, data.rt);
	LoadRegisters({data.rt});
}

void JitX64::SQ(InstructionData& data) {
	cc.lea(temp.r32(), x86::ptr(r[data.rs].r32(), (i32)(i16)data.imm));
	
	FlushRegisters({data.rt});
	InvokeNode* node;
	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_sq), FuncSignature::build<void, R5900*, Memory*, u32, u8>());
	node->set_arg(0, m_R5900);
	node->set_arg(1, m_Memory);
	node->set_arg(2, temp.r32());
	node->set_arg(3, data.rt);
}

void JitX64::POR(InstructionData& data) {
	x86::Vec rs = cc.new_vec128();
	x86::Vec rt = cc.new_vec128();

	FlushRegisters({data.rs, data.rt});
	EmitLoad128(rs, data.rs);
	EmitLoad128(rt, data.rt);
	cc.por(rs, rt);
	EmitStore128(data.rd, rs);
	LoadRegisters({data.rd});
}

void JitX64::PMFHI(InstructionData& data) {
	x86::Vec hi = cc.new_vec128("HI");
	cc.movdqu(hi, x86::oword_ptr(r5900, offsetof(R5900, hi)));

	EmitStore128(data.rd, hi);
	LoadRegisters({data.rd});
}

void JitX64::PMFLO(InstructionData& data) {
	x86::Vec lo = cc.new_vec128("LO");
	cc.movdqu(lo, x86::oword_ptr(r5900, offsetof(R5900, lo)));

	EmitStore128(data.rd, lo);
	LoadRegisters({data.rd});
}

void JitX64::PCPYLD(InstructionData& data) {
	asmjit::x86::Vec rs = cc.new_vec128("rs");
	asmjit::x86::Vec rt = cc.new_vec128("rt");

	// load rs and rt
	FlushRegisters({data.rs, data.rt});
	EmitLoad128(rs, data.rs);
	EmitLoad128(rt, data.rt);

	cc.punpcklqdq(rt, rs);
	EmitStore128(data.rd, rt);
	LoadRegisters({data.rd});
}

void JitX64::PCPYHD(InstructionData& data) {
	asmjit::x86::Vec rs = cc.new_vec128("rs");
	asmjit::x86::Vec rt = cc.new_vec128("rt");

	// load rs and rt
	FlushRegisters({data.rs, data.rt});
	EmitLoad128(rs, data.rs);
	EmitLoad128(rt, data.rt);

	cc.punpckhqdq(rt, rs);
	EmitStore128(data.rd, rt);
	LoadRegisters({data.rd});
}

void JitX64::PEXTLW(InstructionData& data) {
	asmjit::x86::Vec rs = cc.new_vec128("rs");
	asmjit::x86::Vec rt = cc.new_vec128("rt");

	// load rs and rt
	FlushRegisters({data.rs, data.rt});
	EmitLoad128(rs, data.rs);
	EmitLoad128(rt, data.rt);

	cc.punpckldq(rt, rs);
	EmitStore128(data.rd, rt);
	LoadRegisters({data.rd});
}