#include "intc.hpp"
#include "../emotion.hpp"
using namespace EmotionEngine::Interrupt;

void INTC::Interrupt(IRQ irq) {
	// set bit in I_STAT
	m_STAT |= (1 << static_cast<u8>(irq));
	TryInterrupt();
}

void INTC::TryInterrupt() {
	for (u8 i = 0; i <= 10; i++) {
		u32 bit = (1 << i);

		if ((m_STAT & bit) && (m_MASK & bit)) {
			// Cause.10 set (INT0)
			m_EE->GetR5900().cop0.cause |= (1 << 10);
			return;
		}
		
		// Cause.10 clear (INT0)
		else {
			m_EE->GetR5900().cop0.cause &= ~(u32)(1 << 10);
		}
	}
}