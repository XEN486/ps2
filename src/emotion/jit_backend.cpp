#include "emotion.hpp"
#include "../utils.hpp"
#include "../config.hpp"

#include <asmjit/x86.h>
using namespace EmotionEngine::MIPS;
using namespace asmjit;

bool JitBackend::InitJit(R5900* cpu) {
	m_R5900 = cpu;
	m_Logger.set_file(fopen("asmjit.log", "w"));

	Error err = m_CodeHolder.init(m_Runtime.environment(), m_Runtime.cpu_features());

#ifdef ENABLE_ASMJIT_LOGGING
	m_CodeHolder.set_logger(&m_Logger);
#endif

	return err == Error::kOk;
}

void JitBackend::Release() {
	for (auto& element : m_BlockCache) {
		CompiledBlock& block = element.second;
		if (!block.valid) continue;

		m_Runtime.release(block.fn);
	}
}

void JitBackend::Invalidate(u32 pc) {
#ifdef ENABLE_SELF_MODIFYING_CODE
	for (auto& element : m_BlockCache) {
		CompiledBlock& block = element.second;
		if (!block.valid) continue;

		// invalidate if address is in block
		if (pc >= block.start_pc && pc <= block.end_pc) {
			debug_log("invalidating block {:08x}->{:08x}", block.start_pc, block.end_pc);
			block.valid = false;
		}
	}
#endif
}

CompiledBlock& JitBackend::RecompileBlock(u32 pc) {
	CompiledBlock& block = m_BlockCache[pc];
	m_CompilePC = pc;
	block.start_pc = pc;
	block.instructions = 0;

	m_CodeHolder.reinit();

	u32 end_pc = 0;
	bool delay_slot = false;
	bool do_recompile = true;

	EmitBeginBlock();
	bool compile_delay_slot = false;

	while (true) {
		end_pc = m_CompilePC;

		InstructionData data = AnalyzeOp(Fetch());
		if (data.type == InstructionType::Normal) {
			(this->*(data.ptr))(data);
			block.instructions++;
			continue;
		}

		else if (data.type == InstructionType::Branch) {
			// recompile branch delay slot first
			InstructionData delay_slot = AnalyzeOp(Fetch());
			(this->*(delay_slot.ptr))(delay_slot);
			block.instructions++;

			// recompile branch and break out of this loop
			(this->*(data.ptr))(data);
			block.instructions++;

			// account for delay slot
			end_pc += 4;
			break;
		}

		// end the block early if this is a syscall or sync instruction
		else if (data.type == InstructionType::Syscall || data.type == InstructionType::Sync) {
			(this->*(data.ptr))(data);
			block.instructions++;

			break;
		}
	}
	EmitEndBlock();

	block.execution_count = 0;
	block.after_end_pc = m_CompilePC;
	block.end_pc = end_pc;

	// add the function to the jit runtime
	Error err = m_Runtime.add(&block.fn, &m_CodeHolder);
	if (err != Error::kOk) {
		error_log("asmjit error: {}", DebugUtils::error_as_string(err));
		exit(1);
	}

	block.valid = true;
	return block;
}

CompiledBlock& JitBackend::GetOrCompileBlock(u32 pc) {
	VirtualToPhysical(pc);

    auto it = m_BlockCache.find(pc);
    if (it != m_BlockCache.end() && it->second.valid) {
		return it->second;
	}

    //debug_log("compiling new block @ {:04x}", pc);
    return RecompileBlock(pc);
}

inline InstructionData JitBackend::AnalyzeOp(u32 instruction) {
	InstructionData data;
	data.type = InstructionType::Normal;
	DecodeOp(data, instruction);

	u8 op = (instruction >> 26) & 0b111111;
	switch (op) {
		// SPECIAL
		case 0b000000: {
			switch (data.funct) {
				case 0b000000: { data.ptr = &JitBackend::SLL; break; }		// SLL
				case 0b101011: { data.ptr = &JitBackend::SLTU; break; }		// SLTU
				case 0b101101: { data.ptr = &JitBackend::DADDU; break; }	// DADDU
				case 0b011000: { data.ptr = &JitBackend::MULT; break; }		// MULT
				case 0b100001: { data.ptr = &JitBackend::ADDU; break; }		// ADDU
				case 0b100100: { data.ptr = &JitBackend::AND; break; }		// AND

				// system call (HLE for now)
				case 0b001100: { data.ptr = &JitBackend::SYSCALL; data.type = InstructionType::Syscall; break; }

				// sync (ends the block)
				case 0b001111: { data.ptr = &JitBackend::SYNC; data.type = InstructionType::Sync; break; }

				// branch
				case 0b001000: { data.ptr = &JitBackend::JR; data.type = InstructionType::Branch; break; }

				default: {
					error_log("unknown special opcode {:06b} {:08x}", data.funct, instruction);
					exit(1);
				}
			}

			break;
		}

		// normal
		case 0b001111: { data.ptr = &JitBackend::LUI; break; }		// LUI
		case 0b001001: { data.ptr = &JitBackend::ADDIU; break; }	// ADDIU
		case 0b011111: { data.ptr = &JitBackend::SQ; break; }		// SQ
		case 0b010000: { data.ptr = &JitBackend::EI; break; }		// EI
		case 0b100011: { data.ptr = &JitBackend::LW; break; }		// LW
		case 0b111111: { data.ptr = &JitBackend::SD; break; }		// SD
		case 0b101011: { data.ptr = &JitBackend::SW; break; }		// SW
		case 0b100101: { data.ptr = &JitBackend::LHU; break; }		// LHU
		case 0b101001: { data.ptr = &JitBackend::SH; break; }		// SH
		case 0b001101: { data.ptr = &JitBackend::ORI; break; }		// ORI

		// branch
		case 0b000101: { data.ptr = &JitBackend::BNE; data.type = InstructionType::Branch; break; }	// BNE
		case 0b000011: { data.ptr = &JitBackend::JAL; data.type = InstructionType::Branch; break; }	// JAL

		default: {
			error_log("unknown opcode {:06b} {:08x}", op, instruction);
			exit(1);
		}
	}
	
	return data;
}

void JitBackend::DecodeOp(InstructionData& data, u32 instruction) {
	// R-type and I-type
	data.rs = (instruction >> 21) & 0b11111;
	data.rt = (instruction >> 16) & 0b11111;

	// R-type
	data.rd = (instruction >> 11) & 0b11111;
	data.sa = (instruction >> 6) & 0b11111;
	data.funct = (instruction >> 0) & 0b111111;
	
	// I-type
	data.imm = instruction & 0xffff;

	// J-type
	data.addr = instruction & 0x3ffffff;
}