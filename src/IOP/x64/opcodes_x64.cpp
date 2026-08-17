#include "jit_x64.hpp"
using namespace IOProcessor;
using namespace asmjit;

static u32 WRAP_ReadCOP0(R3000A* r3000a, u8 reg) { return r3000a->ReadCOP0(reg); }
static void WRAP_WriteCOP0(R3000A* r3000a, u8 reg, u32 val) { r3000a->WriteCOP0(reg, val); }

static void IMPL_div(R3000A* r3000a, i32 numerator, i32 denominator) {
	// division by zero case
	if (denominator == 0) {
		r3000a->hi = numerator;
		r3000a->lo = (numerator >= 0) ? 0xffffffff : 1;
		return;
	}

	// 0x80000000 / -1 case
	if ((u32)numerator == 0x80000000 && denominator == -1) {
		r3000a->hi = 0;
		r3000a->lo = 0x80000000;
		return;
	}

	r3000a->hi = static_cast<u32>(numerator % denominator);
	r3000a->lo = static_cast<u32>(numerator / denominator);
}

static void IMPL_divu(R3000A* r3000a, u32 numerator, u32 denominator) {
	// division by zero case
	if (denominator == 0) {
		r3000a->hi = numerator;
		r3000a->lo = 0xffffffff;
		return;
	}

	r3000a->hi = numerator % denominator;
	r3000a->lo = numerator / denominator;
}

static void IMPL_mult(R3000A* r3000a, i32 rs, i32 rt) {
	u64 result = static_cast<u64>(rs) * static_cast<u64>(rt);
	r3000a->hi = (result >> 32) & 0xffffffff;
	r3000a->lo = result & 0xffffffff;
}

static void IMPL_multu(R3000A* r3000a, u32 rs, u32 rt) {
	u64 result = static_cast<u64>(rs) * static_cast<u64>(rt);
	r3000a->hi = (result >> 32) & 0xffffffff;
	r3000a->lo = result & 0xffffffff;
}

static void IMPL_rfe(R3000A* r3000a) {
	u32 mode = r3000a->cop0[SR] & 0x3f;
	r3000a->cop0[SR] &= 0xfffffff0;
	r3000a->cop0[SR] |= (mode >> 2); // shift in previous bits
}

static u32 IMPL_lwl(Memory* memory, u32 address, u32 rt) {
	u32 aligned = address & ~(u32)3;
	u32 word = memory->ReadVirtualMemory32(aligned);

	switch (address & 3) {
		case 0: return (rt & 0x00ffffff) | (word << 24);
		case 1: return (rt & 0x0000ffff) | (word << 16);
		case 2: return (rt & 0x000000ff) | (word << 8);
		case 3: return word;
	}

	std::unreachable();
}

static u32 IMPL_lwr(Memory* memory, u32 address, u32 rt) {
	u32 aligned = address & ~(u32)3;
	u32 word = memory->ReadVirtualMemory32(aligned);

	switch (address & 3) {
		case 0: return word;
		case 1: return (rt & 0xff000000) | (word >> 8);
		case 2: return (rt & 0xffff0000) | (word >> 16);
		case 3: return (rt & 0xffffff00) | (word >> 24);
	}

	std::unreachable();
}

static void IMPL_swl(Memory* memory, u32 address, u32 rt) {
	u32 aligned = address & ~(u32)3;
	u32 word = memory->ReadVirtualMemory32(aligned);

	u32 mem;
	switch (address & 3) {
		case 0: { mem = (word & 0xffffff00) | (rt >> 24); break; }
		case 1: { mem = (word & 0xffff0000) | (rt >> 16); break; }
		case 2: { mem = (word & 0xff000000) | (rt >> 8); break; }
		case 3: { mem = rt; break; }
		default: std::unreachable();
	}

	memory->WriteVirtualMemory32(aligned, mem);
}

static void IMPL_swr(Memory* memory, u32 address, u32 rt) {
	u32 aligned = address & ~(u32)3;
	u32 word = memory->ReadVirtualMemory32(aligned);

	u32 mem;
	switch (address & 3) {
		case 0: { mem = rt; break; }
		case 1: { mem = (word & 0x000000ff) | (rt << 8); break; }
		case 2: { mem = (word & 0x0000ffff) | (rt << 16); break; }
		case 3: { mem = (word & 0x00ffffff) | (rt << 24); break; }
		default: std::unreachable();
	}

	memory->WriteVirtualMemory32(aligned, mem);
}

void JitX64::MoveImm(InstructionData& data) {
	cc.mov(r[data.rt], data.addr);
}

void JitX64::Illegal(InstructionData& data) {
	EmitException(data, ExceptionCause::IllegalInstruction);
}

void JitX64::LUI(InstructionData& data) {
	if (data.rt == 0) return;
	cc.mov(r[data.rt], (data.imm << 16));
}

void JitX64::ORI(InstructionData& data) {
	if (data.rt == 0) return;
	cc.mov(r[data.rt], r[data.rs]);
	cc.or_(r[data.rt], data.imm);
}

void JitX64::SW(InstructionData& data) {
	cc.lea(temp, x86::ptr(r[data.rs], (i32)(i16)data.imm));
	EmitWriteVirtualMemory<u32>(data, temp, data.rt);
}

void JitX64::SLL(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(r[data.rd], r[data.rt]);
	cc.shl(r[data.rd], data.sa);
}

void JitX64::ADDIU(InstructionData& data) {
	if (data.rt == 0) return;
	cc.lea(r[data.rt], x86::ptr(r[data.rs], (i32)(i16)data.imm));
}

void JitX64::J(InstructionData& data) {
	EmitJump(((data.pc + 4) & 0xf0000000) | (data.addr << 2));
	EmitBranchDelay(data);
}

void JitX64::LHU(InstructionData& data) {
	cc.lea(temp, x86::ptr(r[data.rs], (i32)(i16)data.imm));
	EmitReadVirtualMemory<u16>(data, data.rt, temp, false);
}

void JitX64::OR(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(temp, r[data.rs]);
	cc.or_(temp, r[data.rt]);
	cc.mov(r[data.rd], temp);
}

void JitX64::MTC0(InstructionData& data) {
	// no need to flush and load registers here
	InvokeNode* node;

	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(&WRAP_WriteCOP0), FuncSignature::build<void, R3000A*, u8, u32>());
	node->set_arg(0, r3000a);
	node->set_arg(1, data.rd);
	node->set_arg(2, r[data.rt]);
}

void JitX64::BNE(InstructionData& data) {
	Label end = cc.new_label();
	cc.cmp(r[data.rs], r[data.rt]);
	cc.j(x86::CondCode::kEqual, end);

	EmitJump((data.pc + 4) + (i32)((i16)data.imm << 2));

	cc.bind(end);
	EmitBranchDelay(data);
}

void JitX64::ADDI(InstructionData& data) {
	cc.mov(temp, r[data.rs]);
	cc.add(temp, (i32)(i16)data.imm);
	CheckOverflow(data, (data.rt == 0) ? temp : r[data.rt], temp);
}

void JitX64::LW(InstructionData& data) {
	cc.lea(temp, x86::ptr(r[data.rs], (i32)(i16)data.imm));
	EmitReadVirtualMemory<u32>(data, data.rt, temp, false);
}

void JitX64::SLTU(InstructionData& data) {
	if (data.rd == 0) return;
	cc.cmp(r[data.rs], r[data.rt]);
	cc.set(x86::CondCode::kUnsignedLT, r[data.rd].r8());
	cc.movzx(r[data.rd], r[data.rd].r8());
}

void JitX64::ADDU(InstructionData& data) {
	if (data.rd == 0) return;
	cc.lea(r[data.rd], x86::ptr(r[data.rs], r[data.rt]));
}

void JitX64::SH(InstructionData& data) {
	cc.lea(temp, x86::ptr(r[data.rs], (i32)(i16)data.imm));
	EmitWriteVirtualMemory<u16>(data, temp, data.rt);
}

void JitX64::JAL(InstructionData& data) {
	EmitJump(((data.pc + 4) & 0xf0000000) | (data.addr << 2));
	EmitBranchDelay(data);
	cc.mov(r[31], data.pc + 8);
}

void JitX64::ANDI(InstructionData& data) {
	if (data.rt == 0) return;
	cc.mov(r[data.rt], r[data.rs]);
	cc.and_(r[data.rt], data.imm);
}

void JitX64::SB(InstructionData& data) {
	cc.lea(temp, x86::ptr(r[data.rs], (i32)(i16)data.imm));
	EmitWriteVirtualMemory<u8>(data, temp, data.rt);
}

void JitX64::JR(InstructionData& data) {
	EmitJump(r[data.rs]);
	EmitBranchDelay(data);
}

void JitX64::LB(InstructionData& data) {
	cc.lea(temp, x86::ptr(r[data.rs], (i32)(i16)data.imm));
	EmitReadVirtualMemory<u8>(data, data.rt, temp, true);
}

void JitX64::SLLV(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(temp, r[data.rs]);
	cc.and_(temp, 0b11111);
	cc.mov(r[data.rd], r[data.rt]);
	cc.shl(r[data.rd], temp);
}

void JitX64::BEQ(InstructionData& data) {
	Label end = cc.new_label();
	cc.cmp(r[data.rs], r[data.rt]);
	cc.j(x86::CondCode::kNotEqual, end);

	EmitJump((data.pc + 4) + (i32)((i16)data.imm << 2));

	cc.bind(end);
	EmitBranchDelay(data);
}

void JitX64::MFC0(InstructionData& data) {
	// no need to flush and load registers here
	InvokeNode* node;

	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(&WRAP_ReadCOP0), FuncSignature::build<u32, R3000A*, u8>());
	node->set_arg(0, r3000a);
	node->set_arg(1, data.rd);
	node->set_ret(0, r[data.rt]);
}

void JitX64::AND(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(temp, r[data.rs]);
	cc.and_(temp, r[data.rt]);
	cc.mov(r[data.rd], temp);
}

void JitX64::ADD(InstructionData& data) {
	cc.mov(temp, r[data.rs]);
	cc.add(temp, r[data.rt]);
	CheckOverflow(data, (data.rd == 0) ? temp : r[data.rd], temp);
}

void JitX64::BGTZ(InstructionData& data) {
	Label end = cc.new_label();
	cc.cmp(r[data.rs], 0);
	cc.j(x86::CondCode::kSignedLE, end);

	EmitJump((data.pc + 4) + (i32)((i16)data.imm << 2));

	cc.bind(end);
	EmitBranchDelay(data);
}

void JitX64::BLEZ(InstructionData& data) {
	Label end = cc.new_label();
	cc.cmp(r[data.rs], 0);
	cc.j(x86::CondCode::kSignedGT, end);

	EmitJump((data.pc + 4) + (i32)((i16)data.imm << 2));

	cc.bind(end);
	EmitBranchDelay(data);
}

void JitX64::LBU(InstructionData& data) {
	cc.lea(temp, x86::ptr(r[data.rs], (i32)(i16)data.imm));
	EmitReadVirtualMemory<u8>(data, data.rt, temp, false);
}

void JitX64::JALR(InstructionData& data) {
	EmitJump(r[data.rs]);
	EmitBranchDelay(data);
	if (data.rd) cc.mov(r[data.rd], data.pc + 8);
}

void JitX64::BLTZ(InstructionData& data) {
	Label end = cc.new_label();
	cc.cmp(r[data.rs], 0);
	cc.j(x86::CondCode::kSignedGE, end);

	EmitJump((data.pc + 4) + (i32)((i16)data.imm << 2));

	cc.bind(end);
	EmitBranchDelay(data);
}

void JitX64::SLTI(InstructionData& data) {
	if (data.rt == 0) return;
	cc.cmp(r[data.rs], (i32)(i16)data.imm);
	cc.set(x86::CondCode::kSignedLT, r[data.rt].r8());
	cc.movzx(r[data.rt], r[data.rt].r8());
}

void JitX64::SUBU(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(temp, r[data.rs]);
	cc.sub(temp, r[data.rt]);
	cc.mov(r[data.rd], temp);
}

void JitX64::SRA(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(r[data.rd], r[data.rt]);
	cc.sar(r[data.rd], data.sa);
}

void JitX64::DIV(InstructionData& data) {
	// no need to flush and load registers here
	InvokeNode* node;

	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_div), FuncSignature::build<void, R3000A*, i32, i32>());
	node->set_arg(0, m_R3000A);
	node->set_arg(1, r[data.rs]);
	node->set_arg(2, r[data.rt]);
}

void JitX64::MFLO(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(r[data.rd], x86::dword_ptr(r3000a, offsetof(R3000A, lo)));
}

void JitX64::BGEZ(InstructionData& data) {
	Label end = cc.new_label();
	cc.cmp(r[data.rs], 0);
	cc.j(x86::CondCode::kSignedLT, end);

	EmitJump((data.pc + 4) + (i32)((i16)data.imm << 2));

	cc.bind(end);
	EmitBranchDelay(data);
}

void JitX64::SRL(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(r[data.rd], r[data.rt]);
	cc.shr(r[data.rd], data.sa);
}

void JitX64::SLTIU(InstructionData& data) {
	if (data.rt == 0) return;
	cc.cmp(r[data.rs], (u64)(i16)data.imm);
	cc.set(x86::CondCode::kUnsignedLT, r[data.rt].r8());
	cc.movzx(r[data.rt], r[data.rt].r8());
}

void JitX64::DIVU(InstructionData& data) {
	// no need to flush and load registers here
	InvokeNode* node;

	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_divu), FuncSignature::build<void, R3000A*, u32, u32>());
	node->set_arg(0, m_R3000A);
	node->set_arg(1, r[data.rs]);
	node->set_arg(2, r[data.rt]);
}

void JitX64::MFHI(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(r[data.rd], x86::dword_ptr(r3000a, offsetof(R3000A, hi)));
}

void JitX64::SLT(InstructionData& data) {
	if (data.rd == 0) return;
	cc.cmp(r[data.rs], r[data.rt]);
	cc.set(x86::CondCode::kSignedLT, r[data.rd].r8());
	cc.movzx(r[data.rd], r[data.rd].r8());
}

void JitX64::SYSCALL(InstructionData& data) {
	EmitException(data, ExceptionCause::Syscall);
}

void JitX64::MTLO(InstructionData& data) {
	cc.mov(x86::dword_ptr(r3000a, offsetof(R3000A, lo)), r[data.rs]);
}

void JitX64::MTHI(InstructionData& data) {
	cc.mov(x86::dword_ptr(r3000a, offsetof(R3000A, hi)), r[data.rs]);
}

void JitX64::RFE(InstructionData& data) {
	if ((data.imm & 0x3f) != 0b010000) {
		EmitException(data, ExceptionCause::IllegalInstruction);
		return;
	}

	// no need to flush and load registers here
	InvokeNode* node;

	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_rfe), FuncSignature::build<void, R3000A*>());
	node->set_arg(0, m_R3000A);
}

void JitX64::LH(InstructionData& data) {
	cc.lea(temp, x86::ptr(r[data.rs], (i32)(i16)data.imm));
	EmitReadVirtualMemory<u16>(data, data.rt, temp, true);
}

void JitX64::NOR(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(temp, r[data.rs]);
	cc.or_(temp, r[data.rt]);
	cc.not_(temp);
	cc.mov(r[data.rd], temp);
}

void JitX64::SRAV(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(temp, r[data.rs]);
	cc.and_(temp, 0b11111);
	cc.mov(r[data.rd], r[data.rt]);
	cc.sar(r[data.rd], temp);
}

void JitX64::SRLV(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(temp, r[data.rs]);
	cc.and_(temp, 0b11111);
	cc.mov(r[data.rd], r[data.rt]);
	cc.shr(r[data.rd], temp);
}

void JitX64::MULTU(InstructionData& data) {
	// no need to flush and load registers here
	InvokeNode* node;

	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_multu), FuncSignature::build<void, R3000A*, u32, u32>());
	node->set_arg(0, m_R3000A);
	node->set_arg(1, r[data.rs]);
	node->set_arg(2, r[data.rt]);
}

void JitX64::XOR(InstructionData& data) {
	if (data.rd == 0) return;
	cc.mov(temp, r[data.rs]);
	cc.xor_(temp, r[data.rt]);
	cc.mov(r[data.rd], temp);
}

void JitX64::XORI(InstructionData& data) {
	if (data.rt == 0) return;
	cc.mov(r[data.rt], r[data.rs]);
	cc.xor_(r[data.rt], data.imm);
}

void JitX64::MULT(InstructionData& data) {
	// no need to flush and load registers here
	InvokeNode* node;

	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_mult), FuncSignature::build<void, R3000A*, i32, i32>());
	node->set_arg(0, m_R3000A);
	node->set_arg(1, r[data.rs]);
	node->set_arg(2, r[data.rt]);
}

void JitX64::SUB(InstructionData& data) {
	cc.mov(temp, r[data.rs]);
	cc.sub(temp, r[data.rt]);
	CheckOverflow(data, (data.rd == 0) ? temp : r[data.rd], temp);
}

void JitX64::LWL(InstructionData& data) {
	cc.lea(temp, x86::ptr(r[data.rs], (i32)(i16)data.imm));

	// no need to flush and load registers here
	InvokeNode* node;

	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_lwl), FuncSignature::build<u32, Memory*, u32, u32>());
	node->set_arg(0, m_Memory);
	node->set_arg(1, temp);
	node->set_arg(2, r[data.rt]);
	node->set_ret(0, temp);
	if (data.rt) cc.mov(r[data.rt], temp);
}

void JitX64::LWR(InstructionData& data) {
	cc.lea(temp, x86::ptr(r[data.rs], (i32)(i16)data.imm));

	// no need to flush and load registers here
	InvokeNode* node;

	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_lwr), FuncSignature::build<u32, Memory*, u32, u32>());
	node->set_arg(0, m_Memory);
	node->set_arg(1, temp);
	node->set_arg(2, r[data.rt]);
	node->set_ret(0, temp);
	if (data.rt) cc.mov(r[data.rt], temp);
}

void JitX64::SWL(InstructionData& data) {
	cc.lea(temp, x86::ptr(r[data.rs], (i32)(i16)data.imm));

	// no need to flush and load registers here
	InvokeNode* node;

	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_swl), FuncSignature::build<void, Memory*, u32, u32>());
	node->set_arg(0, m_Memory);
	node->set_arg(1, temp);
	node->set_arg(2, r[data.rt]);
}

void JitX64::SWR(InstructionData& data) {
	cc.lea(temp, x86::ptr(r[data.rs], (i32)(i16)data.imm));

	// no need to flush and load registers here
	InvokeNode* node;

	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(IMPL_swr), FuncSignature::build<void, Memory*, u32, u32>());
	node->set_arg(0, m_Memory);
	node->set_arg(1, temp);
	node->set_arg(2, r[data.rt]);
}

void JitX64::BLTZAL(InstructionData& data) {
	Label end = cc.new_label();
	cc.cmp(r[data.rs], 0);
	cc.j(x86::CondCode::kSignedGE, end);

	EmitJump((data.pc + 4) + (i32)((i16)data.imm << 2));

	cc.bind(end);
	EmitBranchDelay(data);
	cc.mov(r[31], data.pc + 8);
}

void JitX64::BGEZAL(InstructionData& data) {
	Label end = cc.new_label();
	cc.cmp(r[data.rs], 0);
	cc.j(x86::CondCode::kSignedLT, end);

	EmitJump((data.pc + 4) + (i32)((i16)data.imm << 2));

	cc.bind(end);
	EmitBranchDelay(data);
	cc.mov(r[31], data.pc + 8);
}

void JitX64::BREAK(InstructionData& data) {
	EmitException(data, ExceptionCause::Break);
}