#include "jit_x64.hpp"

using namespace EmotionEngine::Core;
using namespace asmjit;

static u32 WRAP_ReadCOP0(EmotionEngine::Core::R5900* r5900, u8 reg) { return r5900->ReadCOP0(reg); }
static void WRAP_WriteCOP0(EmotionEngine::Core::R5900* r5900, u8 reg, u32 val) { r5900->WriteCOP0(reg, val); }

void JitX64::MFC0(InstructionData& data) {
	InvokeNode* node;
	cc.invoke(Out(node), reinterpret_cast<uintptr_t>(&WRAP_ReadCOP0), FuncSignature::build<u32, R5900*, u8>());
	node->set_arg(0, m_R5900);
	node->set_arg(1, data.rd);
	node->set_ret(0, r[data.rt].r32());
	cc.movsxd(r[data.rt].r64(), r[data.rt].r32());
}