#include "jit_x64.hpp"

using namespace EmotionEngine::MIPS;
using namespace asmjit;

bool JitX64::InitJit(R5900* cpu) {
	if (!JitBackend::InitJit(cpu)) return false;
	if (m_CodeHolder.attach(&cc) != Error::kOk) return false;
	return true;
}

void JitX64::EmitBeginBlock() {
	cc.add_func(FuncSignature::build<void, R5900*>());

	// r5900 = pointer to R5900 struct
	r5900 = cc.new_gp_ptr("r5900");
	cc.mov(r5900, m_R5900);

	// make a few scratch registers so we dont have to keep allocating
	scratch1 = cc.new_gp64("scratch1");
	scratch2 = cc.new_gp64("scratch2");
	scratch3 = cc.new_gp64("scratch3");
	scratch4 = cc.new_gp64("scratch4");

	// this can be freely overwritten by Emit*() functions
	temp = cc.new_gp64("temp");
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