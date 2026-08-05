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
		// SPECIAL
		case 0b000000: {
			switch (data.funct) {
				// SLL
				case 0b000000: {
					// rt == rd && sa == 0 -> NOP
					if (data.rt == data.rd && data.sa == 0) {
						data.ptr = &JitBackend::NOP;
						break;
					}

					UseRegisters({data.rt, data.rd});
					data.ptr = &JitBackend::SLL;
					break;
				}

				// JR
				case 0b001000: {
					UseRegisters({data.rs});
					data.ptr = &JitBackend::JR;
					data.type = InstructionType::Branch;
					break;
				}

				default: {
					error_log("unknown special opcode {:06b} {:08x} @ {:08x}", data.funct, instruction, data.pc);
					exit(1);
				}
			}
			break;
		}

		// COP0
		case 0b010000: {
			if ((instruction & 0b11111111111) == 0) {
				switch (data.rs) {
					// MFC0
					case 0b00000: {
						UseRegisters({data.rt});
						data.ptr = &JitBackend::MFC0;
						break;
					}

					default: {
						error_log("unknown cop0 opcode {:06b} {:08x} @ {:08x}", data.rs, instruction, data.pc);
						exit(1);
					}
				}
				break;
			}
			break;
		}

		// normal
		case 0b001010: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::SLTI; break; }
		case 0b001111: { UseRegisters({data.rt}); data.ptr = &JitBackend::LUI; break; }
		case 0b001101: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::ORI; break; }
		
		// BNE
		case 0b000101: {
			UseRegisters({data.rs, data.rt});
			data.ptr = &JitBackend::BNE;
			data.type = InstructionType::Branch;
			break;
		}

		default: {
			error_log("unknown opcode {:06b} {:08x} @ {:08x}", op, instruction, data.pc);
			exit(1);
		}
	}
	
	return data;
}