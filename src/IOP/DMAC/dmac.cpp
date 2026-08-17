#include "dmac.hpp"
using namespace IOProcessor::DMA;

u32 Channel::GetControl() {
	u32 r = 0;
	r |= (u8)direction;
	r |= (u8)step << 1;
	r |= (u8)chop << 8;
	r |= (u8)sync << 9;
	r |= chop_dma_size << 16;
	r |= chop_cpu_size << 20;
	r |= enable << 24;
	r |= trigger << 28;
	r |= dummy << 29;

	return r;
}

void Channel::SetControl(u32 value) {
	direction = (Direction)(value & 1);
	step = (Step)((value >> 1) & 1);
	sync = (Sync)((value >> 9) & 3);

	chop = ((value >> 8) & 1);
	chop_dma_size = (value >> 16) & 7;
	chop_cpu_size = (value >> 20) & 7;

	enable = (value >> 24) & 1;
	trigger = (value >> 28) & 1;
	
	dummy = (value >> 29) & 3;
}

bool InterruptRegister::GetIRQStatus() {
	bool channel_irq = channel_irq_flags & channel_enable_irq;

	// there is an irq if either condition is met:
	// 1. irq is forced to be enabled
	// 2. irqs are enabled and there is an ongoing irq
	return force_irq || (enable_irq && (channel_irq != 0));
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

u32 DMAC::Read(u32 address) {
	if ((address & 0xf00) == 0x500) {
		debug_log("read from unknown dmac address {:08x}", address);
		return 0;
	}

	u32 offset = address - 0x1f801080;
	u8 major = (offset & 0x70) >> 4;
	u8 minor = offset & 0xf;

	if (major <= 6) {
		auto channel = m_Channels[major];
		if (!channel) {
			error_log("reading unknown DMAC channel {}", major);
			return 0;
		}

		switch (minor) {
			case 0: return channel->base;
			case 4: return channel->GetBlockControl();
			case 8: return channel->GetControl();
			default: { error_log("unhandled DMAC read"); exit(1); }
		}
	}
	
	else if (major == 7) {
		switch (minor) {
			case 0: return m_Control;
			case 4: return m_Interrupt.GetValue();
			default: { error_log("unhandled DMAC read"); exit(1); }
		}
	}

	else {
		error_log("unhandled DMAC read");
		exit(1);
	}
}

void DMAC::Write(u32 address, u32 value) {
	if ((address & 0xf00) == 0x500) {
		debug_log("write {:08x} -> unknown dmac address {:08x}", value, address);
		return;
	}

	u32 offset = address - 0x1f801080;
	u8 major = (offset & 0x70) >> 4;
	u8 minor = offset & 0xf;

	if (major <= 6) {
		auto channel = m_Channels[major];
		if (!channel) {
			error_log("writing {:08x} -> unknown DMAC channel {}", value, major);
			return;
		}

		switch (minor) {
			case 0: channel->base = value & 0xffffff; break;
			case 4: channel->SetBlockControl(value); break;
			case 8: channel->SetControl(value); break;
			default: { error_log("unhandled DMAC write"); exit(1); }
		}

		// execute a DMAC transfer if it has now been activated
		if (channel->IsActive()) {
			DoDMATransfer(channel);
		}
	}
	
	else if (major == 7) {
		switch (minor) {
			case 0: m_Control = value; break;
			case 4: m_Interrupt.SetValue(value); break;
			default: { error_log("unhandled DMAC write"); exit(1); }
		}
	}

	else {
		error_log("unhandled DMAC write");
		exit(1);
	}
}

void DMAC::DoDMATransfer(std::shared_ptr<Channel> channel) {
	if (!channel) {
		error_log("attempting DMAC transfer to unknown port");
		return;
	}

	switch (channel->sync) {
		case Sync::LinkedList: DoLinkedList(channel); break;
		case Sync::Manual: case Sync::Request: DoBlockCopy(channel); break;
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
	m_Control = 0x07654321;
}