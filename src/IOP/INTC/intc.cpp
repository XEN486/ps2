#include "intc.hpp"
#include "../iop.hpp"

using namespace IOProcessor::Interrupt;

void INTC::Interrupt(IRQ irq) {
	// set bit in I_STAT
	m_STAT |= (1 << static_cast<u8>(irq));
	TryInterrupt();
}

void INTC::TryInterrupt() {
	if (!m_CTRL) {
		m_IOP->GetR3000A().cop0[IOProcessor::CAUSE] &= ~(1u << 10);
		return;
	}

	if (m_STAT & m_MASK) {
		m_IOP->GetR3000A().cop0[IOProcessor::CAUSE] |= (1u << 10);
	} else {
		m_IOP->GetR3000A().cop0[IOProcessor::CAUSE] &= ~(1u << 10);
	}
}