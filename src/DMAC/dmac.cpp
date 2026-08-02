#include "dmac.hpp"
#include <cassert>

using namespace DMA;

void DMAC::Reset() {
	// clear out channels
	m_Channels = {
		Channel { .id = ChannelID::VIF0 },
		Channel { .id = ChannelID::VIF1 },
		Channel { .id = ChannelID::GIF },
		Channel { .id = ChannelID::IPU_FROM },
		Channel { .id = ChannelID::IPU_TO },
		Channel { .id = ChannelID::SIF0 },
		Channel { .id = ChannelID::SIF1 },
		Channel { .id = ChannelID::SIF2 },
		Channel { .id = ChannelID::SPR_FROM },
		Channel { .id = ChannelID::SPR_TO },
	};

	// ps2tek says that SCPH-30001 BIOS expects this to be the reset value
	// we are not emulating the BIOS yet, but when we do, it would be nice
	// to have this already set up.
	m_Regs.enable = 0x1201;

	// not in transfer
	m_InTransfer = false;
}

void DMAC::Tick() {
	// dont try do a transfer if the DMAC is disabled or if DMA is disabled
	if ((m_Regs.enable & (1 << 16)) || (m_Regs.ctrl & 1) == 0) {
		return;
	}

	if (m_InTransfer) {
		DoTransfer();
		return;
	}

	for (u8 id = 0; id < 10; id++) {
		auto& channel = m_Channels.channels[id];
		bool condition;

		// check only enabled channels if priority is enabled
		if (m_Regs.pcr & 0x80000000) {
			condition = ((m_Regs.pcr & (1u << (16 + static_cast<u8>(id)))) && (channel.chcr & CHCRBits::STR));
		}
		
		// check all channels if priority is enabled
		else {
			condition = channel.chcr & CHCRBits::STR;
		}

		if (condition) {
			debug_log("{}: begin transfer", channel.id);
			m_InTransfer = true;
			m_TransferChannel = &channel;
			return;
		}
	}
}

void DMAC::WriteMemory32(u32 address, u32 word) {
	// channel registers
	if (address < 0x1000e000) {
		ChannelID channel = GetChannelFromAddress((address >> 8) & 0xff);
		WriteToChannel(channel, address, word);
		return;
	}

	// dmac registers
	WriteToReg(address, word);
}

u32 DMAC::ReadMemory32(u32 address) {
	// channel registers
	if (address < 0x1000e000) {
		ChannelID channel = GetChannelFromAddress((address >> 8) & 0xff);
		return ReadFromChannel(channel, address);
	}

	// dmac registers
	return ReadFromReg(address);
}

void DMAC::WriteToChannel(ChannelID channel, u32 address, u32 word) {
	ChannelReg reg = static_cast<ChannelReg>(address & 0xff);
	debug_log("write {:08x} -> {}({})", word, channel, reg);

	switch (reg) {
		case ChannelReg::CHCR:	{ m_Channels.channels[channel].chcr = word; return; }
		case ChannelReg::MADR:	{ m_Channels.channels[channel].madr = word; return; }
		case ChannelReg::TADR:	{ m_Channels.channels[channel].tadr = word; return; }
		case ChannelReg::QWC:	{ m_Channels.channels[channel].qwc = word; return; }
		case ChannelReg::ASR0:	{ m_Channels.channels[channel].asr0 = word; return; }
		case ChannelReg::ASR1:	{ m_Channels.channels[channel].asr1 = word; return; }
		case ChannelReg::SADR:	{ m_Channels.channels[channel].sadr = word; return; }
	}

	std::unreachable();
}

u32 DMAC::ReadFromChannel(ChannelID channel, u32 address) {
	ChannelReg reg = static_cast<ChannelReg>(address & 0xff);
	//debug_log("read from {}({})", channel, reg);

	switch (reg) {
		case ChannelReg::CHCR:	return m_Channels.channels[channel].chcr;
		case ChannelReg::MADR:	return m_Channels.channels[channel].madr;
		case ChannelReg::TADR:	return m_Channels.channels[channel].tadr;
		case ChannelReg::QWC:	return m_Channels.channels[channel].qwc;
		case ChannelReg::ASR0:	return m_Channels.channels[channel].asr0;
		case ChannelReg::ASR1:	return m_Channels.channels[channel].asr1;
		case ChannelReg::SADR:	return m_Channels.channels[channel].sadr;
	}

	std::unreachable();
}

void DMAC::WriteToReg(u32 address, u32 word) {
	DmacReg reg = static_cast<DmacReg>(address & 0xffff);
	debug_log("write {:08x} -> D_{}", word, reg);

	switch (reg) {
		case DmacReg::CTRL:		{ m_Regs.ctrl = word; return; }
		case DmacReg::STAT:		{ m_Regs.stat = word; return; }
		case DmacReg::PCR:		{ m_Regs.pcr = word; return; }
		case DmacReg::SQWC:		{ m_Regs.sqwc = word; return; }
		case DmacReg::RBSR:		{ m_Regs.rbsr = word; return; }
		case DmacReg::RBOR:		{ m_Regs.rbor = word; return; }
		case DmacReg::ENABLEW:	{ m_Regs.enable = word; return; }
	}

	debug_log("{:08x} -> unknown address {:08x}", word, address);
}

u32 DMAC::ReadFromReg(u32 address) {
	DmacReg reg = static_cast<DmacReg>(address & 0xffff);
	debug_log("read <- D_{}", reg);

	switch (reg) {
		case DmacReg::CTRL:		{ return m_Regs.ctrl; }
		case DmacReg::STAT:		{ return m_Regs.stat; }
		case DmacReg::PCR:		{ return m_Regs.pcr; }
		case DmacReg::SQWC:		{ return m_Regs.sqwc; }
		case DmacReg::RBSR:		{ return m_Regs.rbsr; }
		case DmacReg::RBOR:		{ return m_Regs.rbor; }
		case DmacReg::ENABLER:	{ return m_Regs.enable; }
	}

	debug_log("unknown address {:08x}", address);
	return 0;
}

ChannelID DMAC::GetChannelFromAddress(u8 addr) {
	switch (addr) {
		case 0x80: return ChannelID::VIF0;
		case 0x90: return ChannelID::VIF1;
		case 0xa0: return ChannelID::GIF;
		case 0xb0: return ChannelID::IPU_FROM;
		case 0xb4: return ChannelID::IPU_TO;
		case 0xc0: return ChannelID::SIF0;
		case 0xc4: return ChannelID::SIF1;
		case 0xc8: return ChannelID::SIF2;
		case 0xd0: return ChannelID::SPR_FROM;
		case 0xd4: return ChannelID::SPR_TO;
	}

	std::unreachable();
}

void DMAC::DoTransfer() {
	// only support normal transfer for now
	assert(((m_TransferChannel->chcr & CHCRBits::MOD) >> 2) == 0);

	switch (m_TransferChannel->id) {
		default: {
			error_log("unimplemented transfer for {} channel", m_TransferChannel->id);
			exit(1);
		}
	}
}