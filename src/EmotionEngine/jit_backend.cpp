#include "emotion.hpp"
#include "../utils.hpp"
#include "../config.hpp"

using namespace EmotionEngine::Core;
using namespace asmjit;

bool JitBackend::InitJit(R5900* cpu, Memory* memory) {
	m_Memory = memory;
	m_R5900 = cpu;
	m_Logger.set_file(fopen("asmjit.log", "w"));

	Error err = m_CodeHolder.init(m_Runtime.environment(), m_Runtime.cpu_features());

#ifdef ENABLE_ASMJIT_LOGGING
	m_CodeHolder.set_logger(&m_Logger);
#endif

	return err == Error::kOk;
}

void JitBackend::Reset() {
	//m_InBranchDelay = false;
	memset(m_UsedRegisters, 0, sizeof(bool) * 32);
}

void JitBackend::Release() {
	for (auto& element : m_BlockCache) {
		CompiledBlock& block = element.second;
		if (!block.valid) continue;

		block.valid = false;
		m_Runtime.release(block.fn);
	}
}

void JitBackend::Invalidate(u32 pc) {
#ifdef ENABLE_SELF_MODIFYING_CODE
	//pc &= 0x1fffffff;
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

	memset(m_UsedRegisters, 0, sizeof(bool) * 32);
	m_Instructions.clear();
	m_CodeHolder.reinit();

	u32 end_pc = 0;
	while (true) {
		end_pc = m_CompilePC;

		InstructionData data = AnalyzeOp(Fetch());
		if (data.type == InstructionType::Normal) {
			m_Instructions.push_back(data);
			block.instructions++;
			continue;
		}

		else if (data.type == InstructionType::Branch) {
			// analyze and store branch delay slot first
			InstructionData branch_delay = AnalyzeOp(Fetch());
			branch_delay.in_branch_delay = true;
			data.branch_delay = std::make_shared<InstructionData>(branch_delay);
			block.instructions++;

			// recompile branch and break out of this loop
			m_Instructions.push_back(data);
			block.instructions++;

			// account for delay slot
			end_pc += 4;
			//m_InBranchDelay = true;
			break;
		}

		// end the block early if this instruction is supposed to end the block
		else if (data.type == InstructionType::EndBlock) {
			m_Instructions.push_back(data);
			block.instructions++;

			break;
		}
	}

	EmitBeginBlock();
	for (auto& data : m_Instructions) {
		(this->*(data.ptr))(data);
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
    auto it = m_BlockCache.find(pc);
    if (it != m_BlockCache.end() && it->second.valid) {
		return it->second;
	}

    //debug_log("compiling new block @ {:04x}", pc);
    return RecompileBlock(pc);
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

void JitBackend::EmitBranchDelay(InstructionData& data) {
	(this->*(data.branch_delay->ptr))(*data.branch_delay);
}