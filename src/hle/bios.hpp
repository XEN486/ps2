#ifndef HLE_BIOS_HPP
#define HLE_BIOS_HPP

#include "../emotion/emotion.hpp"

namespace HLE {
	enum EESyscalls {
		InitMainThread	= 0x3c,
		InitHeap		= 0x3d,
		FlushCache		= 0x64,
	};

	// HLE BIOS
	class Bios {
	public:
		static void EESysCall(EmotionEngine::MIPS::R5900* r5900);

	private:
		inline static u32 m_ThreadSP;
		inline static u32 m_HeapEnd;
	};
}

#endif