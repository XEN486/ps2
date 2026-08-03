#ifndef EMOTIONENGINE_MEMORY_HPP
#define EMOTIONENGINE_MEMORY_HPP

#include <cstdint>
#include <vector>
#include <filesystem>
#include <fstream>

#include "../../utils.hpp"

#define RDRAM_LAST_ADDR 0x01ffffff

namespace EmotionEngine {
	// --- forward declarations ---
	namespace Core { class JitBackend; }
	namespace DMA { class DMAC; }

	/// @brief The EmotionEngine's virtual memory map.
	class Memory {
	public:
		/// @brief Initializes and allocates everything necessary for the physical memory map.
		/// @param backend Pointer to the JIT backend.
		/// @param dmac Pointer to the EmotionEngine's DMAC.
		void Initialize(Core::JitBackend* backend, DMA::DMAC* dmac) {
			m_JitBackend = backend;
			m_DMAC = dmac;
			rdram = static_cast<u8*>(calloc(1, 0x2000000)); // 32MiB
		}

		/// @brief Reads a 64-bit value from the specified address
		/// @param address Address to read from.
		/// @return Value at the address.
		u64 ReadVirtualMemory64(u32 address);

		/// @brief Writes a 64-bit value to the specified address
		/// @param address Address to write to.
		/// @param dword Value to write.
		void WriteVirtualMemory64(u32 address, u64 dword);

		/// @brief Reads a 32-bit value from the specified address
		/// @param address Address to read from.
		/// @return Value at the address.
		u32 ReadVirtualMemory32(u32 address);

		/// @brief Writes a 32-bit value to the specified address
		/// @param address Address to write to.
		/// @param dword Value to write.
		void WriteVirtualMemory32(u32 address, u32 word);

		/// @brief Releases the resources used by the memory map. Called by EmotionEngine::EE::Reset();
		void Release() {
			free(rdram);
		}

		u8* rdram;

	private:
		EmotionEngine::Core::JitBackend* m_JitBackend;
		EmotionEngine::DMA::DMAC* m_DMAC;
		bool m_ExposeBootROM;
	};
}


#endif