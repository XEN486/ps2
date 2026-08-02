#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <cstdint>
#include <vector>
#include <filesystem>
#include <fstream>

#include "utils.hpp"

// --- forward declarations ---
namespace EmotionEngine::Core { class JitBackend; }
namespace EmotionEngine::DMA { class DMAC; }

#define RDRAM_LAST_ADDR 0x01ffffff

/// @brief The EmotionEngine's physical memory map.
class Memory {
public:
	/// @brief Initializes and allocates everything necessary for the physical memory map. This function must be called.
	/// @param backend Pointer to the JIT backend.
	/// @param dmac Pointer to the EmotionEngine's DMAC.
	static void Initialize(EmotionEngine::Core::JitBackend* backend, EmotionEngine::DMA::DMAC* dmac) {
		m_JitBackend = backend;
		m_DMAC = dmac;
		m_Memory = static_cast<u8*>(calloc(1, 0x2000000)); // 32MiB
	}

	/// @brief Reads a 64-bit value from the specified address
	/// @param address Address to read from.
	/// @return Value at the address.
	static u64 ReadVirtualMemory64(u32 address);

	/// @brief Writes a 64-bit value to the specified address
	/// @param address Address to write to.
	/// @param dword Value to write.
	static void WriteVirtualMemory64(u32 address, u64 dword);

	/// @brief Reads a 32-bit value from the specified address
	/// @param address Address to read from.
	/// @return Value at the address.
	static u32 ReadVirtualMemory32(u32 address);

	/// @brief Writes a 32-bit value to the specified address
	/// @param address Address to write to.
	/// @param dword Value to write.
	static void WriteVirtualMemory32(u32 address, u32 word);

	/// @brief Releases the resources used by the memory map. Call this before terminating.
	static void Release() {
		free(m_Memory);
	}

	inline static u8* m_Memory;

private:
	inline static EmotionEngine::Core::JitBackend* m_JitBackend;
	inline static EmotionEngine::DMA::DMAC* m_DMAC;
	inline static bool m_ExposeBootROM;
};

#endif