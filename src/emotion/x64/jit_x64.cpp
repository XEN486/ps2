#include "jit_x64.hpp"
#include "../../utils.hpp"

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

void JitX64::test() {
	// $00000004: $deadbeefcafebabe
	EmitWriteVirtualMemory32(0x00000004, 0xcafebabe);
	EmitWriteVirtualMemory32(0x00000008, 0xdeadbeef);

	x86::Gp data = cc.new_gp64("data");
	
	EmitReadVirtualMemory32(data, 0x00000008);
	cc.shl(data, 32);
	EmitReadVirtualMemory32(data, 0x00000004);

	EmitStoreRegister(R64, 1, data); // R1 <-data
	
	cc.mov(x86::dword_ptr(r5900, offsetof(R5900, next_pc)), 0);
}