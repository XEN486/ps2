#ifndef GRAPHICSSYNTHESIZER_REGISTERS_HPP
#define GRAPHICSSYNTHESIZER_REGISTERS_HPP

#include "types.hpp"
#include "../utils.hpp"

namespace GraphicsSynthesizer {
	/// @brief PRIM
	struct RegPrim {
		RegPrim() : RegPrim(0) {}
		RegPrim(u16 data) {
			type			= static_cast<PrimitiveType>(data & 0b111);
			gouraud			= (data >> 3) & 1;
			textured		= (data >> 4) & 1;
			fog				= (data >> 5) & 1;
			alpha_blending	= (data >> 6) & 1;
			antialiasing	= (data >> 7) & 1;
			uv				= (data >> 8) & 1;
			context2		= (data >> 9) & 1;
			fix_fragment	= (data >> 10) & 1;

			initial_kick = true;
		}

		// necessary vertex count for the next drawing kick
		u8 GetVertexCount() {
			switch (type) {
				case PrimitiveType::Point:			return 1;
				case PrimitiveType::Line:			return 2;
				case PrimitiveType::LineStrip:		return initial_kick ? 2 : 1;
				case PrimitiveType::Triangle:		return 3;
				case PrimitiveType::TriangleStrip:	return initial_kick ? 3 : 1;
				case PrimitiveType::TriangleFan:	return initial_kick ? 3 : 1;
				case PrimitiveType::Sprite:			return 2;
			}
		}

		PrimitiveType type;
		bool gouraud;
		bool textured;
		bool fog;
		bool alpha_blending;
		bool antialiasing;
		bool uv;
		bool context2;
		bool fix_fragment;

		// not part of the data but it is easier to store here
		bool initial_kick;
	};

	/// @brief FRAME_1/2
	struct RegFrame {
		RegFrame() : RegFrame(0) {}
		RegFrame(u64 data) {
			fbp = data & 0b111111111;
			fbw = (data >> 16) & 0b111111;
			psm = static_cast<PixelStorageFormat>((data >> 24) & 0b111111);
			fbmsk = (data >> 32) & 0xffffffff;
		}

		u16 fbp;
		u8 fbw;
		PixelStorageFormat psm;
		u32 fbmsk;
	};

	/// @brief XYOFFSET_1/2
	struct RegXYOffset {
		RegXYOffset() : RegXYOffset(0) {}
		RegXYOffset(u64 data) {
			x = data & 0xffff;
			y = (data >> 32) & 0xffff;
		}

		u16 x;
		u16 y;
	};
	
	/// @brief SCISSOR_1/2
	struct RegScissor {
		RegScissor() : RegScissor(0) {}
		RegScissor(u64 data) {
			x0 = data & 0x7ff;
			x1 = (data >> 16) & 0x7ff;
			y0 = (data >> 32) & 0x7ff;
			y1 = (data >> 48) & 0x7ff;
		}

		u16 x0;
		u16 x1;
		u16 y0;
		u16 y1;
	};

	/// @brief RGBAQ
	struct RegRGBAQ {
		RegRGBAQ() : RegRGBAQ(0, true) {}
		RegRGBAQ(u64 data, bool set_q) {
			red = data & 0xff;
			green = (data >> 8) & 0xff;
			blue = (data >> 16) & 0xff;
			alpha = (data >> 24) & 0xff;
			if (set_q) q = (data >> 32) & 0xffffffff;
		}

		u8 red;
		u8 green;
		u8 blue;
		u8 alpha;
		u32 q;
	};

	struct AnyXYZ {
		ufp12_4 x;
		ufp12_4 y;
		u32 z;
	};

	/// @brief XYZF2/3
	struct RegXYZF : public AnyXYZ {
		RegXYZF() : RegXYZF(0) {}
		RegXYZF(u64 data) {
			x = data & 0xffff;
			y = (data >> 16) & 0xffff;
			z = (data >> 32) & 0xfff;
		}
	};

	/// @brief XYZ2/3
	struct RegXYZ : public AnyXYZ {
		RegXYZ() : RegXYZ(0) {}
		RegXYZ(u64 data) {
			x = data & 0xffff;
			y = (data >> 16) & 0xffff;
			z = (data >> 32) & 0xffff;
		}
	};
}

#endif