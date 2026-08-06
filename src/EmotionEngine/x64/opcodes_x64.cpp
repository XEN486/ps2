#include "jit_x64.hpp"

using namespace EmotionEngine::Core;
using namespace asmjit;

static u32 WRAP_ReadCOP0(EmotionEngine::Core::R5900* r5900, u8 reg) { return r5900->ReadCOP0(reg); }
static void WRAP_WriteCOP0(EmotionEngine::Core::R5900* r5900, u8 reg, u32 val) { r5900->WriteCOP0(reg, val); }

static void IMPL_div(EmotionEngine::Core::R5900* r5900, bool pipeline1, u8 index_rs, u8 index_rt) {
	GPR& rs = r5900->gpr[index_rs];
	GPR& rt = r5900->gpr[index_rt];

	u64& hi = pipeline1 ? r5900->hi1 : r5900->hi;
	u64& lo = pipeline1 ? r5900->lo1 : r5900->lo;

	if (rs.reg_i32[0] == INT32_MIN && rt.reg_i32[0] == -1) {
		lo = (u64)(i64)(i32)0x80000000;
		hi = 0;
	} else if (rt.reg_i32[0] != 0) {
		lo = (u64)(i64)(i32)(rs.reg_i32[0] / rt.reg_i32[0]);
		hi = (u64)(i64)(i32)(rs.reg_i32[0] % rt.reg_i32[0]);
	} else {
		lo = (u64)(i64)((rs.reg_i32[0] < 0) ? 1 : -1);
		hi = (u64)(i64)(i32)rs.reg_i32[0];
	}

	if (index_rs && !pipeline1) r5900->gpr[index_rs].reg_u64[0] = lo;
}

static void IMPL_divu(EmotionEngine::Core::R5900* r5900, bool pipeline1, u8 index_rs, u8 index_rt) {
	u32 numerator   = r5900->gpr[index_rs].reg_u32[0];
	u32 denominator = r5900->gpr[index_rt].reg_u32[0];

	u64& hi = pipeline1 ? r5900->hi1 : r5900->hi;
	u64& lo = pipeline1 ? r5900->lo1 : r5900->lo;

	if (denominator != 0) {
		lo = (u64)(i64)(i32)(numerator / denominator);
		hi = (u64)(i64)(i32)(numerator % denominator);
	} else {
		lo = (u64)(i64)-1;
		hi = (u64)(i64)(i32)numerator;
	}

	//if (index_rs && !pipeline1) r5900->gpr[index_rs].reg_u64[0] = lo;
}

static void IMPL_mult(EmotionEngine::Core::R5900* r5900, bool pipeline1, u8 index_rs, u8 index_rt, u8 index_rd) {
	i64 result = (i64)r5900->gpr[index_rs].reg_i32[0] * (i64)r5900->gpr[index_rt].reg_i32[0];

	u64& hi = pipeline1 ? r5900->hi1 : r5900->hi;
	u64& lo = pipeline1 ? r5900->lo1 : r5900->lo;

	lo = (u64)(i64)(i32)result;
	hi = (u64)(i64)(i32)(result >> 32);

	if (index_rd) r5900->gpr[index_rd].reg_u64[0] = lo;
}

static void IMPL_multu(EmotionEngine::Core::R5900* r5900, bool pipeline1, u8 index_rs, u8 index_rt) {
	u64 result = (u64)r5900->gpr[index_rs].reg_u32[0] * (u64)r5900->gpr[index_rt].reg_u32[0];
	u64& hi = pipeline1 ? r5900->hi1 : r5900->hi;
	u64& lo = pipeline1 ? r5900->lo1 : r5900->lo;

	lo = (u64)(i64)(i32)result;
	hi = (u64)(i64)(i32)(result >> 32);

	//if (index_rd) r5900->gpr[index_rd].reg_u64[0] = lo;
}

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
	cc.mov(temp.r32(), r[data.rt].r32());
	cc.shl(temp.r32(), data.sa);
	cc.movsxd(r[data.rd], temp.r32());
}

void JitX64::SLTI(InstructionData& data) {
	if (data.rt == 0) return;
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
	cc.mov(temp, r[data.rs]);
	cc.or_(temp, data.imm);
	cc.mov(r[data.rt], temp);
}

void JitX64::JR(InstructionData& data) {
	EmitJump(r[data.rs].r32());
}

void JitX64::MTC0(InstructionData& data) {
	InvokeNode* node;
	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(&WRAP_WriteCOP0), FuncSignature::build<void, R5900*, u8, u32>());
	node->set_arg(0, r5900);
	node->set_arg(1, data.rd);
	node->set_arg(2, r[data.rt].r32());
}

void JitX64::ADDIU(InstructionData& data) {
	if (data.rt == 0) return;
	cc.mov(temp.r32(), r[data.rs].r32());
	cc.add(temp.r32(), (i32)(i16)data.imm);
	cc.movsxd(r[data.rt], temp.r32());
}

void JitX64::SW(InstructionData& data) {
	cc.mov(temp.r32(), r[data.rs].r32());
	if (data.imm) cc.add(temp.r32(), (i32)(i16)data.imm);
	EmitWriteVirtualMemory<u32>(temp.r32(), r[data.rt].r32());
}

void JitX64::JALR(InstructionData& data) {
	cc.mov(temp.r32(), r[data.rs].r32());
	if (data.rd) cc.mov(r[data.rd], data.pc + 8);
	EmitJump(temp.r32());
}

void JitX64::SD(InstructionData& data) {
	cc.mov(temp.r32(), r[data.rs].r32());
	if (data.imm) cc.add(temp.r32(), (i32)(i16)data.imm);
	EmitWriteVirtualMemory<u64>(temp.r32(), r[data.rt]);
}

void JitX64::JAL(InstructionData& data) {
	cc.mov(r[31], data.pc + 8);
	EmitJump((data.pc & 0xf0000000) | (data.addr << 2));
}

void JitX64::SRA(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(temp.r32(), r[data.rt].r32());
	cc.sar(temp.r32(), data.sa);
	cc.movsxd(r[data.rd], temp.r32());
}

void JitX64::BGEZ(InstructionData& data) {
	Label exit_bgez = cc.new_label();
	Label end = cc.new_label();
	cc.cmp(r[data.rs], 0);
	cc.j(x86::CondCode::kSignedLT, exit_bgez);

	EmitJump((data.pc + 4) + (i32)((i16)data.imm << 2));
	EmitBranchDelay(data);
	cc.jmp(end);

	cc.bind(exit_bgez);
	if (!data.likely) {
		EmitBranchDelay(data);
	}

	cc.bind(end);	
}

void JitX64::LBU(InstructionData& data) {
	if (data.rt == 0) return;
	cc.mov(temp.r32(), r[data.rs].r32());
	if (data.imm) cc.add(temp.r32(), (i32)(i16)data.imm);
	EmitReadVirtualMemory<u8>(r[data.rt].r8(), temp.r32());
	cc.movzx(r[data.rt].r64(), r[data.rt].r8());
}

void JitX64::ANDI(InstructionData& data) {
	if (data.rt == 0) return;
	cc.mov(temp, r[data.rs]);
	cc.and_(temp.r32(), data.imm);
	cc.mov(r[data.rt], temp);
}

void JitX64::BEQ(InstructionData& data) {
	Label exit_beq = cc.new_label();
	Label end = cc.new_label();
	cc.cmp(r[data.rs], r[data.rt]);
	cc.j(x86::CondCode::kNotEqual, exit_beq);

	EmitJump((data.pc + 4) + (i32)((i16)data.imm << 2));
	EmitBranchDelay(data);
	cc.jmp(end);

	cc.bind(exit_beq);
	if (!data.likely) {
		EmitBranchDelay(data);
	}

	cc.bind(end);
}

void JitX64::LD(InstructionData& data) {
	if (data.rt == 0) return;
	cc.mov(temp.r32(), r[data.rs].r32());
	if (data.imm) cc.add(temp.r32(), (i32)(i16)data.imm);
	EmitReadVirtualMemory<u64>(r[data.rt], temp.r32());
}

void JitX64::DSRL(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(temp, r[data.rt]);
	cc.shr(temp, data.sa);
	cc.mov(r[data.rd], temp);
}

void JitX64::DSLL(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(temp, r[data.rt]);
	cc.shl(temp, data.sa);
	cc.mov(r[data.rd], temp);
}

void JitX64::DSLL32(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(temp, r[data.rt]);
	cc.shl(temp, data.sa + 32);
	cc.mov(r[data.rd], temp);
}

void JitX64::DSRA32(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(temp, r[data.rt]);
	cc.sar(temp, data.sa + 32);
	cc.mov(r[data.rd], temp);
}

void JitX64::OR(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(temp, r[data.rs]);
	cc.or_(temp, r[data.rt]);
	cc.mov(r[data.rd], temp);
}

void JitX64::DADDU(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(temp, r[data.rs]);
	cc.add(temp, r[data.rt]);
	cc.mov(r[data.rd], temp);
}

void JitX64::LW(InstructionData& data) {
	if (data.rt == 0) return;
	cc.mov(temp.r32(), r[data.rs].r32());
	if (data.imm) cc.add(temp.r32(), (i32)(i16)data.imm);
	EmitReadVirtualMemory<u32>(r[data.rt].r32(), temp.r32());
}

void JitX64::MULT(InstructionData& data) {
	FlushRegisters();
	InvokeNode* node;
	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_mult), FuncSignature::build<void, R5900*, bool, u8, u8, u8>());
	node->set_arg(0, r5900);
	node->set_arg(1, data.pipeline1);
	node->set_arg(2, data.rs);
	node->set_arg(3, data.rt);
	node->set_arg(4, data.rd);
	LoadRegisters();
}

void JitX64::MULTU(InstructionData& data) {
	FlushRegisters();
	InvokeNode* node;
	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_multu), FuncSignature::build<void, R5900*, bool, u8, u8>());
	node->set_arg(0, r5900);
	node->set_arg(1, data.pipeline1);
	node->set_arg(2, data.rs);
	node->set_arg(3, data.rt);
	LoadRegisters();
}

void JitX64::DIV(InstructionData& data) {
	FlushRegisters();
	InvokeNode* node;
	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_div), FuncSignature::build<void, R5900*, bool, u8, u8>());
	node->set_arg(0, r5900);
	node->set_arg(1, data.pipeline1);
	node->set_arg(2, data.rs);
	node->set_arg(3, data.rt);
	LoadRegisters();
}

void JitX64::DIVU(InstructionData& data) {
	FlushRegisters();
	InvokeNode* node;
	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_divu), FuncSignature::build<void, R5900*, bool, u8, u8>());
	node->set_arg(0, r5900);
	node->set_arg(1, data.pipeline1);
	node->set_arg(2, data.rs);
	node->set_arg(3, data.rt);
	LoadRegisters();
}