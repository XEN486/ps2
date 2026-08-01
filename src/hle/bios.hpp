#ifndef HLE_BIOS_HPP
#define HLE_BIOS_HPP

#include "../emotion/emotion.hpp"

namespace HLE {
	enum EESyscalls {
		SetGsCrt		= 0x02,
		InitMainThread	= 0x3c,
		InitHeap		= 0x3d,
		FlushCache		= 0x64,
		GsSetIMR		= 0x71,
	};

	// HLE BIOS
	class Bios {
	public:
		static void EESysCall(EmotionEngine::Core::R5900* r5900);

	private:
		inline static u32 m_ThreadSP;
		inline static u32 m_HeapEnd;
	};
}

#endif