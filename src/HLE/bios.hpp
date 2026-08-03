#ifndef HLE_BIOS_HPP
#define HLE_BIOS_HPP

#include "../EmotionEngine/emotion.hpp"

/// @brief Parts of the system emulated at a higher-level.
namespace HLE {
	enum EESyscalls {
		SetGsCrt		= 0x02,
		InitMainThread	= 0x3c,
		InitHeap		= 0x3d,
		FlushCache		= 0x64,
		GsSetIMR		= 0x71,
	};

	/// @brief High-level emulation of the PlayStation2's BIOS.
	class BIOS {
	public:
		/// @brief Emulates a system call from the EmotionEngine.
		/// @param r5900 Pointer to the MIPS CPU state.
		static void EESysCall(EmotionEngine::Core::R5900* r5900);

	private:
		inline static u32 m_ThreadSP;
		inline static u32 m_HeapEnd;
	};
}

#endif