#ifndef EMOTIONENGINE_TIMERS_HPP
#define EMOTIONENGINE_TIMERS_HPP

#include "../../utils.hpp"

/// @brief The EmotionEngine's 16-bit timers.
namespace EmotionEngine::Timers {
	enum class ClockType {
		BUSCLK,
		BUSCLK_16,
		BUSCLK_256,
		HBlank,
	};

	enum class GateType {
		HBlank,
		VBlank
	};

	enum class GateMode {
		CountWhileLow,
		ResetOnLowToHigh,
		ResetOnHighToLow,
		ResetOnTransition,
	};

	struct TimerMode {
		ClockType clock;
		bool gate_enable;
		GateType gate_type;
		GateMode gate_mode;
		bool clear_on_compare;
		bool timer_enable;

		bool compare_interrupt_enable;
		bool overflow_interrupt_enable;

		bool compare_interrupt_flag;
		bool overflow_interrupt_flag;
	};

	/// @brief The EmotionEngine has 4 of these timers to keep track of time.
	class Timer {
	public:
		void Tick();
		void NotifyHBlank(bool start);
		void NotifyVBlank(bool start);

		void Write(u32 addr, u32 val);
		u32 Read(u32 addr);

	private:
		void Increment();

		void CompareInterrupt();
		void OverflowInterrupt();

		void SetTimerInterrupt() {
			// set irq number here
		}

	private:
		u16 m_Count = 0;
		u16 m_Compare = 0;
		u16 m_Hold;

		TimerMode m_Mode = {};
		bool m_Disable = false;

		// BUSCLK cycle count
		size_t m_Cycles = 0;
	
	private:
		friend class Timers;
	};

	/// @brief Structure holding the 4 EE timers.
	class Timers {
	public:
		Timers() {
			m_Timers[0].SetTimerInterrupt();
			m_Timers[1].SetTimerInterrupt();
			m_Timers[2].SetTimerInterrupt();
			m_Timers[3].SetTimerInterrupt();
		}

		Timer* GetTimers() {
			return m_Timers;
		}
		
		void Tick() {
			m_Timers[0].Tick();
			m_Timers[1].Tick();
			m_Timers[2].Tick();
			m_Timers[3].Tick();
		}

		void NotifyHBlank(bool start) {
			m_Timers[0].NotifyHBlank(start);
			m_Timers[1].NotifyHBlank(start);
			m_Timers[2].NotifyHBlank(start);
			m_Timers[3].NotifyHBlank(start);
		}

		void NotifyVBlank(bool start) {
			m_Timers[0].NotifyVBlank(start);
			m_Timers[1].NotifyVBlank(start);
			m_Timers[2].NotifyVBlank(start);
			m_Timers[3].NotifyVBlank(start);
		}

		void Write(u32 addr, u32 val) {
			m_Timers[((addr >> 8) & 0xff) / 8].Write(addr, val);
		}

		u32 Read(u32 addr) {
			return m_Timers[((addr >> 8) & 0xff) / 8].Read(addr);
		}

	private:
		Timer m_Timers[4];
	};
}

#endif