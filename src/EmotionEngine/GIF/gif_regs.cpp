#include "gif.hpp"
using namespace EmotionEngine::Graphics;

void GIF::WriteCtrl(u32 word) {
	if (word & 1) {
		Reset();
	}

	// TODO: temporary stop
}

void GIF::WriteMode(u32 word) {
	m_ModeMaskPath3 = word & 1;
	// TODO: intermittent mode
}

u32 GIF::ReadStat() {
	u32 r = 0;
	r |= (m_ModeMaskPath3 ? 1 : 0) << 0;
	r |= (m_VifMaskPath3 ? 1 : 0) << 1;
	r |= 0 << 2; // TODO: temporary stop
	r |= 0 << 3; // TODO: intermittent mode
	r |= 0 << 4; // unused
	r |= 0 << 5; // TODO: intermittent mode
	r |= (m_Path3Fifo.size() ? 1 : 0) << 6;
	r |= m_Path2.queued << 7;
	r |= m_Path1.queued << 8;
	r |= ((m_State == GIFstate::ReceiveData) ? 1 : 0) << 9;
	r |= static_cast<u8>(m_ActivePath) << 10;
	r |= 0 << 12; // EE->GS (TODO: can there ever be GS->EE transfer??)
	r |= m_Path3Fifo.size() << 24;
	return r;
}