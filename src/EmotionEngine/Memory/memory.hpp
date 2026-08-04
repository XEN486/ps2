#ifndef EMOTIONENGINE_MEMORY_HPP
#define EMOTIONENGINE_MEMORY_HPP

#include <cstdint>
#include <vector>
#include <filesystem>
#include <fstream>

#include "../../utils.hpp"

#define RDRAM_LAST_ADDR 0x01ffffff

namespace GraphicsSynthesizer { class GS; }

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
		void Initialize(Core::JitBackend* backend, DMA::DMAC* dmac, GraphicsSynthesizer::GS* gs) {
			m_JitBackend = backend;
			m_DMAC = dmac;
			m_GS = gs;
			
			scratchpad = static_cast<u8*>(calloc(1, 0x4000)); // 4KiB
			rdram = static_cast<u8*>(calloc(1, 0x2000000)); // 32MiB
			bios = static_cast<u8*>(calloc(1, 0x400000)); // 4MiB
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
		/// @param word Value to write.
		void WriteVirtualMemory32(u32 address, u32 word);

		/// @brief Reads a 16-bit value from the specified address
		/// @param address Address to read from.
		/// @return Value at the address.
		u16 ReadVirtualMemory16(u32 address);

		/// @brief Writes a 16-bit value to the specified address
		/// @param address Address to write to.
		/// @param hword Value to write.
		void WriteVirtualMemory16(u32 address, u16 hword);

		/// @brief Reads a 8-bit value from the specified address
		/// @param address Address to read from.
		/// @return Value at the address.
		u8 ReadVirtualMemory8(u32 address);

		/// @brief Writes a 8-bit value to the specified address
		/// @param address Address to write to.
		/// @param byte Value to write.
		void WriteVirtualMemory8(u32 address, u8 byte);

		/// @brief Releases the resources used by the memory map. Called by EmotionEngine::EE::Reset();
		void Release() {
			free(rdram);
			free(bios);
		}

		void LoadBIOS(std::filesystem::path path);

		u8* rdram;
		u8* scratchpad;
		u8* bios;

	private:
		Core::JitBackend* m_JitBackend;
		DMA::DMAC* m_DMAC;
		GraphicsSynthesizer::GS* m_GS;
	};
}

#endif