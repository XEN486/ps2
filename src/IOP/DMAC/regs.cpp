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
	channel_int_flags &= (word >> 24) & 0b1111111;

	RecalculateMIF();
}

void ICR::RecalculateMIF() {
	master_int_flag = master_channel_int_enable && (channel_int_flags || icr2->channel_int_flags);
	if (master_int_flag && !(*dmacinten)) {
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
	channel_int_flags &= (word >> 24) & 0b111111;
}