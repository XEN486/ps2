#include "dmac.hpp"
using namespace IOProcessor::DMA;

void DMAC::Reset() {
	// clear out channels
	memset(m_Channels.channels, 0, sizeof(m_Channels.channels));
	m_Channels = {
		Channel { .id = ChannelID::MDECin },
		Channel { .id = ChannelID::MDECout },
		Channel { .id = ChannelID::SIF2 },
		Channel { .id = ChannelID::CDVD },
		Channel { .id = ChannelID::SPU1 },
		Channel { .id = ChannelID::PIO },
		Channel { .id = ChannelID::OTC },
		Channel { .id = ChannelID::SPU2 },
		Channel { .id = ChannelID::DEV9 },
		Channel { .id = ChannelID::SIF0 },
		Channel { .id = ChannelID::SIF1 },
		Channel { .id = ChannelID::SIO2in },
		Channel { .id = ChannelID::SIO2out },
	};

	// clear out registers
	memset(&m_Regs, 0, sizeof(m_Regs));

	// not in transfer
	m_InTransfer = false;
}

void DMAC::Tick() {
	// dont try do a transfer if the DMAC is disabled
	if (!m_Regs.dmacen) return;

	if (m_InTransfer) {
		DoTransfer();
		return;
	}

	for (u8 id = 0; id < 13; id++) {
		auto& channel = m_Channels.channels[id];
		bool condition = false; // TODO: how to calculate this?

		if (condition) {
			m_InTransfer = true;
			m_TransferChannel = &channel;
			return;
		}
	}
}

void DMAC::Write(u32 address, u32 word) {
	// channel registers
	if ((address >= 0x1f801080 && address <= 0x1f8010ef) || (address >= 0x1f801500 && address <= 0x1f80155f)) {
		ChannelID channel = GetChannelFromAddress((address >> 4) & 0xfff);
		WriteToChannel(channel, address, word);
	}

	// dmac registers
	else if ((address >= 0x1f8010f0 && address <= 0x1f8010f4) || (address >= 0x1f801570 && address <= 0x1f80157c)) {
		WriteToReg(address, word);
	}
}

u32 DMAC::Read(u32 address) {
	// channel registers
	if (address < 0x1000e000) {
		ChannelID channel = GetChannelFromAddress((address >> 4) & 0xfff);
		return ReadFromChannel(channel, address);
	}

	// dmac registers
	return ReadFromReg(address);
}

void DMAC::WriteToChannel(ChannelID ch, u32 address, u32 word) {
	u8 channel = static_cast<u8>(ch);
	ChannelReg reg = static_cast<ChannelReg>(address & 0xf);
	//debug_log("write {:08x} -> {}({})", word, channel, reg);

	switch (reg) {
		case ChannelReg::MADR:	{ m_Channels.channels[channel].madr = word; return; }
		case ChannelReg::BCR:	{ m_Channels.channels[channel].bcr = word; return; }
		case ChannelReg::CHCR:	{ m_Channels.channels[channel].chcr = word; return; }
		case ChannelReg::TADR:	{ m_Channels.channels[channel].tadr = word; return; }
	}

	std::unreachable();
}

u32 DMAC::ReadFromChannel(ChannelID ch, u32 address) {
	u8 channel = static_cast<u8>(ch);
	ChannelReg reg = static_cast<ChannelReg>(address & 0xf);
	//debug_log("read from {}({})", channel, reg);

	switch (reg) {
		case ChannelReg::MADR:	return m_Channels.channels[channel].madr;
		case ChannelReg::BCR:	return m_Channels.channels[channel].bcr;
		case ChannelReg::CHCR:	return m_Channels.channels[channel].chcr;
		case ChannelReg::TADR:	return m_Channels.channels[channel].tadr;
	}

	std::unreachable();
}

void DMAC::WriteToReg(u32 address, u32 word) {
	DmacReg reg = static_cast<DmacReg>(address & 0xffff);

	switch (reg) {
		case DmacReg::DPCR:			{ m_Regs.dpcr = word; return; }
		case DmacReg::DPCR2:		{ m_Regs.dpcr2 = word; return; }
		case DmacReg::DICR:			{ m_Regs.dicr.Write(word); return; }
		case DmacReg::DICR2:		{ m_Regs.dicr2.Write(word); return; }
		case DmacReg::DMACEN:		{ m_Regs.dmacen = word & 1; return; }
		case DmacReg::DMACINTEN:	{ m_Regs.dmacinten = word & 1; return; }
	}

	debug_log("{:08x} -> unknown address {:08x}", word, address);
}

u32 DMAC::ReadFromReg(u32 address) {
	DmacReg reg = static_cast<DmacReg>(address & 0xffff);

	switch (reg) {
		case DmacReg::DPCR:			{ return m_Regs.dpcr; }
		case DmacReg::DPCR2:		{ return m_Regs.dpcr2; }
		case DmacReg::DICR:			{ return m_Regs.dicr.Read(); }
		case DmacReg::DICR2:		{ return m_Regs.dicr2.Read(); }
		case DmacReg::DMACEN:		{ return m_Regs.dmacen; }
		case DmacReg::DMACINTEN:	{ return m_Regs.dmacinten; }
	}

	debug_log("unknown address {:08x}", address);
	return 0;
}

ChannelID DMAC::GetChannelFromAddress(u16 addr) {
	switch (addr) {
		// old channels
		case 0x108: return ChannelID::MDECin;
		case 0x109: return ChannelID::MDECout;
		case 0x10a: return ChannelID::SIF2;
		case 0x10b: return ChannelID::CDVD;
		case 0x10c: return ChannelID::SPU1;
		case 0x10d: return ChannelID::PIO;
		case 0x10e: return ChannelID::OTC;

		// new channels
		case 0x150: return ChannelID::SPU2;
		case 0x151: return ChannelID::DEV9;
		case 0x152: return ChannelID::SIF0;
		case 0x153: return ChannelID::SIF1;
		case 0x154: return ChannelID::SIO2in;
		case 0x155: return ChannelID::SIO2out;
	}

	std::unreachable();
}

void DMAC::DoTransfer() {
	error_log("unimplemented");
	exit(1);
}

void DMAC::SendWord(u32 word) {
	switch (m_TransferChannel->id) {
		default: {
			error_log("unimplemented transfer for {} channel", m_TransferChannel->id);
			exit(1);
		}
	}
}

void DMAC::RaiseInterrupt(ChannelID channel) {
	// old channels
	if (channel <= ChannelID::OTC) {
		u8 bit = (1 << static_cast<u8>(channel));
		if (m_Regs.dicr.channel_int_mask & bit) {
			m_Regs.dicr.channel_int_flags |= bit;
			m_Regs.dicr.RecalculateMIF();
		}

		return;
	}

	// new channels
	u8 bit = (1 << (static_cast<u8>(channel) - 7));
	if (m_Regs.dicr2.channel_int_mask & bit) {
		m_Regs.dicr2.channel_int_flags |= bit;
		m_Regs.dicr.RecalculateMIF();
	}
}

void DMAC::FinishTransfer() {
	m_TransferChannel->chcr &= ~static_cast<u32>(CHCRBits::StartTransfer);
	m_TransferChannel->chcr &= ~static_cast<u32>(CHCRBits::ForceStartTransfer);
}