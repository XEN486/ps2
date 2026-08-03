#include "gif.hpp"
using namespace EmotionEngine::Graphics;

void GIF::Reset() {
	// get ready for giftag from any path
	m_ActivePath = ActivePath::None;
	m_State = GIFstate::ReceiveTag;

	// empty queues
	m_Path1.queued = false;
	m_Path2.queued = false;

	std::queue<u128> empty;
	std::swap(m_Path3Fifo, empty);
}

void GIF::ReceivePath1(u128 qword) {
	m_Path1.qword = qword;
	m_Path1.queued = true;
}

void GIF::ReceivePath2(u128 qword) {
	m_Path2.qword = qword;
	m_Path2.queued = true;
}

void GIF::ReceivePath3(u128 qword) {
	m_Path3Fifo.push(qword);
}

void GIF::ProcessQword() {
	// try find a path to activate
	if (m_ActivePath == ActivePath::None) {
		// path1
		if (m_Path1.queued) {
			m_ActivePath = ActivePath::Path1;
		}
		
		// path2
		else if (m_Path2.queued) {
			m_ActivePath = ActivePath::Path2;
		}
		
		// path3
		else if (!m_Path3Fifo.empty()) {
			m_ActivePath = ActivePath::Path3;
		}

		// nothing queued
		else {
			return;
		}
	}

	u128 qword;
	switch (m_ActivePath) {
		case ActivePath::Path1: {
			if (!m_Path1.queued) {
				m_ActivePath = ActivePath::None;
				return;
			}

			qword = m_Path1.qword;
			m_Path1.queued = false;
			break;
		}

		case ActivePath::Path2: {
			if (!m_Path2.queued) {
				m_ActivePath = ActivePath::None;
				return;
			}

			qword = m_Path2.qword;
			m_Path2.queued = false;
			break;
		}

		case ActivePath::Path3: {
			if (m_Path3Fifo.empty()) {
				m_ActivePath = ActivePath::None;
				return;
			}

			qword = m_Path3Fifo.front();
			m_Path3Fifo.pop();
			break;
		}
	}

	if (m_State == GIFstate::ReceiveTag) {
		m_RecentGIFtag = qword;
		u64 lo64 = static_cast<u64>(m_RecentGIFtag);

		m_LastTag.nloop			= lo64 & 0x7fff;
		m_LastTag.eop			= (lo64 >> 15) & 1;
		m_LastTag.enable_prim	= (lo64 >> 46) & 1;
		m_LastTag.prim_data		= (lo64 >> 47) & 0x3ff;
		m_LastTag.data_format	= static_cast<DataFormat>((lo64 >> 58) & 0b11);
		m_LastTag.nregs			= (lo64 >> 60) & 0b1111;
		if (m_LastTag.nregs == 0) m_LastTag.nregs = 16;

		debug_log("giftag({}, {}, {}, {}, {}, {})", m_LastTag.nloop, m_LastTag.eop, m_LastTag.enable_prim, m_LastTag.prim_data, (u8)m_LastTag.data_format, m_LastTag.nregs);

		// NLOOP == 0 case
		if (m_LastTag.nloop == 0) {
			if (m_LastTag.eop) {
				debug_log("end of packet");
				return;
			}
			m_State = GIFstate::ReceiveTag;
			return;
		}

		// PRIM field enabled
		if (m_LastTag.enable_prim) {
			m_GS->WritePrim(m_LastTag.prim_data);
		}

		m_State = GIFstate::ReceiveData;
		m_CurrentReg = 0;
		return;
	}

	switch (m_LastTag.data_format) {
		case DataFormat::Packed: {
			u64 reg_field = static_cast<u64>((m_RecentGIFtag >> 64) & 0xffffffffffffffff);
			u8 reg_idx = (reg_field >> (m_CurrentReg * 4)) & 0b1111;

			// special registers
			switch (reg_idx) {
				// PRIM
				case 0x0: {
					m_GS->WritePrim(static_cast<u16>(qword & 0x3ff));
					break;
				}

				// RGBA
				case 0x1: {
					error_log("RGBA unimplemented");
					exit(1);
				}

				// STQ
				case 0x2: {
					error_log("STQ unimplemented");
					exit(1);
				}

				// UV
				case 0x3: {
					error_log("UV unimplemented");
					exit(1);
				}

				// XYZF2/XYZF3
				case 0x4: {
					error_log("XYZF2/XYZF3 unimplemented");
					exit(1);
				}

				// XYZ2/XYZ3
				case 0x5: {
					error_log("XYZ2/XYZ3 unimplemented");
					exit(1);
				}

				// FOG
				case 0xa: {
					error_log("FOG unimplemented");
					exit(1);
				}

				// output data to other register address
				case 0xe: {
					u8 new_idx = static_cast<u8>((qword >> 64) & 0x7f);
					m_GS->WriteInternalReg(static_cast<GraphicsSynthesizer::InternalRegisterID>(new_idx), static_cast<u64>(qword & 0xffffffffffffffff));
					break;
				}

				// low 64-bit -> GS register directly
				default: {
					m_GS->WriteInternalReg(static_cast<GraphicsSynthesizer::InternalRegisterID>(reg_idx), static_cast<u64>(qword & 0xffffffffffffffff));
					break;
				}
			}

			m_CurrentReg++;
			if (m_CurrentReg == m_LastTag.nregs) {
				m_CurrentReg = 0;
				m_LastTag.nloop--;

				if (m_LastTag.nloop == 0) {
					m_State = GIFstate::ReceiveTag;
					if (m_LastTag.eop) {
						debug_log("end of packet");
						m_ActivePath = ActivePath::None;
						return;
					}
				}
			}

			break;
		}

		case DataFormat::Image1:
		case DataFormat::Image2: {
			u64 hi = static_cast<u64>((qword >> 64) & 0xffffffffffffffff);
			u64 lo = static_cast<u64>(qword & 0xffffffffffffffff);
			
			m_GS->WriteInternalReg(GraphicsSynthesizer::HWREG, lo);
			m_GS->WriteInternalReg(GraphicsSynthesizer::HWREG, hi);
			break;
		}

		default: {
			error_log("unknown data format {}", (u8)m_LastTag.data_format);
			exit(1);
		}
	}
}