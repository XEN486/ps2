#include "intc.hpp"
#include "../emotion.hpp"
using namespace EmotionEngine::Interrupt;

void INTC::Interrupt(IRQ irq) {
	// set bit in I_STAT
	m_STAT |= (1 << static_cast<u8>(irq));
	TryInterrupt();
}

void INTC::TryInterrupt() {
	if (m_STAT & m_MASK) {
		m_EE->GetR5900().cop0.cause |= (1 << 10);
	} else {
		m_EE->GetR5900().cop0.cause &= ~(1 << 10);
	}
}