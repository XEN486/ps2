#include "dmac.hpp"
#include "../iop.hpp"
using namespace IOProcessor::DMA;

u32 ICR::Read() const {
	u32 r = 0;
	r |= channel_int_on_slice_and_ll;
	r |= channel_int_mask << 16;
	r |= master_channel_int_enable << 23;
	r |= channel_int_flags << 24;
	r |= master_int_flag << 31;
	return r;
}

void ICR::Write(u32 word) {
	channel_int_on_slice_and_ll = word & 0b1111111;
	channel_int_mask = (word >> 16) & 0b1111111;
	master_channel_int_enable = (word >> 23) & 1;
	channel_int_flags &= ~((word >> 24) & 0b1111111);

	RecalculateMIF();
}

void ICR::RecalculateMIF() {
	u8 int_enable = *dmacinten;

	// dmacinten.0: "When 0, all channel interrupts disabled. Master interrupt flag is 0 in all cases but bus error interrupts."
	if (!(int_enable & 1)) {
		master_int_flag = false;
		return;
	}

	// dmacinten.1: "When 1, DMA interrupts disabled - IRQ 3 is never sent to INTC. Does not affect master interrupt flag."
	master_int_flag = master_channel_int_enable && (channel_int_flags || icr2->channel_int_flags);
	if (master_int_flag && !(int_enable & 0b10)) {
		iop->GetINTC().Interrupt(IOProcessor::Interrupt::IRQ::DMA);
	}
}

u32 ICR2::Read() const {
	u32 r = 0;
	r |= int_on_tag;
	r |= channel_int_mask << 16;
	r |= channel_int_flags << 24;
	return r;
}

void ICR2::Write(u32 word) {
	int_on_tag = word & 0b0011000010000;
	channel_int_mask = (word >> 16) & 0b111111;
	channel_int_flags &= ~((word >> 24) & 0b111111);

	icr->RecalculateMIF();
}