#ifndef DMAC_DMAC_HPP
#define DMAC_DMAC_HPP

#include "../utils.hpp"
#include <unordered_map>

namespace DMA {
	enum class ChannelID : u8 {
		VIF0		= 0x80,
		VIF1		= 0x90,
		GIF			= 0xa0,
		IPU_FROM	= 0xb0,
		IPU_TO		= 0xb4,
		SIF0		= 0xc0,
		SIF1		= 0xc4,
		SIF2		= 0xc8,
		SPR_FROM	= 0xd0,
		SPR_TO		= 0xd4,
	};

	struct Channel {
		u32 chcr;	// channel control
		u32 madr;	// channel address
		u32 tadr;	// channel tag address
		u32 qwc;	// quadword count
		u32 asr0;	// channel saved tag address 0
		u32 asr1;	// channel saved tag address 1
		u32 sadr;	// channel scratchpad address
	};

	struct DmacRegisters {
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
		DmacRegisters m_Regs;
		std::unordered_map<ChannelID, Channel> m_Channels;
	};
}

#endif