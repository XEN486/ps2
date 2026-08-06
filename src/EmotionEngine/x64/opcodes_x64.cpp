#include "jit_x64.hpp"

using namespace EmotionEngine::Core;
using namespace asmjit;

static u32 WRAP_ReadCOP0(EmotionEngine::Core::R5900* r5900, u8 reg) { return r5900->ReadCOP0(reg); }
static void WRAP_WriteCOP0(EmotionEngine::Core::R5900* r5900, u8 reg, u32 val) { r5900->WriteCOP0(reg, val); }

static void IMPL_lq(EmotionEngine::Core::R5900* r5900, EmotionEngine::Memory* memory, u32 addr, u8 reg) {
	r5900->gpr[reg].reg_u64[0] = memory->ReadVirtualMemory64(addr);
	r5900->gpr[reg].reg_u64[1] = memory->ReadVirtualMemory64(addr + 8);
}

static void IMPL_sq(EmotionEngine::Core::R5900* r5900, EmotionEngine::Memory* memory, u32 addr, u8 reg) {
	memory->WriteVirtualMemory64(addr, r5900->gpr[reg].reg_u64[0]);
	memory->WriteVirtualMemory64(addr + 8, r5900->gpr[reg].reg_u64[1]);
}

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

	//if (index_rs && !pipeline1) r5900->gpr[index_rs].reg_u64[0] = lo;
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

static void IMPL_multu(EmotionEngine::Core::R5900* r5900, bool pipeline1, u8 index_rs, u8 index_rt, u8 index_rd) {
	u64 result = (u64)r5900->gpr[index_rs].reg_u32[0] * (u64)r5900->gpr[index_rt].reg_u32[0];
	u64& hi = pipeline1 ? r5900->hi1 : r5900->hi;
	u64& lo = pipeline1 ? r5900->lo1 : r5900->lo;

	lo = (u64)(i64)(i32)result;
	hi = (u64)(i64)(i32)(result >> 32);

	if (index_rd) r5900->gpr[index_rd].reg_u64[0] = lo;
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
	cc.mov(r[data.rd].r32(), r[data.rt].r32());
	cc.shl(r[data.rd].r32(), data.sa);
	cc.movsxd(r[data.rd], r[data.rd].r32());
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
	cc.mov(r[data.rt], r[data.rs]);
	cc.or_(r[data.rt], data.imm);
}

void JitX64::JR(InstructionData& data) {
	EmitJump(r[data.rs].r32());
	EmitBranchDelay(data);
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
	cc.mov(r[data.rt].r32(), r[data.rs].r32());
	cc.add(r[data.rt].r32(), (i32)(i16)data.imm);
	cc.movsxd(r[data.rt], r[data.rt].r32());
}

void JitX64::SW(InstructionData& data) {
	cc.mov(temp.r32(), r[data.rs].r32());
	if (data.imm) cc.add(temp.r32(), (i32)(i16)data.imm);
	EmitWriteVirtualMemory<u32>(temp.r32(), r[data.rt].r32());
}

void JitX64::JALR(InstructionData& data) {
	if (data.rd) cc.mov(r[data.rd], data.pc + 8);
	EmitJump(r[data.rs].r32());
	EmitBranchDelay(data);
}

void JitX64::SD(InstructionData& data) {
	cc.mov(temp.r32(), r[data.rs].r32());
	if (data.imm) cc.add(temp.r32(), (i32)(i16)data.imm);
	EmitWriteVirtualMemory<u64>(temp.r32(), r[data.rt]);
}

void JitX64::JAL(InstructionData& data) {
	cc.mov(r[31], data.pc + 8);
	EmitJump(((data.pc + 4) & 0xf0000000) | (data.addr << 2));
	EmitBranchDelay(data);
}

void JitX64::SRA(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(r[data.rd].r32(), r[data.rt].r32());
	cc.sar(r[data.rd].r32(), data.sa);
	cc.movsxd(r[data.rd], r[data.rd].r32());
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
	cc.movzx(r[data.rt].r32(), r[data.rt].r8());
}

void JitX64::ANDI(InstructionData& data) {
	if (data.rt == 0) return;
	cc.mov(r[data.rt], r[data.rs]);
	cc.and_(r[data.rt], data.imm);
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
	cc.mov(r[data.rd], r[data.rt]);
	cc.shr(r[data.rd], data.sa);
}

void JitX64::DSLL(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(r[data.rd], r[data.rt]);
	cc.shl(r[data.rd], data.sa);
}

void JitX64::DSLL32(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(r[data.rd], r[data.rt]);
	cc.shl(r[data.rd], data.sa + 32);
}

void JitX64::DSRA32(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(r[data.rd], r[data.rt]);
	cc.sar(r[data.rd], data.sa + 32);
}

void JitX64::OR(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(r[data.rd], r[data.rs]);
	cc.or_(r[data.rd], r[data.rt]);
}

void JitX64::DADDU(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(r[data.rd], r[data.rs]);
	cc.add(r[data.rd], r[data.rt]);
}

void JitX64::LW(InstructionData& data) {
	if (data.rt == 0) return;
	cc.mov(temp.r32(), r[data.rs].r32());
	if (data.imm) cc.add(temp.r32(), (i32)(i16)data.imm);
	EmitReadVirtualMemory<u32>(r[data.rt].r32(), temp.r32());
	cc.movsxd(r[data.rt].r64(), r[data.rt].r32());
}

void JitX64::MULT(InstructionData& data) {
	FlushRegisters();
	InvokeNode* node;
	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_mult), FuncSignature::build<void, R5900*, bool, u8, u8, u8>());
	node->set_arg(0, m_R5900);
	node->set_arg(1, data.pipeline1);
	node->set_arg(2, data.rs);
	node->set_arg(3, data.rt);
	node->set_arg(4, data.rd);
	LoadRegisters();
}

void JitX64::MULTU(InstructionData& data) {
	FlushRegisters();
	InvokeNode* node;
	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_multu), FuncSignature::build<void, R5900*, bool, u8, u8, u8>());
	node->set_arg(0, m_R5900);
	node->set_arg(1, data.pipeline1);
	node->set_arg(2, data.rs);
	node->set_arg(3, data.rt);
	node->set_arg(4, data.rd);
	LoadRegisters();
}

void JitX64::DIV(InstructionData& data) {
	FlushRegisters();
	InvokeNode* node;
	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_div), FuncSignature::build<void, R5900*, bool, u8, u8>());
	node->set_arg(0, m_R5900);
	node->set_arg(1, data.pipeline1);
	node->set_arg(2, data.rs);
	node->set_arg(3, data.rt);
	LoadRegisters();
}

void JitX64::DIVU(InstructionData& data) {
	FlushRegisters();
	InvokeNode* node;
	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_divu), FuncSignature::build<void, R5900*, bool, u8, u8>());
	node->set_arg(0, m_R5900);
	node->set_arg(1, data.pipeline1);
	node->set_arg(2, data.rs);
	node->set_arg(3, data.rt);
	LoadRegisters();
}

void JitX64::BREAK(InstructionData&) {
	cc.int3();
}

void JitX64::MFLO(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(r[data.rd], x86::qword_ptr(r5900, (data.pipeline1) ? offsetof(R5900, lo1) : offsetof(R5900, lo)));
}

void JitX64::ADDU(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(r[data.rd].r32(), r[data.rs].r32());
	cc.add(r[data.rd].r32(), r[data.rt].r32());
	cc.movsxd(r[data.rd], r[data.rd].r32());
}

void JitX64::SLT(InstructionData& data) {
	if (data.rd == 0) return;
	cc.cmp(r[data.rs], r[data.rt]);
	cc.set(x86::CondCode::kSignedLT, temp.r8());
	cc.movzx(r[data.rd], temp.r8());
}

void JitX64::MOVN(InstructionData& data) {
	if (data.rd == 0) return;
	Label exit_movn = cc.new_label();

	cc.cmp(r[data.rt], 0);
	cc.j(x86::CondCode::kEqual, exit_movn);

	cc.mov(r[data.rd], r[data.rs]);
	cc.bind(exit_movn);
}

void JitX64::SLTIU(InstructionData& data) {
	if (data.rt == 0) return;
	cc.cmp(r[data.rs], (u64)(i16)data.imm);
	cc.set(x86::CondCode::kUnsignedLT, temp.r8());
	cc.movzx(r[data.rt], temp.r8());
}

void JitX64::LB(InstructionData& data) {
	if (data.rt == 0) return;
	cc.mov(temp.r32(), r[data.rs].r32());
	if (data.imm) cc.add(temp.r32(), (i32)(i16)data.imm);
	EmitReadVirtualMemory<u8>(r[data.rt].r8(), temp.r32());
	cc.movsx(r[data.rt].r32(), r[data.rt].r8());
	cc.movsxd(r[data.rt].r64(), r[data.rt].r32());
}

void JitX64::SWC1(InstructionData& data) {
	x86::Gp address = cc.new_gp32();
	cc.mov(address, r[data.rs].r32());
	if (data.imm) cc.add(address, (i32)(i16)data.imm);
	cc.mov(temp.r32(), x86::dword_ptr(r5900, offsetof(R5900, fpr) + (sizeof(float) * data.rt)));
	EmitWriteVirtualMemory<u32>(address, temp.r32());
}

void JitX64::J(InstructionData& data) {
	EmitJump(((data.pc + 4) & 0xf0000000) | (data.addr << 2));
	EmitBranchDelay(data);
}

void JitX64::SB(InstructionData& data) {
	cc.mov(temp.r32(), r[data.rs].r32());
	if (data.imm) cc.add(temp.r32(), (i32)(i16)data.imm);
	EmitWriteVirtualMemory<u8>(temp.r32(), r[data.rt].r8());
}

void JitX64::MFHI(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(r[data.rd], x86::qword_ptr(r5900, (data.pipeline1) ? offsetof(R5900, hi1) : offsetof(R5900, hi)));
}

void JitX64::SLTU(InstructionData& data) {
	if (data.rd == 0) return;
	cc.cmp(r[data.rs], r[data.rt]);
	cc.set(x86::CondCode::kUnsignedLT, temp.r8());
	cc.movzx(r[data.rd], temp.r8());
}

void JitX64::BLEZ(InstructionData& data) {
	Label exit_blez = cc.new_label();
	Label end = cc.new_label();
	cc.cmp(r[data.rs], 0);
	cc.j(x86::CondCode::kSignedGT, exit_blez);

	EmitJump((data.pc + 4) + (i32)((i16)data.imm << 2));
	EmitBranchDelay(data);
	cc.jmp(end);

	cc.bind(exit_blez);
	if (!data.likely) {
		EmitBranchDelay(data);
	}

	cc.bind(end);	
}

void JitX64::SUBU(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(r[data.rd].r32(), r[data.rs].r32());
	cc.sub(r[data.rd].r32(), r[data.rt].r32());
	cc.movsxd(r[data.rd], r[data.rd].r32());
}

void JitX64::BGTZ(InstructionData& data) {
	Label exit_bgtz = cc.new_label();
	Label end = cc.new_label();
	cc.cmp(r[data.rs], 0);
	cc.j(x86::CondCode::kSignedLE, exit_bgtz);

	EmitJump((data.pc + 4) + (i32)((i16)data.imm << 2));
	EmitBranchDelay(data);
	cc.jmp(end);

	cc.bind(exit_bgtz);
	if (!data.likely) {
		EmitBranchDelay(data);
	}

	cc.bind(end);	
}

void JitX64::AND(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(r[data.rd], r[data.rs]);
	cc.and_(r[data.rd], r[data.rt]);
}

void JitX64::SRL(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(r[data.rd].r32(), r[data.rt].r32());
	cc.shr(r[data.rd].r32(), data.sa);
	cc.movsxd(r[data.rd], r[data.rd].r32());
}

void JitX64::DSRL32(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(r[data.rd], r[data.rt]);
	cc.shr(r[data.rd], data.sa + 32);
}

void JitX64::LHU(InstructionData& data) {
	if (data.rt == 0) return;
	cc.mov(temp.r32(), r[data.rs].r32());
	if (data.imm) cc.add(temp.r32(), (i32)(i16)data.imm);
	EmitReadVirtualMemory<u16>(r[data.rt].r16(), temp.r32());
	cc.movzx(r[data.rt].r64(), r[data.rt].r16());
}

void JitX64::BLTZ(InstructionData& data) {
	Label exit_bltz = cc.new_label();
	Label end = cc.new_label();
	cc.cmp(r[data.rs], 0);
	cc.j(x86::CondCode::kSignedGE, exit_bltz);

	EmitJump((data.pc + 4) + (i32)((i16)data.imm << 2));
	EmitBranchDelay(data);
	cc.jmp(end);

	cc.bind(exit_bltz);
	if (!data.likely) {
		EmitBranchDelay(data);
	}

	cc.bind(end);	
}

void JitX64::SH(InstructionData& data) {
	cc.mov(temp.r32(), r[data.rs].r32());
	if (data.imm) cc.add(temp.r32(), (i32)(i16)data.imm);
	EmitWriteVirtualMemory<u16>(temp.r32(), r[data.rt].r16());
}

void JitX64::DSRAV(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(temp, r[data.rs]);
	cc.and_(temp, 0b111111);
	cc.mov(r[data.rd], r[data.rt]);
	cc.sar(r[data.rd], temp);
}

void JitX64::XORI(InstructionData& data) {
	if (data.rt == 0) return;
	cc.mov(r[data.rt], r[data.rs]);
	cc.xor_(r[data.rt], data.imm);
}

void JitX64::LWU(InstructionData& data) {
	if (data.rt == 0) return;
	cc.mov(temp.r32(), r[data.rs].r32());
	if (data.imm) cc.add(temp.r32(), (i32)(i16)data.imm);
	EmitReadVirtualMemory<u32>(r[data.rt].r32(), temp.r32());
}

void JitX64::MOVZ(InstructionData& data) {
	if (data.rd == 0) return;
	Label exit_movz = cc.new_label();

	cc.cmp(r[data.rt], 0);
	cc.j(x86::CondCode::kNotEqual, exit_movz);

	cc.mov(r[data.rd], r[data.rs]);
	cc.bind(exit_movz);
}

void JitX64::DSLLV(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(temp, r[data.rs]);
	cc.and_(temp, 0b111111);
	cc.mov(r[data.rd], r[data.rt]);
	cc.shl(r[data.rd], temp);
}

void JitX64::DADDIU(InstructionData& data) {
	if (data.rt == 0) return;
	cc.mov(r[data.rt], r[data.rs]);
	cc.add(r[data.rt], (i32)(i16)data.imm);
}

void JitX64::LQ(InstructionData& data) {
	if (data.rt == 0) return;
	cc.mov(temp.r32(), r[data.rs].r32());
	if (data.imm) cc.add(temp.r32(), (i32)(i16)data.imm);

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
	if (data.rt == 0) return;
	cc.mov(temp.r32(), r[data.rs].r32());
	if (data.imm) cc.add(temp.r32(), (i32)(i16)data.imm);
	
	InvokeNode* node;
	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_sq), FuncSignature::build<void, R5900*, Memory*, u32, u8>());
	node->set_arg(0, m_R5900);
	node->set_arg(1, m_Memory);
	node->set_arg(2, temp.r32());
	node->set_arg(3, data.rt);
}