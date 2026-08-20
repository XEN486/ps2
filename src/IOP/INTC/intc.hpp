#ifndef IOP_INTC_HPP
#define IOP_INTC_HPP

#include "../../utils.hpp"

namespace IOProcessor { class IOP; }
namespace IOProcessor::Interrupt {
	enum class IRQ : u8 {
		VBlankStart,
		GPU,
		CDVD,
		DMA,
		Timer0,
		Timer1,
		Timer2,
		SIO0,
		SIO1,
		SPU2,
		PIO,
		VBlankEnd,
		DVD,
		PCMCIA,
		Timer3,
		Timer4,
		Timer5,
		SIO2,
		HTR0,
		HTR1,
		HTR2,
		HTR3,
		USB,
		EXTR,
		FWRE,
		FDMA
	};

	class IOP;
	class INTC {
	public:
		void Initialize(IOProcessor::IOP* iop) {
			m_IOP = iop;
		}

		void Reset() {
			m_STAT = 0;
			m_MASK = 0;
		}

		/// @brief Sends an IRQ to the IOP.
		void Interrupt(IRQ irq);

		/// @brief Writes to I_CTRL.
		/// @param word Word to write to I_CTRL.
		void SetCTRL(u32 word) {
			m_CTRL = word & 1;
			TryInterrupt();
		}

		/// @brief Writes to I_STAT.
		/// @param word Word to write to I_STAT.
		void SetSTAT(u32 word) {
			m_STAT &= word; // bit=0 -> clear, bit=1 -> no change
			TryInterrupt();
		}

		/// @brief Writes to I_MASK.
		/// @param word Word to write to I_MASK.
		void SetMASK(u32 word) {
			m_MASK = word;
			TryInterrupt();
		}

		u32 GetCTRL() {
			bool value = m_CTRL;
			m_CTRL = false;
			return value;
		}

		u32 GetSTAT() const { return m_STAT; }
		u32 GetMASK() const { return m_MASK; }

	private:
		void TryInterrupt();

	private:
		IOProcessor::IOP* m_IOP;
		u32 m_STAT = 0;
		u32 m_MASK = 0;
		bool m_CTRL = false;
	};
}

#endif