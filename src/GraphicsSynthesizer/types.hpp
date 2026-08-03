#ifndef GRAPHICSSYNTHESIZER_TYPES_HPP
#define GRAPHICSSYNTHESIZER_TYPES_HPP

#include "../utils.hpp"

namespace GraphicsSynthesizer {
	enum class PrimitiveType {
		Point,
		Line,
		LineStrip,
		Triangle,
		TriangleStrip,
		TriangleFan,
		Sprite,
		Reserved
	};

	enum class PixelStorageFormat {
		PSMCT32		= 0b000000,
		PSMCT24		= 0b000001,
		PSMCT16		= 0b000010,
		PSMCT16S	= 0b001010,
		PSMZ32		= 0b110000,
		PSMZ24		= 0b110001,
		PSMZ16		= 0b110010,
		PSMZ16S		= 0b111010,
	};

	/// @brief unsigned fixed point 12.4 number
	struct ufp12_4 {
		ufp12_4() : ufp12_4(0) {}
		ufp12_4(u16 data) {
			integer = (data >> 4) & 0xfff;
			frac = data & 0xf;
		}

		inline float get() {
			return (float)integer + ((float)frac / 16);
		}

		u16 integer;
		u8 frac;
	};
}

#endif