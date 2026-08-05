#include "emotion.hpp"
using namespace EmotionEngine::Core;

InstructionData JitBackend::AnalyzeOp(u32 instruction) {
	InstructionData data;
	data.pc = m_CompilePC - 4;
	data.type = InstructionType::Normal;
	data.pipeline1 = false;
	data.likely = false;
	DecodeOp(data, instruction);

	u8 op = (instruction >> 26) & 0b111111;
	switch (op) {
		// COP0
		case 0b010000: {
			if ((instruction & 0b11111111111) == 0) {
				switch (data.rs) {
					case 0b00000: { UseRegisters({data.rt, data.rd}); data.ptr = &JitBackend::MFC0; data.type = InstructionType::Sync; break; }
					default: {
						error_log("unknown cop0 opcode {:06b} {:08x} @ {:08x}", op, instruction, data.pc);
					}
				}
				break;
			}
			break;
		}
		default: {
			error_log("unknown opcode {:06b} {:08x} @ {:08x}", op, instruction, data.pc);
			exit(1);
		}
	}
	
	return data;
}