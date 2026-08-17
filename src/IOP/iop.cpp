#include "iop.hpp"
#include <fstream>

#define EXCEPTION_WRITE_MASK 0b11110000000000000000000001111100
using namespace IOProcessor;

IOP::IOP(JitBackend* backend, SubsystemInterface::SIF* sif)
	: m_JitBackend(backend), m_DMAC(&m_Memory, &m_INTC), m_SIF(sif) {
	if (!m_JitBackend->InitJit(&m_R3000A, &m_Memory)) {
		error_log("failed to initialize backend");
		exit(1);
	}

	m_Memory.Initialize(this);
	m_INTC.Initialize(this);
}

size_t IOP::RunOnce() {
	CompiledBlock& block = m_JitBackend->GetOrCompileBlock(m_R3000A.pc);
	block.execution_count++;

	m_R3000A.next_pc = block.after_end_pc;
	block.fn();
	m_R3000A.pc = m_R3000A.next_pc;

	// IOP console
	if (m_R3000A.pc == 0x12c48 || m_R3000A.pc == 0x1420c || m_R3000A.pc == 0x1430c) [[unlikely]] {
		u32 ptr = m_R3000A.gpr[5];
		u32 text_size = m_R3000A.gpr[6];
		while (text_size) {
			std::print("{}", (char)m_Memory.ReadVirtualMemory8(ptr));

			ptr++;
			text_size--;
		}
	}

	//if (block.execution_count == 1) {
	//	debug_log("execute new block {:04x}->{:04x} [{} instructions]", block.start_pc, block.end_pc, block.instructions);
	//}

	// assume 1 instruction = 1 clock cycle.
	//m_R3000A.cop0.count += (u32)block.instructions;

	// check for interrupts
	// EPC = beginning of the new block
	if ((m_R3000A.cop0[SR] & 1) && (m_R3000A.cop0[CAUSE] & m_R3000A.cop0[SR] & 0xff00)) {
		m_R3000A.Exception(ExceptionCause::Interrupt, m_R3000A.pc);
	}

	return block.instructions;
}

void IOP::Reset() {
	for (u8 i = 0; i < 32; i++) {
		m_R3000A.gpr[i] = 0;
	}
	
	m_R3000A.pc = 0xbfc00000;

	// COP0 registers
	memset(&m_R3000A.cop0, 0, 32 * sizeof(u32));
	m_R3000A.cop0[SR] = 0x10900000;
	m_R3000A.cop0[PRID] = 0x00000001f;

	m_JitBackend->Reset();
	m_INTC.Reset();
	m_DMAC.Reset();
}

void IOP::Release() {
	m_JitBackend->Release();
	m_Memory.Release();
}

u32 R3000A::ReadCOP0(u8 reg) {
	return cop0[reg];
}

void R3000A::WriteCOP0(u8 reg, u32 word) {
	//debug_log("write cop0[{}] <- {:08x}", reg, word);

	// only bit 8/9 are R/W
	if (reg == CAUSE) {
		u32 mask = 0b11 << 8;
		cop0[CAUSE] = (cop0[CAUSE] & (~mask)) | (word & mask);
	}

	if (reg == BPC || reg == BDA || reg == DCIC || reg == BDAM || reg == BPCM || reg == SR) {
		cop0[reg] = word;
	}
}

void R3000A::Exception(ExceptionCause cause, u32 epc, bool in_delay_slot) {
	// set EPC
	cop0[EPC] = epc;

	// set CAUSE
	u8 cause_bits = (u8)cause << 2;
	cop0[CAUSE] = (cop0[CAUSE] & ~EXCEPTION_WRITE_MASK) | (cause_bits & EXCEPTION_WRITE_MASK);

	// find handler from BEV bit
	bool bev = (cop0[SR] & (1 << 22)) != 0;
	u32 handler = bev ? 0xbfc00180 : 0x80000080;

	// update SR
	u8 mode = cop0[SR] & 0x3f;
	cop0[SR] &= ~0x3f;
	cop0[SR] |= (mode << 2) & 0x3f;
	
	// if we are in a branch delay slot, EPC points to the branch instruction and bit 31 in CAUSE is set
	if (in_delay_slot) {
		cop0[EPC] -= 4;
		cop0[CAUSE] |= 1 << 31;
	}

	// no branch delay for exceptions
	next_pc = handler;
}