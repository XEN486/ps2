#include "iop.hpp"
using namespace IOProcessor;

InstructionData JitBackend::AnalyzeOp(u32 instruction) {
	InstructionData data;
	data.pc = m_CompilePC - 4;
	data.in_branch_delay = false;
	data.type = InstructionType::Normal;
	DecodeOp(data, instruction);

	u8 op = (instruction >> 26) & 0b111111;
	switch (op) {
		// --- special ---
		case 0b000000: {
			switch (data.funct) {
				// SLL
				case 0b000000: {
					// optimize canonical NOP (SLL $0,$0,0)
					if (data.rt == 0 && data.rd == 0 && data.sa == 0) {
						data.ptr = &JitBackend::NOP;
						break;
					}

					UseRegisters({data.rt, data.rd});
					data.ptr = &JitBackend::SLL;
					break;
				}

				// OR
				case 0b100101: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::OR;
					break;
				}

				// SLTU
				case 0b101011: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::SLTU;
					break;
				}
				
				// ADDU
				case 0b100001: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::ADDU;
					break;
				}

				// SLLV
				case 0b000100: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::SLLV;
					break;
				}

				// AND
				case 0b100100: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::AND;
					break;
				}

				// ADD
				case 0b100000: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::ADD;
					break;
				}

				// SUBU
				case 0b100011: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::SUBU;
					break;
				}

				// SRA
				case 0b000011: {
					UseRegisters({data.rt, data.rd});
					data.ptr = &JitBackend::SRA;
					break;
				}

				// DIV
				case 0b011010: {
					UseRegisters({data.rs, data.rt});
					data.ptr = &JitBackend::DIV;
					break;
				}

				// MFLO
				case 0b010010: {
					UseRegisters({data.rd});
					data.ptr = &JitBackend::MFLO;
					break;
				}

				// SRL
				case 0b000010: {
					UseRegisters({data.rt, data.rd});
					data.ptr = &JitBackend::SRL;
					break;
				}

				// DIVU
				case 0b011011: {
					UseRegisters({data.rs, data.rt});
					data.ptr = &JitBackend::DIVU;
					break;
				}

				// MFHI
				case 0b010000: {
					UseRegisters({data.rd});
					data.ptr = &JitBackend::MFHI;
					break;
				}
				
				// SLT
				case 0b101010: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::SLT;
					break;
				}

				// MTLO
				case 0b010011: {
					UseRegisters({data.rs});
					data.ptr = &JitBackend::MTLO;
					break;
				}

				// MTHI
				case 0b010001: {
					UseRegisters({data.rs});
					data.ptr = &JitBackend::MTHI;
					break;
				}

				// NOR
				case 0b100111: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::NOR;
					break;
				}

				// SRAV
				case 0b000111: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::SRAV;
					break;
				}

				// SRLV
				case 0b000110: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::SRLV;
					break;
				}

				// MULTU
				case 0b011001: {
					UseRegisters({data.rs, data.rt});
					data.ptr = &JitBackend::MULTU;
					break;
				}

				// XOR
				case 0b100110: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::XOR;
					break;
				}

				// MULT
				case 0b011000: {
					UseRegisters({data.rs, data.rt});
					data.ptr = &JitBackend::MULT;
					break;
				}

				// SUB
				case 0b100010: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::SUB;
					break;
				}

				// --- branches ---
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

				// --- special ---
				// SYSCALL
				case 0b001100: {
					data.ptr = &JitBackend::SYSCALL;
					data.type = InstructionType::Syscall;
					break;
				}

				// BREAK
				case 0b001101: {
					data.ptr = &JitBackend::BREAK;
					data.type = InstructionType::Syscall;
					break;
				}

				default: {
					error_log("unknown special opcode {:06b} @ {:08x}", data.funct, data.pc);
					data.ptr = &JitBackend::Illegal;
					data.type = InstructionType::Syscall;
					break;
				}
			}

			break;
		}

		// --- regimm ---
		case 0b000001: {
			switch (data.rt) {
				// BLTZ
				case 0b00010:
				case 0b00000: {
					UseRegisters({data.rs}); 
					data.ptr = &JitBackend::BLTZ; 
					data.type = InstructionType::Branch;
					break;
				}

				// BGEZ
				case 0b00011:
				case 0b00001: {
					UseRegisters({data.rs}); 
					data.ptr = &JitBackend::BGEZ; 
					data.type = InstructionType::Branch;
					break;
				}

				// BLTZAL
				case 0b10010:
				case 0b10000: {
					UseRegisters({data.rs, 31}); 
					data.ptr = &JitBackend::BLTZAL; 
					data.type = InstructionType::Branch;
					break;
				}

				// BGEZAL
				case 0b10011:
				case 0b10001: {
					UseRegisters({data.rs, 31}); 
					data.ptr = &JitBackend::BGEZAL; 
					data.type = InstructionType::Branch;
					break;
				}

				default: {
					error_log("unknown regimm opcode {:05b} @ {:08x}", data.rt, data.pc);
					data.ptr = &JitBackend::Illegal;
					data.type = InstructionType::Syscall;
					break;;
				}
			}
			break;	
		}

		// --- cop0 ---
		case 0b010000: {
			switch (data.rs) {
				case 0b00100: { UseRegisters({data.rt}); data.ptr = &JitBackend::MTC0; break; }
				case 0b00000: { UseRegisters({data.rt}); data.ptr = &JitBackend::MFC0; break; }
				case 0b10000: { data.ptr = &JitBackend::RFE; break; }

				default: {
					error_log("unknown cop0 opcode {:05b} @ {:08x}", data.rs, data.pc);
					data.ptr = &JitBackend::Illegal;
					data.type = InstructionType::Syscall;
					break;;
				}
			}
			break;
		}

		// --- normal ---
		case 0b001111: {
			// try optimize for LUI+ORI
			// ORI = 001101
			u32 next = m_Memory->ReadVirtualMemory32(m_CompilePC);
			if (((next >> 26) & 0b111111) == 0b001101) {
				InstructionData ori_data;
				DecodeOp(ori_data, next);

				// following condition must be met to optimize:
				// ORI.rs == LUI.rt
				// ORI.rt == LUI.rt
				if (ori_data.rs == data.rt && ori_data.rt == data.rt) {
					UseRegisters({data.rt});

					data.addr = (data.imm << 16) | ori_data.imm;
					data.ptr = &JitBackend::MoveImm;

					// skip past ORI
					m_CompilePC += 4;
					break;
				}
				
			}
			
			UseRegisters({data.rt});
			data.ptr = &JitBackend::LUI;
			break;
		}
		
		case 0b001101: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::ORI; break; }
		case 0b101011: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::SW; break; }
		case 0b001001: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::ADDIU; break; }
		case 0b100101: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::LHU; break; }
		case 0b001000: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::ADDI; break; }
		case 0b100011: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::LW; break; }
		case 0b101001: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::SH; break; }
		case 0b001100: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::ANDI; break; }
		case 0b101000: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::SB; break; }
		case 0b100000: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::LB; break; }
		case 0b100100: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::LBU; break; }
		case 0b001010: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::SLTI; break; }
		case 0b001011: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::SLTIU; break; }
		case 0b100001: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::LH; break; }
		case 0b001110: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::XORI; break; }
		case 0b100010: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::LWL; break; }
		case 0b100110: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::LWR; break; }
		case 0b101010: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::SWL; break; }
		case 0b101110: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::SWR; break; }

		// --- branches ---
		// J
		case 0b000010: {
			data.ptr = &JitBackend::J;
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

		// BGTZ
		case 0b000111: {
			UseRegisters({data.rs});
			data.ptr = &JitBackend::BGTZ;
			data.type = InstructionType::Branch;
			break;
		}

		// BLEZ
		case 0b000110: {
			UseRegisters({data.rs});
			data.ptr = &JitBackend::BLEZ;
			data.type = InstructionType::Branch;
			break;
		}

		default: {
			error_log("unknown opcode {:06b} @ {:08x}", op, data.pc);
			data.ptr = &JitBackend::Illegal;
			data.type = InstructionType::Syscall;
			break;
		}
	}

	return data;
}