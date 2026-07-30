#include "emotion.hpp"
#include "../utils.hpp"
#include "../config.hpp"

#include <asmjit/x86.h>
using namespace EmotionEngine::MIPS;
using namespace asmjit;

bool JitBackend::InitJit(R5900* cpu) {
	m_R5900 = cpu;
	m_Logger.set_file(stdout);

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
			(this->*(data.ptr))();
			block.instructions++;
			continue;
		}

		else if (data.type == InstructionType::Branch) {
			// recompile branch delay slot first
			InstructionData delay_slot = AnalyzeOp(Fetch());
			(this->*(delay_slot.ptr))();
			block.instructions++;

			// recompile branch and break out of this loop
			(this->*(data.ptr))();
			block.instructions++;

			// account for delay slot in end_pc
			end_pc += 4;
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
    auto it = m_BlockCache.find(pc);
    if (it != m_BlockCache.end() && it->second.valid) {
		return it->second;
	}

    //debug_log("compiling new block @ {:04x}", pc);
    return RecompileBlock(pc);
}

InstructionData JitBackend::AnalyzeOp(u32 opcode) {
	switch (opcode) {
		case 0: {
			return InstructionData {
				InstructionType::Branch,
				&JitBackend::test,
			};
		}
		default: {
			error_log("unknown opcode {:08x}", opcode);
			exit(1);
		}
	}
}

/*
void JitBackend::RecompileOp(u32 opcode, u32 old_pc) {
	switch (opcode) {
		case 0: {
			return test();
		}
		default: {
			error_log("unknown opcode {:08x} [pc={:08x}]", opcode, old_pc);
			exit(1);
		}
	}
}
*/