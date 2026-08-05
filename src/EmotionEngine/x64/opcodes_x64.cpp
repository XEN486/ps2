#include "jit_x64.hpp"

using namespace EmotionEngine::Core;
using namespace asmjit;

static u32 WRAP_ReadCOP0(EmotionEngine::Core::R5900* r5900, u8 reg) { return r5900->ReadCOP0(reg); }
static void WRAP_WriteCOP0(EmotionEngine::Core::R5900* r5900, u8 reg, u32 val) { r5900->WriteCOP0(reg, val); }

void JitX64::MFC0(InstructionData& data) {
	if (data.rt == 0) return;
	InvokeNode* node;
	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(&WRAP_ReadCOP0), FuncSignature::build<u32, R5900*, u8>());
	node->set_arg(0, r5900);
	node->set_arg(1, data.rd);
	node->set_ret(0, r[data.rt].r32());
	cc.movsxd(r[data.rt].r64(), r[data.rt].r32());
}

void JitX64::SLL(InstructionData& data) {
	if (data.rd == 0) return;
	x86::Gp temp = cc.new_gp32();
	cc.mov(temp, r[data.rt].r32());
	cc.shl(temp, data.sa);
	cc.movsxd(r[data.rd], temp);
}

void JitX64::SLTI(InstructionData& data) {
	if (data.rt == 0) return;
	x86::Gp temp = cc.new_gp64();
	cc.cmp(r[data.rs], (u64)(i16)data.imm);
	cc.set(x86::CondCode::kSignedLT, temp.r8());
	cc.movzx(r[data.rt], temp.r8());
}

void JitX64::BNE(InstructionData& data) {
	Label exit_bne = cc.new_label();
	Label end = cc.new_label();
	cc.cmp(r[data.rs], r[data.rt]);
	cc.j(x86::CondCode::kEqual, exit_bne);

	EmitJump((data.pc + 4) + (i32)((i16)data.imm << 2));
	EmitBranchDelay(data);
	cc.jmp(end);

	cc.bind(exit_bne);
	if (!data.likely) {
		EmitBranchDelay(data);
	}

	cc.bind(end);
}

void JitX64::LUI(InstructionData& data) {
	if (data.rt == 0) return;
	cc.mov(r[data.rt].r32(), data.imm << 16);
	cc.movsxd(r[data.rt].r64(), r[data.rt].r32());
}

void JitX64::ORI(InstructionData& data) {
	if (data.rt == 0) return;
	x86::Gp temp = cc.new_gp64();
	cc.mov(temp, r[data.rs]);
	cc.or_(temp, data.imm);
	cc.mov(r[data.rt], temp);
}

void JitX64::JR(InstructionData& data) {
	EmitJump(r[data.rs].r32());
}