#ifndef GRAPHICSSYNTHESIZER_GS_HPP
#define GRAPHICSSYNTHESIZER_GS_HPP

#include "../utils.hpp"
#include "registers.hpp"

#include <vector>

/// @brief The PlayStation2's graphics unit.
namespace GraphicsSynthesizer {
	enum class VideoMode {
		NTSC,
		PAL,
	};

	struct VideoModeTiming {
		u32 total_scanlines;
		u32 visible_scanlines;

		size_t cycles_per_scanline;
		size_t hblank_start;
	};

	static void CalculateTiming(VideoModeTiming& timing, VideoMode mode) {
		switch (mode) {
			case VideoMode::NTSC: {
				timing.total_scanlines = 262;
				timing.visible_scanlines = 240;
				timing.cycles_per_scanline = 9370;
				timing.hblank_start = 7764; // 147.456MHz * 10.9us = 1606 cycles, 9370-1606 = 7764 cycles.
				break;
			}

			case VideoMode::PAL: {
				timing.total_scanlines = 312;
				timing.visible_scanlines = 286;
				timing.cycles_per_scanline = 9436;
				timing.hblank_start = 7667; // 147.456MHz * 12us = 1769 cycles, 9436 - 1769 = 7667
			}
		}
	}

	enum InternalRegisterID : u8 {
		PRIM		= 0x00,
		RGBAQ		= 0x01,
		ST			= 0x02,
		UV			= 0x03,
		XYZF2		= 0x04,
		XYZ2		= 0x05,
		TEX0_1		= 0x06,
		TEX0_2		= 0x07,
		CLAMP_1		= 0x08,
		CLAMP_2		= 0x09,
		FOG			= 0x0a,
		XYZF3		= 0x0c,
		XYZ3		= 0x0d,
		TEX1_1		= 0x14,
		TEX1_2		= 0x15,
		TEX2_1		= 0x16,
		TEX2_2		= 0x17,
		XYOFFSET_1	= 0x18,
		XYOFFSET_2	= 0x19,
		PRMODECONT	= 0x1a,
		PRMODE		= 0x1b,
		TEXCLUT		= 0x1c,
		SCANMSK		= 0x22,
		MIPTBP1_1	= 0x34,
		MIPTBP1_2	= 0x35,
		MIPTBP2_1	= 0x36,
		MIPTBP2_2	= 0x37,
		TEXA		= 0x3b,
		FOGCOL		= 0x3d,
		TEXFLUSH	= 0x3f,
		SCISSOR_1	= 0x40,
		SCISSOR_2	= 0x41,
		ALPHA_1		= 0x42,
		ALPHA_2		= 0x43,
		DIMX		= 0x44,
		DTHE		= 0x45,
		COLCLAMP	= 0x46,
		TEST_1		= 0x47,
		TEST_2		= 0x48,
		PABE		= 0x49,
		FBA_1		= 0x4a,
		FBA_2		= 0x4b,
		FRAME_1		= 0x4c,
		FRAME_2		= 0x4d,
		ZBUF_1		= 0x4e,
		ZBUF_2		= 0x4f,
		BITBLTBUF	= 0x50,
		TRXPOS		= 0x51,
		TRXREG		= 0x52,
		TRXDIR		= 0x53,
		HWREG		= 0x54,
		SIGNAL		= 0x60,
		FINISH		= 0x61,
		LABEL		= 0x62,
	};

	enum PrivilegedRegisterID : u16 {
		PMODE		= 0x0000,
		SMODE1		= 0x0010,
		SMODE2		= 0x0020,
		SRFSH		= 0x0030,
		SYNCH1		= 0x0040,
		SYNCH2		= 0x0050,
		SYNCV		= 0x0060,
		DISPFB1		= 0x0070,
		DISPLAY1	= 0x0080,
		DISPFB2		= 0x0090,
		DISPLAY2	= 0x00a0,
		EXTBUF		= 0x00b0,
		EXTDATA		= 0x00c0,
		EXTWRITE	= 0x00d0,
		BGCOLOR		= 0x00e0,
		GS_CSR		= 0x1000,
		GS_IMR		= 0x1010,
		BUSDIR		= 0x1040,
		SIGLBLID	= 0x1080,
	};

	/// @brief Structure containing the internal GS registers for one of the drawing contexts.
	struct DrawingContext {
		RegFrame frame;
		RegXYOffset xyoffset;
		RegScissor scissor;
	};

	/// @brief Structure containing all the internal GS registers.
	struct InternalRegisters {
		RegPrim prim;
		RegRGBAQ rgbaq;

		DrawingContext ctx1;
		DrawingContext ctx2;
	};

	/// @brief Structure containing all the privileged GS registers.
	struct PrivilegedRegisters {
		u64 pmode;
		u64 smode1;
		u64 smode2;
		u64 srfsh;
		u64 synch1;
		u64 synch2;
		u64 syncv;
		u64 dispfb1;
		u64 display1;
		u64 dispfb2;
		u64 display2;
		u64 extbuf;
		u64 extdata;
		u64 extwrite;
		u64 bgcolor;
		u64 gs_csr;
		u64 gs_imr;
		u64 busdir;
		u64 siglblid;
	};
	
	struct Vertex {
		AnyXYZ xyz;
		RegRGBAQ rgbaq;
	};

	/// @brief The PlayStation2's graphics unit.
	class GS {
	public:
		void Tick();

		void Reset() {
			CalculateTiming(m_Timing, VideoMode::NTSC); // assume NTSC timings for now
			m_IRegs.prim.initial_kick = true;
			m_ScanlineCycles = 0;
			m_Scanline = 0;

			m_HBlank = false;
			m_VBlank = false;
			m_LeftHBlank = false;
			m_LeftVBlank = false;
			m_EnteredHBlank = false;
			m_EnteredVBlank = false;
		}

		bool GetEnteredHBlank() {
			bool hblank = m_EnteredHBlank;
			m_EnteredHBlank = false;
			return hblank;
		}

		bool GetEnteredVBlank() {
			bool vblank = m_EnteredVBlank;
			m_EnteredVBlank = false;
			return vblank;
		}

		bool GetLeftHBlank() {
			bool hblank = m_LeftHBlank;
			m_LeftHBlank = false;
			return hblank;
		}

		bool GetLeftVBlank() {
			bool vblank = m_LeftVBlank;
			m_LeftVBlank = false;
			return vblank;
		}

		u32 ReadLoCSR() const {
			return m_PRegs.gs_csr & 0xffffffff;
		}

		u32 ReadLoSIGLBLID() const {
			return m_PRegs.siglblid & 0xffffffff;
		}

		u32 ReadHiCSR() const {
			return (m_PRegs.gs_csr >> 32) & 0xffffffff;
		}

		u32 ReadHiSIGLBLID() const {
			return (m_PRegs.siglblid >> 32) & 0xffffffff;
		}

		void WritePrim(u16 value);
		void WritePrivilegedRegEE(u32 address, u32 word);
		void WritePrivilegedReg(u32 address, u64 dword);
		void WriteInternalReg(InternalRegisterID id, u64 dword);

	private:
		void WriteXYZ(AnyXYZ xyz, bool three);

		void DrawingKick();

	private:
		InternalRegisters m_IRegs;
		PrivilegedRegisters m_PRegs;

		std::vector<Vertex> m_VertexQueue;
		Vertex m_InitialVertex;
		Vertex m_LastVertex;

		VideoModeTiming m_Timing;
		size_t m_ScanlineCycles = 0;
		size_t m_Scanline = 0;

		bool m_HBlank = false;			// true during hblank
		bool m_VBlank = false;			// true during vblank

		bool m_EnteredHBlank = false;	// true until GetEnteredHBlank() is ran or hblank ends
		bool m_EnteredVBlank = false;	// true until GetEnteredVBlank() is ran or vblank ends

		bool m_LeftHBlank = false;		// true until GetLeftHBlank() is ran or hblank starts (only starts after first hblank)
		bool m_LeftVBlank = false;		// true until GetLeftHBlank() is ran or hblank starts (only starts after first vblank)
	};
}

#endif