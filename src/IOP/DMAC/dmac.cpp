#include "dmac.hpp"
using namespace IOProcessor::DMA;

u32 Channel::GetControl() {
	u32 r = 0;
	r |= (u8)direction;
	r |= (u8)step << 1;
	r |= (u8)chop << 8;
	r |= (u8)mode << 9;
	r |= chop_dma_size << 16;
	r |= chop_cpu_size << 20;
	r |= enable << 24;
	r |= trigger << 28;

	return r;
}

void Channel::SetControl(u32 value) {
	direction = (Direction)(value & 1);
	step = (Step)((value >> 1) & 1);
	mode = (Mode)((value >> 9) & 3);

	chop = ((value >> 8) & 1);
	chop_dma_size = (value >> 16) & 7;
	chop_cpu_size = (value >> 20) & 7;

	enable = (value >> 24) & 1;
	trigger = (value >> 28) & 1;
}

bool InterruptRegister::GetIRQStatus() {
	return force_irq || (enable_irq && channel_irq_flags != 0);
}

u32 InterruptRegister::GetValue() {
	u32 r = 0;
	r |= dummy & 0x3f;
	r |= force_irq << 15;
	r |= channel_enable_irq << 16;
	r |= enable_irq << 23;
	r |= channel_irq_flags << 24;
	r |= GetIRQStatus() << 31;

	return r;
}

void InterruptRegister::SetValue(u32 value) {
	dummy = value & 0x3f;
	force_irq = (value >> 15) & 1;
	channel_enable_irq = (value >> 16) & 0x7f;
	enable_irq = (value >> 23) & 1;

	u8 ack = (value >> 24) & 0x3f;
	channel_irq_flags &= ~ack;
}

void InterruptRegister::TryInterrupt(Interrupt::INTC* intc) {
	if (GetIRQStatus()) intc->Interrupt(Interrupt::IRQ::DMA);
}

u32 DMAC::Read(u32 address) {
	if ((address & 0xff0) == 0x0f0) {
		switch (address & 0xf) {
			case 0x0: return m_Control;
			case 0x4: return m_Interrupt.GetValue();
			default: { error_log("unhandled DMAC write"); exit(1); }
		}
	} else if ((address & 0xff0) == 0x570) {
		switch (address & 0xf) {
			case 0x0: return m_Control2;
			case 0x4: return m_Interrupt2.GetValue();
			case 0x8: return m_EnableDMA;
			case 0xc: return m_DisableInterrupt;
		}
	}

	auto channel = GetChannel(address);
	if (!channel) {
		error_log("reading from unknown DMAC channel");
		return 0;
	}

	switch (address & 0xf) {
		case 0x0: return channel->base;
		case 0x4: return channel->GetBlockControl();
		case 0x8: return channel->GetControl();
		case 0xc: return channel->tag_address;
		default: { error_log("unhandled DMAC read"); exit(1); }
	}
}

void DMAC::Write(u32 address, u32 value) {
	if ((address & 0xff0) == 0x0f0) {
		switch (address & 0xf) {
			case 0x0: m_Control = value; return;
			case 0x4: m_Interrupt.SetValue(value); return;
			default: { error_log("unhandled DMAC write"); exit(1); }
		}
	} else if ((address & 0xff0) == 0x570) {
		switch (address & 0xf) {
			case 0x0: m_Control2 = value; return;
			case 0x4: m_Interrupt2.SetValue(value); return;
			case 0x8: m_EnableDMA = value & 1; return;
			case 0xc: m_DisableInterrupt = value & 1; return;
		}
	}

	auto channel = GetChannel(address);
	if (!channel) {
		error_log("writing {:08x} -> unknown DMAC channel", value);
		return;
	}

	switch (address & 0xf) {
		case 0x0: channel->base = value & 0xffffff; break;
		case 0x4: channel->SetBlockControl(value); break;
		case 0x8: {
			channel->SetControl(value);

			// execute a DMAC transfer if it has now been activated
			if (channel->IsActive()) {
				DoDMATransfer(channel);
			}

			break;
		}

		case 0xc: channel->tag_address = value & 0xffffff; break;
		default: { error_log("unhandled DMAC write"); exit(1); }
	}
}

void DMAC::DoDMATransfer(std::shared_ptr<Channel> channel) {
	if (!m_EnableDMA) return;
	if (!channel) {
		error_log("attempting DMAC transfer to unknown port");
		return;
	}

	switch (channel->mode) {
		case Mode::LinkedList: DoLinkedList(channel); break;
		case Mode::Burst: case Mode::Slice: DoBlockCopy(channel); break;
	}

	channel->TransferDone();
}

void DMAC::DoBlockCopy(std::shared_ptr<Channel> channel) {
	int increment = (channel->step == Step::Increment) ? 4 : -4;
	u32 address = channel->base;
	u32 transfer_size = channel->GetTransferSize();

	if (channel->direction == Direction::FromRam) {
		while (transfer_size-- > 0) {
			channel->Write(m_Memory->ReadVirtualMemory32(address & 0x1ffffc));
			address += increment;
		}
	} else {
		while (transfer_size-- > 0) {
			m_Memory->WriteVirtualMemory32(address & 0x1ffffc, channel->Read(address, transfer_size));
			address += increment;
		}
	}
}

void DMAC::DoLinkedList(std::shared_ptr<Channel> channel) {
	u32 address = channel->base & 0x1ffffc;
	if (channel->direction == Direction::ToRam) {
		error_log("invalid DMAC direction for linked list");
		return;
	}

	while (true) {
		// each entry starts with a header word.
		// hi byte = number of words in packet
		// rest = address of next entry
		u32 header = m_Memory->ReadVirtualMemory32(address);
		u8 remaining_words = (header >> 24) & 0xff;

		// do the transfer
		while (remaining_words > 0) {
			address = (address + 4) & 0x1ffffc;
			channel->Write(m_Memory->ReadVirtualMemory32(address));

			remaining_words -= 1;
		}

		// only MSB is checked for the end of table marker
		if (header & 0x800000) {
			break;
		}

		// go to next entry in linked list
		address = header & 0x1ffffc;
	}
}

void DMAC::Reset() {
	m_Control = 0x07777777;
}

std::shared_ptr<Channel> DMAC::GetChannel(u32 address) {
    if (address >= 0x1f801080 && address < 0x1f8010f0) {
        u32 channel = (address - 0x1f801080) / 0x10;
        return m_Channels[channel];
    }

    if (address >= 0x1f801500 && address < 0x1f801560) {
        u32 channel = 7 + (address - 0x1f801500) / 0x10;
        return m_Channels[channel];
    }

    return nullptr;
}

u32 InterruptRegister2::GetValue() {
	u32 r = 0;
	r |= tag_irq_flags << 0;
	r |= channel_enable_irq << 16;
	r |= channel_irq_flags << 24;

	return r;
}

void InterruptRegister2::SetValue(u32 value) {
	tag_irq_flags = value & 0b011000010000; // only bits 4, 9 and 10 can be set
	channel_enable_irq = (value >> 16) & 0x7f;

	u8 ack = (value >> 24) & 0x3f;
	channel_irq_flags &= ~ack;
}

void InterruptRegister2::TryInterrupt(Interrupt::INTC* intc) {
	if (!dicr->enable_irq) return;
	if (channel_irq_flags & channel_enable_irq) intc->Interrupt(Interrupt::IRQ::DMA);
}

void InterruptRegister2::TryTagInterrupt(Interrupt::INTC* intc) {
	if (tag_irq_flags) intc->Interrupt(Interrupt::IRQ::DMA);
}

void DMAC::RaiseInterrupt(Port port) {
	if (m_DisableInterrupt) return;

	u8 idx = static_cast<u8>(port);
	if (idx <= 6) {
		m_Interrupt.channel_irq_flags |= (1 << idx);
		m_Interrupt.TryInterrupt(m_INTC);
	} else {
		m_Interrupt2.channel_irq_flags |= (1 << (idx - 7));
		m_Interrupt2.TryInterrupt(m_INTC);
	}
}

void DMAC::RaiseTagInterrupt(Port port) {
	if (port == Port::SPU1 || port == Port::SIF0 || port == Port::SIF1) {
		m_Interrupt2.tag_irq_flags |= (1 << static_cast<u8>(port));
		m_Interrupt2.TryTagInterrupt(m_INTC);
	}
}