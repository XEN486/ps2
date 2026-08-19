#include "dmac.hpp"
#include "../iop.hpp"
#include "../../SubsystemInterface/sif.hpp"
#include <cassert>

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
	m_Regs.dicr.iop = m_IOP;
	m_Regs.dicr.icr2 = &m_Regs.dicr2;
	m_Regs.dicr.dmacinten = &m_Regs.dmacinten;

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

	// try find the highest priority channel that needs a transfer and is enabled
	u8 highest_priority = 7; // 0=highest, 7=lowest
	Channel* channel = nullptr;
	for (u8 id = 0; id < 13; id++) {
		u8 priority = GetChannelPriority(static_cast<ChannelID>(id));
		if (!(priority & 0b1000)) continue; // not enabled
		if (!(m_Channels.channels[id].chcr & static_cast<u32>(CHCRBits::StartTransfer))) continue;

		// only lo 3-bits are used for priority
		priority &= 0b111;
		if (priority <= highest_priority) {
			highest_priority = priority;
			channel = &m_Channels.channels[id];
		}
	}

	// if there was a channel found then start a transfer
	if (channel) {
		m_TransferChannel = channel;
		m_InTransfer = true;
		DoTransfer();
	}
}

void DMAC::Write(u32 address, u32 word) {
	// channel registers
	if ((address >= 0x1f801080 && address <= 0x1f8010ef) || (address >= 0x1f801500 && address <= 0x1f80155f)) {
		ChannelID channel = GetChannelFromAddress((address >> 4) & 0xff);
		return WriteToChannel(channel, address, word);
	}

	// dmac registers
	else if ((address >= 0x1f8010f0 && address <= 0x1f8010f4) || (address >= 0x1f801570 && address <= 0x1f80157c)) {
		return WriteToReg(address, word);
	}
}

u32 DMAC::Read(u32 address) {
	// channel registers
	if ((address >= 0x1f801080 && address <= 0x1f8010ef) || (address >= 0x1f801500 && address <= 0x1f80155f)) {
		ChannelID channel = GetChannelFromAddress((address >> 4) & 0xff);
		return ReadFromChannel(channel, address);
	}

	// dmac registers
	else if ((address >= 0x1f8010f0 && address <= 0x1f8010f4) || (address >= 0x1f801570 && address <= 0x1f80157c)) {
		return ReadFromReg(address);
	}
	
	return 0;
}

void DMAC::WriteToChannel(ChannelID ch, u32 address, u32 word) {
	u8 channel = static_cast<u8>(ch);
	assert(channel < 13);

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
	assert(channel < 13);

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
		case DmacReg::DMACINTEN:	{ m_Regs.dmacinten = word & 0b11; return; }
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
		case 0x08: return ChannelID::MDECin;
		case 0x09: return ChannelID::MDECout;
		case 0x0a: return ChannelID::SIF2;
		case 0x0b: return ChannelID::CDVD;
		case 0x0c: return ChannelID::SPU1;
		case 0x0d: return ChannelID::PIO;
		case 0x0e: return ChannelID::OTC;

		// new channels
		case 0x50: return ChannelID::SPU2;
		case 0x51: return ChannelID::DEV9;
		case 0x52: return ChannelID::SIF0;
		case 0x53: return ChannelID::SIF1;
		case 0x54: return ChannelID::SIO2in;
		case 0x55: return ChannelID::SIO2out;
	}

	std::unreachable();
}

void DMAC::DoTransfer() {
	switch (m_TransferChannel->id) {
		case ChannelID::SIF1: {
			if (!m_IOP->GetSIF()->GetSIF1()->DataAvailable()) {
				FinishTransfer();
				break;
			}
		}

		default: {
			error_log("IOP: unimplemented transfer for {} channel (mode {}, bcr {:08x})",
				m_TransferChannel->id,
				m_TransferChannel->chcr & static_cast<u32>(CHCRBits::Mode) >> 8,
				m_TransferChannel->bcr);

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

u8 DMAC::GetChannelPriority(ChannelID channel) {
	u8 ch = static_cast<u8>(channel);
	
	// old channels
	if (channel <= ChannelID::OTC) {
		u32 bits = 0b1111 << (ch * 4);
		return static_cast<u8>((m_Regs.dpcr & bits) >> (ch * 4));
	}

	// new channels
	u32 bits = 0b1111 << ((ch - 7) * 4);
	return static_cast<u8>((m_Regs.dpcr2 & bits) >> ((ch - 7) * 4));
}