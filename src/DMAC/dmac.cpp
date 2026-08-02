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
		WriteToChannel(channel, address, word);
		return;
	}
}

u32 DMAC::ReadMemory32(u32 address) {
	// channel registers
	if (address < 0x1000e000) {
		ChannelID channel = static_cast<ChannelID>((address >> 8) & 0xff);
		return ReadFromChannel(channel, address);
	}

	return 0;
}

void DMAC::WriteToChannel(ChannelID channel, u32 address, u32 word) {
	ChannelReg reg = static_cast<ChannelReg>(address & 0xff);
	debug_log("write {:08x} -> {}({})", word, channel, reg);

	switch (reg) {
		case ChannelReg::CHCR:	{ m_Channels[channel].chcr = word; return; }
		case ChannelReg::MADR:	{ m_Channels[channel].madr = word; return; }
		case ChannelReg::TADR:	{ m_Channels[channel].tadr = word; return; }
		case ChannelReg::QWC:	{ m_Channels[channel].qwc = word; return; }
		case ChannelReg::ASR0:	{ m_Channels[channel].asr0 = word; return; }
		case ChannelReg::ASR1:	{ m_Channels[channel].asr1 = word; return; }
		case ChannelReg::SADR:	{ m_Channels[channel].sadr = word; return; }
	}

	std::unreachable();
}

u32 DMAC::ReadFromChannel(ChannelID channel, u32 address) {
	ChannelReg reg = static_cast<ChannelReg>(address & 0xff);
	debug_log("read from {}({})", channel, reg);

	switch (reg) {
		case ChannelReg::CHCR:	return m_Channels[channel].chcr;
		case ChannelReg::MADR:	return m_Channels[channel].madr;
		case ChannelReg::TADR:	return m_Channels[channel].tadr;
		case ChannelReg::QWC:	return m_Channels[channel].qwc;
		case ChannelReg::ASR0:	return m_Channels[channel].asr0;
		case ChannelReg::ASR1:	return m_Channels[channel].asr1;
		case ChannelReg::SADR:	return m_Channels[channel].sadr;
	}

	std::unreachable();
}