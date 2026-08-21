#include "../emotion.hpp"
#include "../../SubsystemInterface/sif.hpp"
#include "dmac.hpp"

using namespace EmotionEngine::DMA;

void DMAC::Reset() {
	// clear out channels
	memset(m_Channels.channels, 0, sizeof(m_Channels.channels));
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

	// clear out registers
	memset(&m_Regs, 0, sizeof(m_Regs));

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
			m_InTransfer = true;
			m_TransferChannel = &channel;

			// make sure chain state is set up
			if (static_cast<Mode>((channel.chcr & CHCRBits::MOD) >> 2) == Mode::Chain) {
				m_TransferChannel->chain = ChainState::ReadData;
				m_TransferChannel->tag_end = false;
			}

			return;
		}
	}
}

void DMAC::Write(u32 address, u32 word) {
	// channel registers
	if (address < 0x1000e000) {
		ChannelID channel = GetChannelFromAddress((address >> 8) & 0xff);
		WriteToChannel(channel, address, word);
		return;
	}

	// dmac registers
	WriteToReg(address, word);
}

u32 DMAC::Read(u32 address) {
	// channel registers
	if (address < 0x1000e000) {
		ChannelID channel = GetChannelFromAddress((address >> 8) & 0xff);
		return ReadFromChannel(channel, address);
	}

	// dmac registers
	return ReadFromReg(address);
}

void DMAC::WriteToChannel(ChannelID ch, u32 address, u32 word) {
	u8 channel = static_cast<u8>(ch);
	ChannelReg reg = static_cast<ChannelReg>(address & 0xff);
	//debug_log("write {:08x} -> {}({})", word, channel, reg);

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

u32 DMAC::ReadFromChannel(ChannelID ch, u32 address) {
	u8 channel = static_cast<u8>(ch);
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
	//debug_log("write {:08x} -> D_{}", word, reg);

	switch (reg) {
		case DmacReg::STAT: {
			m_Regs.stat &= ~(word & 0x000003ff); // channel interrupt status
			m_Regs.stat ^=  (word & 0x03ff0000); // channel interrupt mask
			return;
		}

		case DmacReg::CTRL:		{ m_Regs.ctrl = word; return; }
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

	switch (static_cast<Mode>((m_TransferChannel->chcr & CHCRBits::MOD) >> 2)) {
		case Mode::Normal: {
			DoNormalTransfer();
			break;
		}

		case Mode::Chain: {
			switch (m_TransferChannel->id) {
				case ChannelID::SIF0: {
					if (!m_EE->GetSIF()->GetSIF0()->DataAvailable()) {
						return;
					}
					
					DoDestChainTransfer();
					break;
				}

				case ChannelID::SIF1: {
					DoSourceChainTransfer();
					break;
				}

				default: {
					error_log("unknown chain transfer for {}", m_TransferChannel->id);
					exit(1);
				}
			}

			break;
		}

		case Mode::Interleave: {
			DoInterleaveTransfer();
			break;
		}

		default: {
			error_log("invalid dma mode");
			exit(1);
		}
	}
}

void DMAC::SendQword(u128 qword) {
	switch (m_TransferChannel->id) {
		case ChannelID::GIF: {
			m_EE->GetGIF().ReceivePath3(qword);
			break;
		}
		
		case ChannelID::SIF1: {
			m_EE->GetSIF()->GetSIF1()->PushFifo(qword);
			break;
		}

		default: {
			error_log("write {:016x}{:016x} -> unknown dma channel {}", (u64)(qword >> 64), (u64)qword, m_TransferChannel->id);
			exit(1);
		}
	}
}

u128 DMAC::RecvQword() {
	switch (m_TransferChannel->id) {
		case ChannelID::SIF0: {
			return m_EE->GetSIF()->GetSIF0()->PopFifo();
		}

		default: {
			error_log("read from unknown dma channel {}", m_TransferChannel->id);
			exit(1);
		}
	}
}

u128 DMAC::ReadQwordFromMemory() {
	// send from scratchpad
	if (m_TransferChannel->last_tag.scratchpad) {
		u64 lo = m_EE->GetMemory().ReadVirtualMemory64(0x70000000 + (m_TransferChannel->madr & 0x3ff0));
		u64 hi = m_EE->GetMemory().ReadVirtualMemory64(0x70000000 + ((m_TransferChannel->madr + 8) & 0x3ff0));
		return ((u128)hi << 64) | lo;
	}

	// send from RAM
	else {
		u64 lo = m_EE->GetMemory().ReadVirtualMemory64(m_TransferChannel->madr);
		u64 hi = m_EE->GetMemory().ReadVirtualMemory64(m_TransferChannel->madr + 8);
		return ((u128)hi << 64) | lo;
	}
}

void DMAC::WriteQwordToMemory(u128 qword) {
	// send from scratchpad
	if (m_TransferChannel->last_tag.scratchpad) {
		m_EE->GetMemory().WriteVirtualMemory64(0x70000000 + (m_TransferChannel->madr & 0x3ff0), static_cast<u64>(qword));
		m_EE->GetMemory().WriteVirtualMemory64(0x70000000 + ((m_TransferChannel->madr + 8) & 0x3ff0), static_cast<u64>(qword >> 64));
	}

	// send from RAM
	else {
		m_EE->GetMemory().WriteVirtualMemory64(m_TransferChannel->madr, static_cast<u64>(qword));
		m_EE->GetMemory().WriteVirtualMemory64(m_TransferChannel->madr + 8, static_cast<u64>(qword >> 64));
	}
}

void DMAC::DoNormalTransfer() {
	if (m_TransferChannel->qwc == 0) {
		FinishTransfer();
		return;
	}

	u32 address = m_TransferChannel->madr;
	m_TransferChannel->qwc -= 1;
	m_TransferChannel->madr += 16;

	u64 lo = m_EE->GetMemory().ReadVirtualMemory64(address);
	u64 hi = m_EE->GetMemory().ReadVirtualMemory64(address + 8);
	SendQword(((u128)hi << 64) | lo);
}

void DMAC::DoSourceChainTransfer() {
	if (m_TransferChannel->chain == ChainState::ReadData) {
		// done transferring the tag
		if (m_TransferChannel->qwc == 0) {
			// check if tag ended
			// "When both IRQ and Dn_CHCR.TIE are set, the transfer ends after QWC has been transferred."
			if (m_TransferChannel->tag_end || (m_TransferChannel->last_tag.irq && (m_TransferChannel->chcr & CHCRBits::TIE))) {
				FinishTransfer();
				return;
			}

			// go read another DMAtag
			m_TransferChannel->chain = ChainState::ReadDMAtag;
			return;
		}

		SendQword(ReadQwordFromMemory());
		m_TransferChannel->qwc--;
		m_TransferChannel->madr += 16;
	}

	if (m_TransferChannel->chain == ChainState::ReadDMAtag) {
		// read a DMAtag and process its tag id
		ReadSourceTag();
		ProcessSourceChainTagID();

		// "When Dn_CHCR.TTE is on, bits 64-127 are transferred BEFORE QWC."
		if (m_TransferChannel->chcr & CHCRBits::TTE) {
			SendQword(m_TransferChannel->last_tag.data);
		}

		// go read data
		m_TransferChannel->chain = ChainState::ReadData;
	}
}

void DMAC::DoDestChainTransfer() {
	if (m_TransferChannel->chain == ChainState::ReadData) {
		// done transferring the tag
		if (m_TransferChannel->qwc == 0) {
			// check if tag ended
			// "When both IRQ and Dn_CHCR.TIE are set, the transfer ends after QWC has been transferred."
			if (m_TransferChannel->tag_end || (m_TransferChannel->last_tag.irq && (m_TransferChannel->chcr & CHCRBits::TIE))) {
				FinishTransfer();
				return;
			}

			// go read another DMAtag
			m_TransferChannel->chain = ChainState::ReadDMAtag;
			return;
		}

		WriteQwordToMemory(RecvQword());
		m_TransferChannel->qwc--;
		m_TransferChannel->madr += 16;
	}

	if (m_TransferChannel->chain == ChainState::ReadDMAtag) {
		// read a DMAtag and process its tag id
		ReadDestTag();
		ProcessDestChainTagID();

		// "When Dn_CHCR.TTE is on, bits 64-127 are transferred BEFORE QWC."
		if (m_TransferChannel->chcr & CHCRBits::TTE) {
			WriteQwordToMemory(m_TransferChannel->last_tag.data);
		}

		// go read data
		m_TransferChannel->chain = ChainState::ReadData;
	}
}

void DMAC::DoInterleaveTransfer() {
	error_log("unimplemented");
	exit(1);
}

void DMAC::FinishTransfer() {
	m_InTransfer = false;
	m_TransferChannel->chcr &= ~CHCRBits::STR; // clear STR
	m_Regs.stat |= (1 << static_cast<u8>(m_TransferChannel->id));
	CheckInterrupt();
}

void DMAC::CheckInterrupt() {
	u16 stat = m_Regs.stat & 0x3ff;
	u16 mask = (m_Regs.stat >> 16) & 0x3ff;

	if (stat & mask) {
		m_EE->GetR5900().cop0.cause |= (1 << 11); // INT1
	} else {
		m_EE->GetR5900().cop0.cause &= (1 << 11);
	}
}

void DMAC::ReadSourceTag() {
	u64 lo = m_EE->GetMemory().ReadVirtualMemory64(m_TransferChannel->tadr);
	m_TransferChannel->last_tag.qword_count = lo & 0xffff;
	m_TransferChannel->last_tag.enable_priority_control = ((lo >> 26) & 0b11) == 3 ? true : false;
	m_TransferChannel->last_tag.id = (lo >> 28) & 0b111;
	m_TransferChannel->last_tag.irq = (lo >> 31) & 1;
	m_TransferChannel->last_tag.addr = (lo >> 32) & 0xfffffff0;
	m_TransferChannel->last_tag.scratchpad = (lo >> 63) & 1;

	m_TransferChannel->qwc = m_TransferChannel->last_tag.qword_count;

	// read data if CHCR.TTE==1
	if (m_TransferChannel->chcr & CHCRBits::TTE) {
		m_TransferChannel->last_tag.data = m_EE->GetMemory().ReadVirtualMemory64(m_TransferChannel->tadr + 8);
	}
}

void DMAC::ReadDestTag() {
	u128 qword = RecvQword();
	u64 lo = static_cast<u64>(qword);
	m_TransferChannel->last_tag.qword_count = lo & 0xffff;
	m_TransferChannel->last_tag.enable_priority_control = ((lo >> 26) & 0b11) == 3 ? true : false;
	m_TransferChannel->last_tag.id = (lo >> 28) & 0b111;
	m_TransferChannel->last_tag.irq = (lo >> 31) & 1;
	m_TransferChannel->last_tag.addr = (lo >> 32) & 0xfffffff0;
	m_TransferChannel->last_tag.scratchpad = (lo >> 63) & 1;

	m_TransferChannel->qwc = m_TransferChannel->last_tag.qword_count;

	// read data if CHCR.TTE==1
	if (m_TransferChannel->chcr & CHCRBits::TTE) {
		m_TransferChannel->last_tag.data = static_cast<u64>(qword >> 64);
	}
}

void DMAC::ProcessSourceChainTagID() {
	switch (static_cast<SourceChainTagID>(m_TransferChannel->last_tag.id)) {
		case SourceChainTagID::refe: {
			m_TransferChannel->madr = m_TransferChannel->last_tag.addr;
			m_TransferChannel->tadr += 16;
			m_TransferChannel->tag_end = true;
			break;
		}

		case SourceChainTagID::cnt: {
			m_TransferChannel->madr = m_TransferChannel->tadr + 16;
			m_TransferChannel->tadr = m_TransferChannel->madr;
			break;
		}

		case SourceChainTagID::next: {
			m_TransferChannel->madr = m_TransferChannel->tadr + 16;
			m_TransferChannel->tadr = m_TransferChannel->last_tag.addr;
			break;
		}

		case SourceChainTagID::ref:
		case SourceChainTagID::refs: {
			m_TransferChannel->madr = m_TransferChannel->last_tag.addr;
			m_TransferChannel->tadr += 16;
			break;
		}

		case SourceChainTagID::call: {
			// memory address is just after DMAtag
			m_TransferChannel->madr = m_TransferChannel->tadr + 16;

			// save next DMAtag in the ASR
			u8 asp = (m_TransferChannel->chcr & CHCRBits::ASP) >> 4;
			u32& asr = (asp == 0) ? m_TransferChannel->asr0 : m_TransferChannel->asr1;
			asr = m_TransferChannel->madr + (m_TransferChannel->qwc * 16);

			// set next DMAtag address
			m_TransferChannel->tadr = m_TransferChannel->last_tag.addr;

			// increment ASP
			m_TransferChannel->chcr &= ~CHCRBits::ASP;
			m_TransferChannel->chcr |= (asp + 1) << 4;
			break;
		}

		case SourceChainTagID::ret: {
			// memory address is just after DMAtag
			m_TransferChannel->madr = m_TransferChannel->tadr + 16;

			// transfer finished if ASP=0
			u8 asp = (m_TransferChannel->chcr & CHCRBits::ASP) >> 4;
			if (asp == 0) {
				m_TransferChannel->tag_end = true;
			}

			// load TADR from the saved ASR
			else {
				u32& asr = (asp == 2) ? m_TransferChannel->asr1 : m_TransferChannel->asr0;
				m_TransferChannel->tadr = asr;

				// decrement ASP
				m_TransferChannel->chcr &= ~CHCRBits::ASP;
				m_TransferChannel->chcr |= (asp - 1) << 4;
			}

			break;
		}

		case SourceChainTagID::end: {
			m_TransferChannel->madr = m_TransferChannel->tadr + 16;
			m_TransferChannel->tag_end = true;
			break;
		}
	}
}

void DMAC::ProcessDestChainTagID() {
	switch (static_cast<DestChainTagID>(m_TransferChannel->last_tag.id)) {
		case DestChainTagID::cnt:
		case DestChainTagID::cnts: {
			m_TransferChannel->madr = m_TransferChannel->last_tag.addr;
			break;
		}

		case DestChainTagID::end: {
			m_TransferChannel->madr = m_TransferChannel->last_tag.addr;
			m_TransferChannel->tag_end = true;
			break;
		}
	}
}