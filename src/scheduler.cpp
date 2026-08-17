#include "scheduler.hpp"

void Scheduler::Run() {
	size_t instructions = m_EE->RunOnce();
	//size_t instructions = 1;
	size_t cycles = instructions; // assume 1 cycle per instruction. TODO: maybe make this more accurate?

	// TODO: proper iop timing
	m_IOP->RunOnce();

	// TODO: actually schedule events
	// tick stuff connected to BUSCLK
	for (size_t i = 0; i < (cycles / 2); i++) {
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
		}
	}
}