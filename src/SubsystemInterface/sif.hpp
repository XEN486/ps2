#ifndef SIF_HPP
#define SIF_HPP

#include "../IOP/DMAC/dmac.hpp"
#include "../utils.hpp"

#include <queue>

namespace SubsystemInterface {
	struct MailboxRegs {
		u32 mscom = 0;
		u32 smcom = 0;
		u32 msflg = 0;
		u32 smflg = 0;
		u32 bd6 = 0;
	};

	// IOP -> EE
	class SIF0 {
	public:
		bool DataAvailable() {
			return !m_FIFO.empty();
		}

		void PushFifo(u32 word) {
			m_FIFO.push(word);
		}

		u128 PopFifo() {
			u128 word1 = m_FIFO.front(); m_FIFO.pop();
			u128 word2 = m_FIFO.front(); m_FIFO.pop();
			u128 word3 = m_FIFO.front(); m_FIFO.pop();
			u128 word4 = m_FIFO.front(); m_FIFO.pop();

			return (word4 << 96) | (word3 << 64) | (word2 << 32) | word1;
		}

	private:
		std::queue<u32> m_FIFO;
	};

	// EE -> IOP
	class SIF1 {
	public:
		bool DataAvailable() {
			return !m_FIFO.empty();
		}

		void PushFifo(u128 qword) {
			m_FIFO.push((u32)(qword));
			m_FIFO.push((u32)(qword >> 32));
			m_FIFO.push((u32)(qword >> 64));
			m_FIFO.push((u32)(qword >> 96));
		}

		u32 PopFifo() {
			if (m_FIFO.empty()) {
				return 0;
			}

			u32 v = m_FIFO.front();
			m_FIFO.pop();
			return v;
		}

	private:
		std::queue<u32> m_FIFO;	
	};

	class SIF {
	public:
		void Initialize(u8* iop_ram);

		SIF0* GetSIF0() { return &m_SIF0; }
		SIF1* GetSIF1() { return &m_SIF1; }

	// EE functions
	public:
		// 32-bit bus to IOP RAM
		u32 EE_ReadIOPMemory(u32 address);
		void EE_WriteIOPMemory(u32 address, u32 word);

		// SIF mailbox hw registers
		u32 EE_ReadMailbox(u32 address);
		void EE_WriteMailbox(u32 address, u32 word);

	// IOP functions
	public:
		u32 IOP_ReadMailbox(u32 address);
		void IOP_WriteMailbox(u32 address, u32 word);

	private:
		MailboxRegs m_MailboxRegs;
		u8* m_IopRam;

		SIF0 m_SIF0;
		SIF1 m_SIF1;
	};
}

#endif