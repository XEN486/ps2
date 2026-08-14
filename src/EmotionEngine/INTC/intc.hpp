#ifndef EMOTIONENGINE_INTC_HPP
#define EMOTIONENGINE_INTC_HPP

#include "../../utils.hpp"

namespace EmotionEngine { class EE; }
namespace EmotionEngine::Interrupt {
	enum class IRQ : u8 {
		GS,
		SBUS,
		VBlankStart,
		VBlankEnd,
		VIF0,
		VIF1,
		VU0,
		VU1,
		IPU,
		Timer0,
		Timer1,
		Timer2,
		Timer3,
		SFIFO,
		VU0Watchdog,
	};

	class INTC {
	public:
		INTC(EmotionEngine::EE* ee) : m_EE(ee) {}

		void Reset() {
			m_STAT = 0;
			m_MASK = 0;
		}

		void Interrupt(IRQ irq);

		// NOT the same as IOP INTC
		void SetSTAT(u32 word) {
			m_STAT &= ~word;
			TryInterrupt();
		}

		// NOT the same as IOP INTC
		void SetMASK(u32 word) {
			m_MASK ^= word;
			TryInterrupt();
		}

		u32 GetSTAT() const { return m_STAT; }
		u32 GetMASK() const { return m_MASK; }

	private:
		void TryInterrupt();

	private:
		EmotionEngine::EE* m_EE;
		u32 m_STAT = 0;
		u32 m_MASK = 0;
	};
}

#endif