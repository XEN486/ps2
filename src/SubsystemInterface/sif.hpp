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

	// EE -> IOP
	class SIF1 {
	public:
		void PushFifo(u128 qword) {
			m_FIFO.push((u32)(qword));
			m_FIFO.push((u32)(qword >> 32));
			m_FIFO.push((u32)(qword >> 64));
			m_FIFO.push((u32)(qword >> 96));
		}

		u32 PopFifo() {
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

		SIF1 m_SIF1;
	};
}

namespace IOProcessor::DMA {
	class SIF1 : public IOProcessor::DMA::Channel {
	public:
		SIF1(SubsystemInterface::SIF* sif) {
			m_SIF1 = sif->GetSIF1();
		}

		void Write(u32 word) override {
			error_log("IOP: trying to dma {:08x} -> SIF1", word);
			exit(1);
		}

		u32 Read(u32, u32) {
			return m_SIF1->PopFifo();
		}

	private:
		SubsystemInterface::SIF1* m_SIF1;
	};
}

#endif