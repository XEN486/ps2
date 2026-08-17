#include "intc.hpp"
#include "../iop.hpp"

using namespace IOProcessor::Interrupt;

void INTC::Interrupt(IRQ irq) {
	// set bit in I_STAT
	m_STAT |= (1 << static_cast<u8>(irq));
	TryInterrupt();
}

void INTC::TryInterrupt() {
	if (!m_CTRL) return;
	for (u8 i = 0; i <= 25; i++) {
		u32 bit = (1 << i);

		if ((m_STAT & bit) && (m_MASK & bit)) {
			// cop0r13.10 set
			m_IOP->GetR3000A().cop0[IOProcessor::CAUSE] |= (1 << 10);
			return;
		}
		
		// clear cop0r13.10
		else {
			m_IOP->GetR3000A().cop0[IOProcessor::CAUSE] &= ~(u32)(1 << 10);
		}
	}
}