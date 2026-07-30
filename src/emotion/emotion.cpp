#include "emotion.hpp"
#include "../memory.hpp"
#include "../utils.hpp"

using namespace EmotionEngine;

EE::EE(MIPS::JitBackend* backend) : m_JitBackend(backend) {
	if (!m_JitBackend->InitJit(&m_R5900)) {
		error_log("failed to initialize backend");
		exit(1);
	}
}

size_t EE::RunOnce() {
	MIPS::CompiledBlock& block = m_JitBackend->GetOrCompileBlock(m_R5900.pc);
	block.execution_count++;

	m_R5900.next_pc = block.after_end_pc;
	block.fn(&m_R5900);
	m_R5900.pc = m_R5900.next_pc;

	if (block.execution_count == 1) {
		debug_log("execute new block {:04x}->{:04x} [{} instructions]", block.start_pc, block.end_pc, block.instructions);
	}

	return block.instructions;
}

void EE::Reset() {
	for (u8 i = 0; i < 32; i++) {
		m_R5900.regs[i].reg_u128 = 0;
	}
	
	m_R5900.pc = 0xbfc00000;
}

void EE::Release() {
	m_JitBackend->Release();
	Memory::Release();
}