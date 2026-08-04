#include "jit_x64.hpp"
#include "../../config.hpp"

using namespace EmotionEngine::Core;
using namespace asmjit;

bool JitX64::InitJit(R5900* cpu, Memory* memory) {
	if (!JitBackend::InitJit(cpu, memory)) return false;
	if (m_CodeHolder.attach(&cc) != Error::kOk) return false;

#ifdef ENABLE_ASMJIT_LOGGING
	m_Logger.set_flags(FormatFlags::kHexImms | FormatFlags::kHexOffsets | FormatFlags::kRegCasts);
	cc.add_diagnostic_options(DiagnosticOptions::kRAAnnotate | DiagnosticOptions::kRADebugAll);
#endif
	return true;
}

void JitX64::EmitBeginBlock() {
	cc.add_func(FuncSignature::build<void>());

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
		cc.vpxor(reg, reg, reg);
		return;
	}

	cc.vmovdqu(reg, asmjit::x86::oword_ptr(r5900, index * sizeof(GPR)));
}

void JitX64::EmitStoreRegister128(u8 index, asmjit::x86::Vec& reg) {
	if (index == 0) return;
	cc.vmovdqu(asmjit::x86::oword_ptr(r5900, index * sizeof(GPR)), reg);
}

void JitX64::EmitReadVirtualMemory128(asmjit::x86::Vec& ret, const asmjit::x86::Gp& address) {
	// only allow 128-bit reads from main memory
	cc.cmp(address, RDRAM_LAST_ADDR);

	Label outside_main_memory = cc.new_label();
	Label end = cc.new_label();

	cc.j(x86::CondCode::kA, outside_main_memory);

	// t1 <- &(main_memory[address])
	cc.movabs(t1, m_Memory->rdram);
	cc.add(t1, address);

	// ret <- *t1
	cc.vmovdqu(ret, x86::oword_ptr(t1));
	cc.jmp(end);

	cc.bind(outside_main_memory); {
		cc.int3();
		cc.nop();
	}

	cc.bind(end);
}

void JitX64::EmitWriteVirtualMemory128(const asmjit::x86::Gp& address, asmjit::x86::Vec& val) {
	// only allow 128-bit writes to main memory
	cc.cmp(address, RDRAM_LAST_ADDR);

	Label outside_main_memory = cc.new_label();
	Label end = cc.new_label();

	cc.j(x86::CondCode::kA, outside_main_memory);

	// t1 <- &(main_memory[address])
	cc.movabs(t1, m_Memory->rdram);
	cc.add(t1, address);

	// ret <- *t1
	cc.vmovdqu(x86::oword_ptr(t1), val);
	cc.jmp(end);

	cc.bind(outside_main_memory); {
		cc.int3();
		cc.nop();
	}

	cc.bind(end);
}

void JitX64::EmitStoreSpecialRegister(SpecialRegName dst, const asmjit::x86::Gp& src) {
	asmjit::x86::Mem ptr;
	switch (dst) {
		case SpecialRegName::HI: { ptr = x86::qword_ptr(r5900, offsetof(R5900, hi)); break; };
		case SpecialRegName::LO: { ptr = x86::qword_ptr(r5900, offsetof(R5900, lo)); break; };
		case SpecialRegName::HI1: { ptr = x86::qword_ptr(r5900, offsetof(R5900, hi1)); break; };
		case SpecialRegName::LO1: { ptr = x86::qword_ptr(r5900, offsetof(R5900, lo1)); break; };
	}

	if (src.is_gp32()) {
		cc.movsxd(t1, src);
		cc.mov(ptr, t1);
	} else {
		cc.mov(ptr, src);
	}
}

void JitX64::EmitLoadFPR(const asmjit::x86::Vec& ret, u8 index) {
	cc.vmovss(ret, asmjit::x86::dword_ptr(r5900, offsetof(R5900, fpr) + (index * sizeof(float))));
}

void JitX64::EmitStoreFPR(u8 index, const asmjit::x86::Vec& src) {
	cc.vmovss(asmjit::x86::dword_ptr(r5900, offsetof(R5900, fpr) + (index * sizeof(float))), src);
}


void JitX64::EmitLoadFPR(const asmjit::x86::Gp& ret, u8 index) {
	cc.mov(ret, asmjit::x86::dword_ptr(r5900, offsetof(R5900, fpr) + (index * sizeof(float))));
}

void JitX64::EmitStoreFPR(u8 index, const asmjit::x86::Gp& src) {
	cc.mov(asmjit::x86::dword_ptr(r5900, offsetof(R5900, fpr) + (index * sizeof(float))), src);
}