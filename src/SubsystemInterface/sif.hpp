#ifndef SIF_HPP
#define SIF_HPP

#include "../utils.hpp"

namespace SubsystemInterface {
	struct MailboxRegs {
		u32 mscom = 0;
		u32 smcom = 0;
		u32 msflg = 0;
		u32 smflg = 0;
		u32 bd6 = 0;
	};

	class SIF {
	public:
		void Initialize(u8* iop_ram);

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
	};
}

#endif