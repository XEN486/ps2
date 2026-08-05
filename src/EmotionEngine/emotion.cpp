#include "emotion.hpp"
#include "Memory/memory.hpp"
#include "../utils.hpp"

using namespace EmotionEngine;

u32 Core::R5900::ReadCOP0(u8 reg) {
	switch (reg) {
		case 0: return cop0.index;
		case 2: return cop0.entrylo0;
		case 3: return cop0.entrylo1;
		case 5: return cop0.pagemask;
		case 6: return cop0.wired;
		case 9: return cop0.count;
		case 10: return cop0.entryhi;
		case 11: return cop0.compare;
		case 12: return cop0.status;
		case 15: return cop0.prid;
		case 16: return cop0.config;

		default: {
			error_log("read unknown register {}", reg);
			exit(1);
		}
	}
}

void Core::R5900::WriteCOP0(u8 reg, u32 val) {
	switch (reg) {
		case 0: { cop0.index = val; break; }
		case 2: { cop0.entrylo0 = val; break; }
		case 3: { cop0.entrylo1 = val; break; }
		case 5: { cop0.pagemask = val; break; }
		case 6: { cop0.wired = val; break; }
		case 9: { cop0.count = val; break; }
		case 10: { cop0.entryhi = val; break; }
		case 11: { cop0.compare = val; break; }
		case 12: { cop0.status = val; break; }
		case 16: { cop0.config = val; break; }

		default: {
			error_log("write {:08x} -> unknown register {}", val, reg);
			exit(1);
		}
	}
}

EE::EE(Core::JitBackend* backend, GraphicsSynthesizer::GS* gs) : m_JitBackend(backend), m_GS(gs) {
	if (!m_JitBackend->InitJit(&m_R5900, &m_Memory)) {
		error_log("failed to initialize backend");
		exit(1);
	}

	m_Memory.Initialize(m_JitBackend, this, m_GS);
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
	m_R5900.cop0.count += (size_t)block.instructions;

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

	// COP0 registers
	memset(&m_R5900.cop0, 0, sizeof(EmotionEngine::Core::Cop0));
	m_R5900.cop0.prid	= 0x00002e20; // Imp=2E, Rev=20 on reset
	m_R5900.cop0.config	= 0x00000440; // IC=010, DC=001 on reset
	m_R5900.cop0.status	= 0x00400004; // BEV=1, ERL=1 on reset

	m_JitBackend->Reset();
	m_DMAC.Reset();
	m_GIF.Reset();
}

void EE::Release() {
	m_JitBackend->Release();
	m_Memory.Release();
}