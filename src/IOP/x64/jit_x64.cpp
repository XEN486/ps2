#include "jit_x64.hpp"
#include "../../config.hpp"

using namespace IOProcessor;
using namespace asmjit;

static void WRAP_Exception(R3000A* r3000a, ExceptionCause cause, u32 epc, bool in_delay_slot) { r3000a->Exception(cause, epc, in_delay_slot); }

bool JitX64::InitJit(R3000A* cpu, Memory* memory) {
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

	r3000a = cc.new_gp_ptr("r3000a");
	temp = cc.new_gp32("temp");
	
	cc.movabs(r3000a, reinterpret_cast<uintptr_t>(m_R3000A));
	for (u8 i = 0; i < 32; i++) {
		if (!m_UsedRegisters[i]) continue;

		r[i] = cc.new_gp32(g_RegNames[i]);
		if (i == 0) {
			cc.xor_(r[i], r[i]);
			continue;
		}

		cc.mov(r[i], x86::dword_ptr(r3000a, offsetof(R3000A, gpr) + (i * sizeof(u32))));
	}
}

void JitX64::EmitEndBlock() {
	EmitEpilogue();
	cc.end_func();
	cc.finalize();
}

void JitX64::EmitFlushRegisters() {
	for (u8 i = 1; i < 32; i++) {
		if (!m_UsedRegisters[i]) continue;
		//m_UsedRegisters[i] = false;
		cc.mov(x86::dword_ptr(r3000a, offsetof(R3000A, gpr) + (i * sizeof(u32))), r[i]);
	}
}

void JitX64::EmitLoadRegisters() {
	for (u8 i = 0; i < 32; i++) {
		if (!m_UsedRegisters[i]) continue;
		if (i == 0) {
			cc.xor_(r[i], r[i]);
			continue;
		}

		cc.mov(r[i], x86::dword_ptr(r3000a, offsetof(R3000A, gpr) + (i * sizeof(u32))));
	}
}

void JitX64::EmitException(InstructionData& data, ExceptionCause cause) {
	// no need to flush and load registers here
	InvokeNode* node;

	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(WRAP_Exception), FuncSignature::build<void, R3000A*, ExceptionCause, u32, bool>());
	node->set_arg(0, r3000a);
	node->set_arg(1, cause);
	node->set_arg(2, data.pc);
	node->set_arg(3, data.in_branch_delay);
}

void JitX64::CheckOverflow(InstructionData& data, asmjit::x86::Gp& result, asmjit::x86::Gp& value) {
	assert(result.size() == value.size());

	Label overflow = cc.new_label();
	Label end = cc.new_label();

	cc.j(x86::CondCode::kOverflow, overflow);

	// not overflow case, move value to result
	cc.mov(result, value);
	cc.jmp(end);

	// overflow case, do the exception
	cc.bind(overflow);

	EmitException(data, ExceptionCause::Overflow);
	EmitEpilogue();
	cc.ret();

	cc.bind(end);
}

void JitX64::EmitEpilogue() {
	EmitFlushRegisters();
}