#include "jit_x64.hpp"
#include "../../config.hpp"

using namespace EmotionEngine::MIPS;
using namespace asmjit;

bool JitX64::InitJit(R5900* cpu) {
	if (!JitBackend::InitJit(cpu)) return false;
	if (m_CodeHolder.attach(&cc) != Error::kOk) return false;

#ifdef ENABLE_ASMJIT_LOGGING
	m_Logger.set_flags(FormatFlags::kHexImms | FormatFlags::kHexOffsets | FormatFlags::kRegCasts);
	cc.add_diagnostic_options(DiagnosticOptions::kRAAnnotate | DiagnosticOptions::kRADebugAll);
#endif
	return true;
}

void JitX64::EmitBeginBlock() {
	cc.add_func(FuncSignature::build<void, R5900*>());

	// r5900 = pointer to R5900 struct
	r5900 = cc.new_gp_ptr("r5900");
	cc.mov(r5900, m_R5900);

	// make a few scratch registers so we dont have to keep allocating
	s1 = cc.new_gp64("s1");
	s2 = cc.new_gp64("s2");
	s3 = cc.new_gp64("s3");
	s4 = cc.new_gp64("s4");

	// this can be freely overwritten by Emit*() functions
	t1 = cc.new_gp64("t1");

	// make a few vector registers so we dont have to keep allocating
	v1 = cc.new_vec128("v1");
	v2 = cc.new_vec128("v2");
	v3 = cc.new_vec128("v3");
	v4 = cc.new_vec128("v4");
}

void JitX64::EmitEndBlock() {
	cc.end_func();
	cc.finalize();
}

InvokeNode* JitX64::EmitExternalCall(uintptr_t address, const FuncSignature& sig) {
	InvokeNode* invoke_node;
	cc.invoke(Out(invoke_node), address, sig);
	return invoke_node;
}

void JitX64::EmitLoadRegister128(asmjit::x86::Vec& reg, u8 index) {
	if (index == 0) {
		cc.pxor(reg, reg);
		return;
	}

	cc.movdqu(reg, asmjit::x86::oword_ptr(r5900, index * sizeof(GPR)));
}

void JitX64::EmitStoreRegister128(u8 index, asmjit::x86::Vec& reg) {
	if (index == 0) return;
	cc.movdqu(asmjit::x86::oword_ptr(r5900, index * sizeof(GPR)), reg);
}

void JitX64::EmitReadVirtualMemory128(asmjit::x86::Vec& ret, asmjit::x86::Gp& address) {
	// virtual -> physical
	cc.and_(address, 0x1fffffff);

	// only allow 128-bit reads from main memory
#ifdef ENABLE_DEBUG
	cc.cmp(address, 0x1fffffff);

	Label inside_main_memory = cc.new_label();
	cc.j(x86::CondCode::kBE, inside_main_memory);
	cc.int3();

	cc.bind(inside_main_memory);
#endif

	// t1 <- &(main_memory[address])
	cc.mov(t1, Memory::m_Memory);
	cc.add(t1, address);

	// ret <- *t1
	cc.movdqu(ret, x86::oword_ptr(t1));
}

void JitX64::EmitWriteVirtualMemory128(asmjit::x86::Gp& address, asmjit::x86::Vec& val) {
	// virtual -> physical
	cc.and_(address, 0x1fffffff);

	// only allow 128-bit writes to main memory
#ifdef ENABLE_DEBUG
	cc.cmp(address, 0x1fffffff);

	Label inside_main_memory = cc.new_label();
	cc.j(x86::CondCode::kBE, inside_main_memory);
	cc.int3();

	cc.bind(inside_main_memory);
#endif

	// t1 <- &(main_memory[address])
	cc.mov(t1, Memory::m_Memory);
	cc.add(t1, address);

	// ret <- *t1
	cc.movdqu(x86::oword_ptr(t1), val);
}
