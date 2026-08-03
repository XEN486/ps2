#include "emotion.hpp"
#include "Memory/memory.hpp"
#include "../utils.hpp"

using namespace EmotionEngine;

EE::EE(Core::JitBackend* backend, GraphicsSynthesizer::GS* gs) : m_JitBackend(backend), m_GS(gs) {
	if (!m_JitBackend->InitJit(&m_R5900, &m_Memory)) {
		error_log("failed to initialize backend");
		exit(1);
	}

	m_Memory.Initialize(m_JitBackend, &m_DMAC, m_GS);
	m_GIF.SetGS(m_GS);
	m_DMAC.SetPointers(&m_Memory, &m_GIF);
}

size_t EE::RunOnce() {
	Core::CompiledBlock& block = m_JitBackend->GetOrCompileBlock(m_R5900.pc);
	block.execution_count++;

	m_R5900.next_pc = block.after_end_pc;
	block.fn();
	m_R5900.pc = m_R5900.next_pc;

	//if (block.execution_count == 1) {
	//	debug_log("execute new block {:04x}->{:04x} [{} instructions]", block.start_pc, block.end_pc, block.instructions);
	//}

	// assume 1 instruction = 1 clock cycle.
	// tick dmac every other cycle
	for (size_t i = 0; i < (block.instructions / 2); i++) {
		m_DMAC.Tick();
		m_GIF.ProcessQword();
	}

	return block.instructions;
}

void EE::Reset() {
	for (u8 i = 0; i < 32; i++) {
		m_R5900.gpr[i].reg_u128 = 0;
	}

	for (u8 i = 0; i < 32; i++) {
		m_R5900.fpr[i] = 0.0f;
	}
	
	m_R5900.pc = 0xbfc00000;

	m_JitBackend->Reset();
	m_DMAC.Reset();
	m_GIF.Reset();
}

void EE::Release() {
	m_JitBackend->Release();
	m_Memory.Release();
}