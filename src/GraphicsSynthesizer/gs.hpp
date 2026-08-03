#ifndef GRAPHICSSYNTHESIZER_GS_HPP
#define GRAPHICSSYNTHESIZER_GS_HPP

#include "../utils.hpp"

/// @brief The PlayStation2's graphics unit.
namespace GraphicsSynthesizer {
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

	enum PrimitiveType : u8 {
		Point,
		Line,
		LineStrip,
		Triangle,
		TriangleStrip,
		TriangleFan,
		Sprite,
		Reserved
	};

	/// @brief Structure describing the current GS primitive.
	struct RegPrim {
		PrimitiveType type;
		bool gouraud;
		bool textured;
		bool fog;
		bool alpha_blending;
		bool antialiasing;
		bool uv;
		bool context2;
		bool fix_fragment;
	};

	/// @brief Structure containing all the internal GS registers.
	struct InternalRegisters {
		RegPrim prim;
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

	/// @brief The PlayStation2's graphics unit.
	class GS {
	public:
		void WritePrim(u16 value) {
			debug_log("GS(PRIM) <- {:04x}", value);
		}

		void WriteInternalReg(InternalRegisterID id, u64 dword) {
			debug_log("GS({:02x}) <- {:016x}", (u8)id, dword);
		}

	private:
		InternalRegisters m_IRegs;
		PrivilegedRegisters m_PRegs;
	};
}

#endif