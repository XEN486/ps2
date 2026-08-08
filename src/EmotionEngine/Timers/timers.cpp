#include "timers.hpp"
using namespace EmotionEngine::Timers;

void Timer::Tick() {
	m_Cycles++;

	switch (m_Mode.clock) {
		case ClockType::BUSCLK: {
			Increment();
			break;
		}

		case ClockType::BUSCLK_16: {
			if ((m_Cycles % 16) == 0) {
				Increment();
			}

			break;
		}

		case ClockType::BUSCLK_256: {
			if ((m_Cycles % 256) == 0) {
				Increment();
			}

			break;
		}
	}
}

void Timer::NotifyHBlank(bool start) {
	if (m_Mode.gate_type == GateType::HBlank && m_Mode.timer_enable && m_Mode.gate_enable) {
		switch (m_Mode.gate_mode) {
			// force disable while high
			case GateMode::CountWhileLow: {
				m_Disable = start;
				break;
			}

			// reset on either transition
			case GateMode::ResetOnTransition: {
				m_Count = 0;
				break;
			}

			// reset on transition from low to high
			case GateMode::ResetOnLowToHigh: {
				if (start) {
					m_Count = 0;
				}

				break;
			}

			// reset on transition from high to low
			case GateMode::ResetOnHighToLow: {
				if (!start) {
					m_Count = 0;
				}

				break;
			}
		}
	}

	if (m_Mode.clock == ClockType::HBlank) {
		Increment();
	}
}

void Timer::NotifyVBlank(bool start) {
	if (m_Mode.gate_type == GateType::VBlank && m_Mode.timer_enable && m_Mode.gate_enable) {
		switch (m_Mode.gate_mode) {
			// force disable while high
			case GateMode::CountWhileLow: {
				m_Disable = start;
				break;
			}

			// reset on either transition
			case GateMode::ResetOnTransition: {
				m_Count = 0;
				break;
			}

			// reset on transition from low to high
			case GateMode::ResetOnLowToHigh: {
				if (start) {
					m_Count = 0;
				}

				break;
			}

			// reset on transition from high to low
			case GateMode::ResetOnHighToLow: {
				if (!start) {
					m_Count = 0;
				}

				break;
			}
		}
	}
}

void Timer::Write(u32 addr, u32 val) {
	u8 lo = addr & 0xff;
	switch (lo) {
		// Tn_COUNT
		case 0x00: {
			m_Count = val & 0xffff;
			break;
		}

		// Tn_MODE
		case 0x10: {
			m_Mode.clock						= static_cast<ClockType>(val & 0b11);
			m_Mode.gate_enable					= (val >> 2) & 1;
			m_Mode.gate_type					= static_cast<GateType>((val >> 3) & 1);
			m_Mode.gate_mode					= static_cast<GateMode>((val >> 4) & 0b11);
			m_Mode.clear_on_compare				= (val >> 6) & 1;
			m_Mode.timer_enable					= (val >> 7) & 1;
			m_Mode.compare_interrupt_enable		= (val >> 8) & 1;
			m_Mode.overflow_interrupt_enable	= (val >> 9) & 1;
			m_Mode.compare_interrupt_flag		&= (~(val >> 10)) & 1;
			m_Mode.overflow_interrupt_flag		&= (~(val >> 11)) & 1;

			break;
		}

		// Tn_COMP
		case 0x20: {
			m_Compare = val & 0xffff;
			break;
		}

		// Tn_HOLD
		case 0x30: {
			m_Hold = val & 0xffff;
			break;	
		}
	}
}

u32 Timer::Read(u32 addr) {
	u8 lo = addr & 0xff;
	switch (lo) {
		// Tn_COUNT
		case 0x00: { return m_Count; }

		// Tn_MODE
		case 0x10: {
			u16 val = 0;
			val |= static_cast<u8>(m_Mode.clock);
			val |= m_Mode.gate_enable << 2;
			val |= static_cast<u8>(m_Mode.gate_type) << 3;
			val |= static_cast<u8>(m_Mode.gate_mode) << 4;
			val |= m_Mode.clear_on_compare << 6;
			val |= m_Mode.timer_enable << 7;
			val |= m_Mode.compare_interrupt_enable << 8;
			val |= m_Mode.overflow_interrupt_enable << 9;
			val |= m_Mode.compare_interrupt_flag << 10;
			val |= m_Mode.overflow_interrupt_flag << 11;
			return val;
		}

		// Tn_COMP
		case 0x20: {
			return m_Compare;
		}

		// Tn_HOLD
		case 0x30: {
			return m_Hold;
		}
	}

	error_log("unknown timer address {:08x}", addr);
	return 0;
}

void Timer::Increment() {
	if (!m_Mode.timer_enable || m_Disable) return;
	m_Count++;
	//debug_log("{} {} {} {}", m_Count, m_Compare, m_Mode.compare_interrupt_enable, m_Mode.overflow_interrupt_enable);

	if (m_Count == m_Compare) {
		CompareInterrupt();
		if (m_Mode.clear_on_compare) {
			m_Count = 0;
		}
	}

	if (m_Count == 0) {
		OverflowInterrupt();
	}
}

void Timer::CompareInterrupt() {
	if (m_Mode.compare_interrupt_enable) {
		debug_log("compare interrupt");
		// intc->interrupt(compare)
		m_Mode.compare_interrupt_flag = true;
	}
}

void Timer::OverflowInterrupt() {
	if (m_Mode.overflow_interrupt_enable) {
		debug_log("overflow interrupt");
		// intc->interrupt(overflow)
		m_Mode.overflow_interrupt_flag = true;
	}
}