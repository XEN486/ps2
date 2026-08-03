#include "gs.hpp"
using namespace GraphicsSynthesizer;

void GS::WritePrim(u16 value) {
	m_IRegs.prim = value;
}

void GS::WritePrivilegedRegEE(u32 address, u32 word) {
	auto id = static_cast<PrivilegedRegisterID>(address & 0xffff);
	switch (id) {
		case PMODE		+ 0: { m_PRegs.pmode = word; break; }
		case PMODE		+ 4: { m_PRegs.pmode |= ((u64)word << 32); break; }
		case SMODE2		+ 0: { m_PRegs.smode2 = word; break; }
		case SMODE2		+ 4: { m_PRegs.smode2 |= ((u64)word << 32); break; }
		case DISPFB1	+ 0: { m_PRegs.dispfb1 = word; break; }
		case DISPFB1	+ 4: { m_PRegs.dispfb1 |= ((u64)word << 32); break; }
		case DISPLAY1	+ 0: { m_PRegs.display1 = word; break; }
		case DISPLAY1	+ 4: { m_PRegs.display1 |= ((u64)word << 32); break; }
		case DISPFB2	+ 0: { m_PRegs.dispfb2 = word; break; }
		case DISPFB2	+ 4: { m_PRegs.dispfb2 |= ((u64)word << 32); break; }
		case DISPLAY2	+ 0: { m_PRegs.display2 = word; break; }
		case DISPLAY2	+ 4: { m_PRegs.display2 |= ((u64)word << 32); break; }
		case EXTBUF		+ 0: { m_PRegs.extbuf = word; break; }
		case EXTBUF		+ 4: { m_PRegs.extbuf |= ((u64)word << 32); break; }
		case EXTDATA	+ 0: { m_PRegs.extdata = word; break; }
		case EXTDATA	+ 4: { m_PRegs.extdata |= ((u64)word << 32); break; }
		case BGCOLOR	+ 0: { m_PRegs.bgcolor = word; break; }
		case BGCOLOR	+ 4: { m_PRegs.bgcolor |= ((u64)word << 32); break; }
		case GS_CSR		+ 0: { m_PRegs.gs_csr = word; break; }
		case GS_CSR		+ 4: { m_PRegs.gs_csr |= ((u64)word << 32); break; }
		case GS_IMR		+ 0: { m_PRegs.gs_imr = word; break; }
		case GS_IMR		+ 4: { m_PRegs.gs_imr |= ((u64)word << 32); break; }
		case BUSDIR		+ 0: { m_PRegs.busdir = word; break; }
		case BUSDIR		+ 4: { m_PRegs.busdir |= ((u64)word << 32); break; }
		case SIGLBLID	+ 0: { m_PRegs.siglblid = word; break; }
		case SIGLBLID	+ 4: { m_PRegs.siglblid |= ((u64)word << 32); break; }
	}
}

void GS::WritePrivilegedReg(u32 address, u64 dword) {
	auto id = static_cast<PrivilegedRegisterID>(address & 0xffff);
	switch (id) {
		case PMODE:			{ m_PRegs.pmode = dword; break; }
		case SMODE1:		{ m_PRegs.smode1 = dword; break; }
		case SMODE2:		{ m_PRegs.smode2 = dword; break; }
		case SRFSH:			{ m_PRegs.srfsh = dword; break; }
		case SYNCH1:		{ m_PRegs.synch1 = dword; break; }
		case SYNCH2:		{ m_PRegs.synch2 = dword; break; }
		case SYNCV:			{ m_PRegs.syncv = dword; break; }
		case DISPFB1:		{ m_PRegs.dispfb1 = dword; break; }
		case DISPLAY1:		{ m_PRegs.display1 = dword; break; }
		case DISPFB2:		{ m_PRegs.dispfb2 = dword; break; }
		case DISPLAY2:		{ m_PRegs.display2 = dword; break; }
		case EXTBUF:		{ m_PRegs.extbuf = dword; break; }
		case EXTWRITE:		{ m_PRegs.extwrite = dword; break; }
		case BGCOLOR:		{ m_PRegs.bgcolor = dword; break; }
		case GS_CSR:		{ m_PRegs.gs_csr = dword; break; }
		case GS_IMR:		{ m_PRegs.gs_imr = dword; break; }
		case BUSDIR:		{ m_PRegs.busdir = dword; break; }
		case SIGLBLID:		{ m_PRegs.siglblid = dword; break; }
	}
}

void GS::WriteInternalReg(InternalRegisterID id, u64 dword) {
	switch (id) {
		case FRAME_1:		{ m_IRegs.ctx1.frame = dword; break; }
		case FRAME_2:		{ m_IRegs.ctx2.frame = dword; break; }
		case XYOFFSET_1:	{ m_IRegs.ctx1.xyoffset = dword; break; }
		case XYOFFSET_2:	{ m_IRegs.ctx2.xyoffset = dword; break; }
		case SCISSOR_1:		{ m_IRegs.ctx1.scissor = dword; break; }
		case SCISSOR_2:		{ m_IRegs.ctx2.scissor = dword; break; }
		case PRIM:			{ m_IRegs.prim = dword; break; }
		case RGBAQ:			{ m_IRegs.rgbaq = RegRGBAQ(dword, true); break; }
		case XYZF2:			{ WriteXYZ(RegXYZF(dword), false); break; }
		case XYZF3:			{ WriteXYZ(RegXYZF(dword), true); break; }
		case XYZ2:			{ WriteXYZ(RegXYZ(dword), false); break; }
		case XYZ3:			{ WriteXYZ(RegXYZ(dword), true); break; }

		default: {
			error_log("unknown internal register {:02x}", (u8)id);
			exit(1);
		}
	}
}

void GS::WriteXYZ(AnyXYZ xyz, bool cull) {
	Vertex v(xyz, m_IRegs.rgbaq);
	m_VertexQueue.push_back(v);

	// store initial vertex if this is the initial drawing kick
	if (m_IRegs.prim.initial_kick && m_VertexQueue.size() == 1) {
		m_InitialVertex = v;
	}

	// when we reach the necessary vertex count, try do a kick
	if (m_VertexQueue.size() == m_IRegs.prim.GetVertexCount()) {
		m_IRegs.prim.initial_kick = false;

		// XYZF3/XYZ3 don't do a drawing kick
		if (!cull) {
			DrawingKick();
		}

		// clear the old queue
		m_VertexQueue.clear();
		m_LastVertex = v;
	}
}

void GS::DrawingKick() {
	debug_log("GS: draw {} vertices (type {})", m_VertexQueue.size(), (u8)m_IRegs.prim.type);
}