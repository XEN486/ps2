#include "scheduler.hpp"

#define EE_INSTRUCTIONS 20

void Scheduler::Run() {
	// tick EE
	size_t ee_cycles = 0;
	while (ee_cycles < EE_INSTRUCTIONS) {
		ee_cycles += m_EE->RunOnce();
	}

	// TODO: actually schedule events
	// tick stuff connected to BUSCLK
	for (size_t i = 0; i < (ee_cycles / 2); i++) {
		m_EE->GetTimers().Tick();
		m_EE->GetDMAC().Tick();
		m_EE->GetGIF().ProcessQword();
		m_GS->Tick();

		// enter hblank
		if (m_GS->GetEnteredHBlank()) {
			// notify EE timers about entering hblank
			m_EE->GetTimers().NotifyHBlank(true);
		}

		// enter vblank
		m_FrameReady = m_GS->GetEnteredVBlank();
		if (m_FrameReady) {
			// notify EE timers about entering vblank
			m_EE->GetTimers().NotifyVBlank(true);
			m_EE->GetINTC().Interrupt(EmotionEngine::Interrupt::IRQ::VBlankStart);
			m_IOP->GetINTC().Interrupt(IOProcessor::Interrupt::IRQ::VBlankStart);
		}

		// leave hblank
		if (m_GS->GetLeftHBlank()) {
			// notify EE timers about leaving hblank
			m_EE->GetTimers().NotifyHBlank(false);
		}

		// leave vblank
		m_FrameReady = m_GS->GetEnteredVBlank();
		if (m_GS->GetLeftVBlank()) {
			// notify EE timers about leaving vblank
			m_EE->GetTimers().NotifyVBlank(false);
			m_EE->GetINTC().Interrupt(EmotionEngine::Interrupt::IRQ::VBlankEnd);
			m_IOP->GetINTC().Interrupt(IOProcessor::Interrupt::IRQ::VBlankEnd);
		}
	}

	// tick IOP
	// TODO: proper iop timing
	size_t iop_cycles = 0;
	while (iop_cycles < ee_cycles/4) {
		iop_cycles += m_IOP->RunOnce();
	}
}