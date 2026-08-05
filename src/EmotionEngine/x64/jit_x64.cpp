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

	r5900 = cc.new_gp64("r5900");
	cc.movabs(r5900, reinterpret_cast<uintptr_t>(m_R5900));
	for (u8 i = 0; i < 32; i++) {
		if (!m_UsedRegisters[i]) continue;

		r[i] = cc.new_gp64(g_RegNames[i]);
		if (i == 0) {
			cc.xor_(r[i], r[i]);
			continue;
		}

		cc.mov(r[i], x86::qword_ptr(r5900, offsetof(R5900, gpr) + (i * sizeof(GPR))));
	}
}

void JitX64::EmitEndBlock() {
	FlushRegisters();
	cc.end_func();
	cc.finalize();
}

void JitX64::FlushRegisters() {
	for (u8 i = 1; i < 32; i++) {
		if (!m_UsedRegisters[i]) continue;
		m_UsedRegisters[i] = false;
		cc.mov(x86::qword_ptr(r5900, offsetof(R5900, gpr) + (i * sizeof(GPR))), r[i]);
	}
}