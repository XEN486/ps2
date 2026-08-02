#ifndef DMAC_DMAC_HPP
#define DMAC_DMAC_HPP

#include "../utils.hpp"

namespace DMA {
	enum ChannelID : u8 {
		VIF0,
		VIF1,
		GIF,
		IPU_FROM,
		IPU_TO,
		SIF0,
		SIF1,
		SIF2,
		SPR_FROM,
		SPR_TO,
	};

	enum class ChannelReg : u8 {
		CHCR	= 0x00,
		MADR	= 0x10,
		TADR	= 0x30,
		QWC		= 0x20,
		ASR0	= 0x40,
		ASR1	= 0x50,
		SADR	= 0x80,
	};

	enum class DmacReg : u16 {
		CTRL	= 0xe000,
		STAT	= 0xe010,
		PCR		= 0xe020,
		SQWC	= 0xe030,
		RBSR	= 0xe040,
		RBOR	= 0xe050,
		ENABLER	= 0xf520,
		ENABLEW	= 0xf590,
	};

	// lo 9-bit value of Dn_CHCR
	enum CHCRBits : u16 {
		DIR		= 0b000000001,
		MOD		= 0b000001100,
		ASP		= 0b000110000,
		TTE		= 0b001000000,
		TIE		= 0b010000000,
		STR		= 0b100000000,
	};

	struct Channel {
		ChannelID id;
		u32 chcr;		// channel control
		u32 madr;		// channel address
		u32 tadr;		// channel tag address
		u32 qwc;		// quadword count
		u32 asr0;		// channel saved tag address 0
		u32 asr1;		// channel saved tag address 1
		u32 sadr;		// channel scratchpad address
	};

	struct Channels {
		Channel channels[10];
	};

	struct DmacRegisters {
		u32 ctrl;
		u32 stat;
		u32 pcr;
		u32 sqwc;
		u32 rbsr;
		u32 rbor;
		u32 enable;
	};

	class DMAC {
	public:
		void Reset();
		void Tick();

		void WriteMemory32(u32 address, u32 word);
		u32 ReadMemory32(u32 address);

	private:
		void WriteToChannel(ChannelID channel, u32 address, u32 word);
		u32 ReadFromChannel(ChannelID channel, u32 address);

		void WriteToReg(u32 address, u32 word);
		u32 ReadFromReg(u32 address);

		ChannelID GetChannelFromAddress(u8 addr);

		void DoTransfer();

	private:
		DmacRegisters m_Regs;
		Channels m_Channels;

		bool m_InTransfer;
		Channel* m_TransferChannel;
	};
}

#include "dmac.inl"
#endif