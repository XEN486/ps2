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

				// SRA
				case 0b000011: {
					UseRegisters({data.rt, data.rd});
					data.ptr = &JitBackend::SRA;
					break;
				}

				// DSRL
				case 0b111010: {
					UseRegisters({data.rt, data.rd});
					data.ptr = &JitBackend::DSRL;
					break;
				}

				// DSLL
				case 0b111000: {
					UseRegisters({data.rt, data.rd});
					data.ptr = &JitBackend::DSLL;
					break;
				}

				// DSLL32
				case 0b111100: {
					UseRegisters({data.rt, data.rd});
					data.ptr = &JitBackend::DSLL32;
					break;
				}

				// DSRA32
				case 0b111111: {
					UseRegisters({data.rt, data.rd});
					data.ptr = &JitBackend::DSRA32;
					break;
				}

				// OR
				case 0b100101: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::OR;
					break;
				}

				// DADDU
				case 0b101101: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::DADDU;
					break;
				}

				// SYNC.stype
				case 0b001111: {
					data.ptr = &JitBackend::NOP; // backend doesnt have to do anything
					data.type = InstructionType::Sync;
					break;
				}

				// JR
				case 0b001000: {
					UseRegisters({data.rs});
					data.ptr = &JitBackend::JR;
					data.type = InstructionType::Branch;
					break;
				}

				// JALR
				case 0b001001: {
					UseRegisters({data.rs, data.rd});
					data.ptr = &JitBackend::JALR;
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

		// REGIMM
		case 0b000001: {
			switch (data.rt) {
				// BGEZ
				case 0b00001: {
					UseRegisters({data.rs});
					data.ptr = &JitBackend::BGEZ;
					data.type = InstructionType::Branch;
					break;
				}

				default: {
					error_log("unknown regimm opcode {:05b} {:08x} @ {:08x}", data.rt, instruction, data.pc);
					exit(1);
				}
			}
			break;
		}

		// COP0
		case 0b010000: {
			if ((instruction & 0b11111111111) == 0) {
				switch (data.rs) {
					case 0b00000: { UseRegisters({data.rt, data.rd}); data.ptr = &JitBackend::MFC0; break; } // MFC0
					case 0b00100: { UseRegisters({data.rt, data.rd}); data.ptr = &JitBackend::MTC0; break; } // MTC0

					default: {
						error_log("unknown cop0 opcode (lo 11-bits == 0) {:05b} {:08x} @ {:08x}", data.rs, instruction, data.pc);
						exit(1);
					}
				}
				break;
			}

			switch (data.funct) {
				case 0b000010: { data.ptr = &JitBackend::NOP; break; } // TLBWI (don't care about emulating TLB)
				default: {
					error_log("unknown cop0 opcode {:06b} {:08x} @ {:08x}", data.funct, instruction, data.pc);;
					exit(1);
				}
			}

			break;
		}

		// normal
		case 0b001010: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::SLTI; break; }
		case 0b001111: { UseRegisters({data.rt}); data.ptr = &JitBackend::LUI; break; }
		case 0b001101: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::ORI; break; }
		case 0b001001: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::ADDIU; break; }
		case 0b101011: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::SW; break; }
		case 0b111111: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::SD; break; }
		case 0b100100: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::LBU; break; }
		case 0b001100: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::ANDI; break; }
		case 0b110111: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::LD; break; }
		case 0b100011: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::LW; break; }
		
		// BNE
		case 0b000101: {
			UseRegisters({data.rs, data.rt});
			data.ptr = &JitBackend::BNE;
			data.type = InstructionType::Branch;
			break;
		}

		// BEQ
		case 0b000100: {
			UseRegisters({data.rs, data.rt});
			data.ptr = &JitBackend::BEQ;
			data.type = InstructionType::Branch;
			break;
		}

		// JAL
		case 0b000011: {
			UseRegisters({31});
			data.ptr = &JitBackend::JAL;
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