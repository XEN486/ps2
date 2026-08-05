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
	for (u8 i = 0; i < 32; i++) {
		if (!m_UsedRegisters[i]) continue;
		r[i] = cc.new_gp64(g_RegNames[i]);

		uintptr_t gpr_base = reinterpret_cast<uintptr_t>(m_R5900) + offsetof(R5900, gpr);
		cc.mov(r[i], x86::qword_ptr(gpr_base + (i * sizeof(GPR))));
	}
}

void JitX64::EmitEndBlock() {
	for (u8 i = 0; i < 32; i++) {
		if (!m_UsedRegisters[i]) continue;
		uintptr_t gpr_base = reinterpret_cast<uintptr_t>(m_R5900) + offsetof(R5900, gpr);
		cc.mov(x86::qword_ptr(gpr_base + (i * sizeof(GPR))), r[i]);
	}

	cc.end_func();
	cc.finalize();
}