#include "dmac.hpp"
using namespace DMA;

void DMAC::Reset() {
	// clear out channels
	m_Channels = {
		{ ChannelID::VIF0,		Channel {} },
		{ ChannelID::VIF1,		Channel {} },
		{ ChannelID::GIF,		Channel {} },
		{ ChannelID::IPU_FROM,	Channel {} },
		{ ChannelID::IPU_TO,	Channel {} },
		{ ChannelID::SIF0,		Channel {} },
		{ ChannelID::SIF1,		Channel {} },
		{ ChannelID::SIF2,		Channel {} },
	};

	// ps2tek says that SCPH-30001 BIOS expects this to be the reset value
	// we are not emulating the BIOS yet, but when we do, it would be nice
	// to have this already set up.
	m_Regs.enable = 0x1201;
}

void DMAC::Tick() {

}

void DMAC::WriteMemory32(u32 address, u32 word) {
	// channel registers
	if (address < 0x1000e000) {
		ChannelID channel = static_cast<ChannelID>((address >> 8) & 0xff);
		switch (channel) {
			// GIF (PATH3)
			case ChannelID::GIF: {
				debug_log("{:08x} -> GIF", word);
			}

			default: {
				error_log("{:08x} -> unknown channel {:02x}", word, (u8)channel);
			}
		}
	}
}

u32 DMAC::ReadMemory32(u32 address) {
	// channel registers
	if (address < 0x1000e000) {
		u8 channel = (address >> 8) & 0xff;
		switch (channel) {
			default: {
				error_log("unknown channel {:02x}", channel);
			}
		}
	}

	return 0;
}