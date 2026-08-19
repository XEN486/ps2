#include "emotion.hpp"
using namespace EmotionEngine::Core;

InstructionData JitBackend::AnalyzeOp(u32 instruction) {
	InstructionData data;
	data.pc = m_CompilePC - 4;
	data.type = InstructionType::Normal;
	data.in_branch_delay = false;
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
					// rt == 0 && rd == 0 && sa == 0 -> NOP
					if (data.rt == 0 && data.rd == 0 && data.sa == 0) {
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

				// MULT
				case 0b011000: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::MULT;
					break;
				}

				// MULTU
				case 0b011001: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::MULTU;
					break;
				}

				// DIV
				case 0b011010: {
					UseRegisters({data.rs, data.rt});
					data.ptr = &JitBackend::DIV;
					break;
				}

				// DIVU
				case 0b011011: {
					UseRegisters({data.rs, data.rt});
					data.ptr = &JitBackend::DIVU;
					break;
				}

				// BREAK
				case 0b001101: {
					data.ptr = &JitBackend::BREAK;
					break;
				}

				// MFLO
				case 0b010010: {
					UseRegisters({data.rd});
					data.ptr = &JitBackend::MFLO;
					break;
				}

				// ADDU
				case 0b100001: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::ADDU;
					break;
				}

				// SLT
				case 0b101010: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::SLT;
					break;
				}

				// MOVN
				case 0b001011: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::MOVN;
					break;
				}

				// MFHI
				case 0b010000: {
					UseRegisters({data.rd});
					data.ptr = &JitBackend::MFHI;
					break;
				}

				// SLTU
				case 0b101011: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::SLTU;
					break;
				}

				// SUBU
				case 0b100011: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::SUBU;
					break;
				}

				// AND
				case 0b100100: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::AND;
					break;
				}

				// SRL
				case 0b000010: {
					UseRegisters({data.rt, data.rd});
					data.ptr = &JitBackend::SRL;
					break;
				}

				// DSRL32
				case 0b111110: {
					UseRegisters({data.rt, data.rd});
					data.ptr = &JitBackend::DSRL32;
					break;
				}

				// DSRAV
				case 0b010111: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::DSRAV;
					break;
				}

				// MOVZ
				case 0b001010: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::MOVZ;
					break;
				}

				// DSLLV
				case 0b010100: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::DSLLV;
					break;
				}

				// MFSA
				case 0b101000: {
					UseRegisters({data.rd});
					data.ptr = &JitBackend::MFSA;
					break;
				}

				// SLLV
				case 0b000100: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::SLLV;
					break;
				}

				// SRAV
				case 0b000111: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::SRAV;
					break;
				}

				// NOR
				case 0b100111: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::NOR;
					break;
				}

				// SRLV
				case 0b000110: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::SRLV;
					break;
				}

				// SYSCALL
				case 0b001100: {
					data.ptr = &JitBackend::SYSCALL;
					data.type = InstructionType::EndBlock;
					break;
				}

				// SYNC.stype
				case 0b001111: {
					data.ptr = &JitBackend::NOP; // backend doesnt have to do anything
					data.type = InstructionType::EndBlock;
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

				// BLTZ
				case 0b00000: {
					UseRegisters({data.rs});
					data.ptr = &JitBackend::BLTZ;
					data.type = InstructionType::Branch;
					break;
				}

				// BLTZL
				case 0b00010: {
					UseRegisters({data.rs});
					data.ptr = &JitBackend::BLTZ;
					data.type = InstructionType::Branch;
					data.likely = true;
					break;
				}

				// BGEZL
				case 0b00011: {
					UseRegisters({data.rs});
					data.ptr = &JitBackend::BGEZ;
					data.type = InstructionType::Branch;
					data.likely = true;
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
					case 0b00000: { UseRegisters({data.rt}); data.ptr = &JitBackend::MFC0; break; } // MFC0
					case 0b00100: { UseRegisters({data.rt}); data.ptr = &JitBackend::MTC0; break; } // MTC0

					default: {
						error_log("unknown cop0 opcode (lo 11-bits == 0) {:05b} {:08x} @ {:08x}", data.rs, instruction, data.pc);
						exit(1);
					}
				}
				break;
			}

			switch (data.rs) {
				case 0b10000: {
					switch (data.funct) {
						case 0b000010: { data.ptr = &JitBackend::NOP; break; }	// TLBWI (don't care about emulating TLB)
						case 0b111001: { data.ptr = &JitBackend::DI; break; }	// DI
						case 0b111000: { data.ptr = &JitBackend::EI; break; }	// EI

						// ERET
						case 0b011000: {
							data.ptr = &JitBackend::ERET;
							data.type = InstructionType::EndBlock;
							break;
						}
						
						default: {
							error_log("unknown C0 opcode {:06b} {:08x} @ {:08x}", data.funct, instruction, data.pc);
							exit(1);
						}
					}
					break;
				}

				default: {
					error_log("unknown cop0 opcode {:05b} {:08x} @ {:08x}", data.rs, instruction, data.pc);
					exit(1);
				}
			}

			break;
		}

		// COP1
		case 0b010001: {
			switch (data.rs) {
				case 0b10000: {
					switch (data.funct) {
						default: {
							error_log("unknown cop1 S opcode {:06b} {:08x} @ {:08x}", data.funct, instruction, data.pc);
							data.ptr = &JitBackend::NOP;
							break;
						}
					}

					break;
				}

				case 0b00100: { UseRegisters({data.rt, data.rd}); data.ptr = &JitBackend::MTC1; break; }
				case 0b00110: { UseRegisters({data.rt, data.rd}); data.ptr = &JitBackend::CTC1; break; }

				default: {
					error_log("unknown cop1 opcode {:05b} {:08x} @ {:08x}", data.rs, instruction, data.pc);
					exit(1);
				}
			}

			break;
		}

		// COP2
		case 0b010010: {
			if (data.rs & 0b10000) {
				switch (instruction & 0x7ff) {
					default: {
						error_log("unknown vu instruction {:011b} {:08x} @ {:08x}", (instruction & 0x7ff), instruction, data.pc);
						data.ptr = &JitBackend::NOP;
						break;
					}
				}

				break;
			}

			switch (data.rs) {
				case 0b00010: { UseRegisters({data.rt, data.rd}); data.ptr = &JitBackend::CFC2; break; }
				case 0b00110: { UseRegisters({data.rt, data.rd}); data.ptr = &JitBackend::CTC2; break; }
				case 0b00001: { UseRegisters({data.rt, data.rd}); data.ptr = &JitBackend::QMFC2; break; }
				case 0b00101: { UseRegisters({data.rt, data.rd}); data.ptr = &JitBackend::QMTC2; break; }

				default: {
					error_log("unknown cop2 opcode {:05b} {:08x} @ {:08x}", data.rs, instruction, data.pc);
					exit(1);
				}
			}

			break;
		}

		// MMI
		case 0b011100: {
			switch (data.funct) {
				// MMI0
				case 0b001000: {
					switch (data.sa) {
						case 0b10010: {
							// PEXTLW
							UseRegisters({data.rs, data.rt, data.rd});
							data.ptr = &JitBackend::PEXTLW;
							break;
						}

						default: {
							error_log("unknown mmi0 opcode {:05b} {:08x} @ {:08x}", data.sa, instruction, data.pc);
							exit(1);
						}
					}

					break;
				}

				// MMI1
				case 0b101000: {
					switch (data.sa) {
						// PADDUW
						case 0b10000: {
							UseRegisters({data.rs, data.rt, data.rd});
							data.ptr = &JitBackend::PADDUW;
							break;
						}

						default: {
							error_log("unknown mmi1 opcode {:05b} {:08x} @ {:08x}", data.sa, instruction, data.pc);
							exit(1);
						}
					}
					break;
				}

				// MMI2
				case 0b001001: {
					switch (data.sa) {
						// PMFHI
						case 0b01000: {
							UseRegisters({data.rd});
							data.ptr = &JitBackend::PMFHI;
							break;
						}

						// PMFLO
						case 0b01001: {
							UseRegisters({data.rd});
							data.ptr = &JitBackend::PMFLO;
							break;
						}

						// PCPYLD
						case 0b01110: {
							UseRegisters({data.rs, data.rt, data.rd});
							data.ptr = &JitBackend::PCPYLD;
							break;
						}

						default: {
							error_log("unknown mmi2 opcode {:05b} {:08x} @ {:08x}", data.sa, instruction, data.pc);
							exit(1);
						}
					}

					break;
				}

				// MMI3
				case 0b101001: {
					switch (data.sa) {
						// POR
						case 0b10010: {
							UseRegisters({data.rs, data.rt, data.rd});
							data.ptr = &JitBackend::POR;
							break;
						}

						// PCPYHD
						case 0b01110: {
							UseRegisters({data.rs, data.rt, data.rd});
							data.ptr = &JitBackend::PCPYHD;
							break;
						}

						default: {
							error_log("unknown mmi3 opcode {:05b} {:08x} @ {:08x}", data.sa, instruction, data.pc);
							exit(1);
						}
					}	

					break;
				}

				// MULT1
				case 0b011000: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::MULT;
					data.pipeline1 = true;
					break;
				}

				// MULTU1
				case 0b011001: {
					UseRegisters({data.rs, data.rt, data.rd});
					data.ptr = &JitBackend::MULTU;
					data.pipeline1 = true;
					break;
				}

				// DIV1
				case 0b011010: {
					UseRegisters({data.rs, data.rt});
					data.ptr = &JitBackend::DIV;
					data.pipeline1 = true;
					break;
				}

				// DIVU1
				case 0b011011: {
					UseRegisters({data.rs, data.rt});
					data.ptr = &JitBackend::DIVU;
					data.pipeline1 = true;
					break;
				}

				// MFLO1
				case 0b010010: {
					UseRegisters({data.rd});
					data.ptr = &JitBackend::MFLO;
					data.pipeline1 = true;
					break;
				}

				// MFHI1
				case 0b010000: {
					UseRegisters({data.rd});
					data.ptr = &JitBackend::MFHI;
					data.pipeline1 = true;
					break;
				}

				default: {
					error_log("unknown mmi opcode {:06b} {:08x} @ {:08x}", data.funct, instruction, data.pc);
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
		case 0b001011: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::SLTIU; break; }
		case 0b100000: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::LB; break;}
		case 0b111001: { UseRegisters({data.rs}); data.ptr = &JitBackend::SWC1; break; }
		case 0b101000: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::SB; break;}
		case 0b100101: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::LHU; break; }
		case 0b101001: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::SH; break; }
		case 0b001110: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::XORI; break; }
		case 0b100111: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::LWU; break; }
		case 0b011001: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::DADDIU; break; }
		case 0b011110: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::LQ; break; }
		case 0b011111: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::SQ; break; }
		case 0b100001: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::LH; break; }
		case 0b011010: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::LDL; break; }
		case 0b011011: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::LDR; break; }
		case 0b101100: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::SDL; break; }
		case 0b101101: { UseRegisters({data.rs, data.rt}); data.ptr = &JitBackend::SDR; break; }

		// don't care about emulating CACHE instructions
		case 0b101111: { data.ptr = &JitBackend::NOP; break; }
		
		// BNE
		case 0b000101: {
			UseRegisters({data.rs, data.rt});
			data.ptr = &JitBackend::BNE;
			data.type = InstructionType::Branch;
			break;
		}

		// BNEL
		case 0b010101: {
			UseRegisters({data.rs, data.rt});
			data.ptr = &JitBackend::BNE;
			data.type = InstructionType::Branch;
			data.likely = true;
			break;
		}

		// BEQ
		case 0b000100: {
			UseRegisters({data.rs, data.rt});
			data.ptr = &JitBackend::BEQ;
			data.type = InstructionType::Branch;
			break;
		}

		// BEQL
		case 0b010100: {
			UseRegisters({data.rs, data.rt});
			data.ptr = &JitBackend::BEQ;
			data.type = InstructionType::Branch;
			data.likely = true;
			break;
		}

		// BLEZ
		case 0b000110: {
			UseRegisters({data.rs});
			data.ptr = &JitBackend::BLEZ;
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

		// JAL
		case 0b000011: {
			UseRegisters({31});
			data.ptr = &JitBackend::JAL;
			data.type = InstructionType::Branch;
			break;
		}

		// J
		case 0b000010: {
			data.ptr = &JitBackend::J;
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