#ifndef IOP_MEMORY_HPP
#define IOP_MEMORY_HPP

#include "../../utils.hpp"
#include <filesystem>


namespace IOProcessor {
	static constexpr const size_t KiB = 1024;
	static constexpr const size_t MiB = 1024 * KiB;

	class IOP;
	class Memory {
	public:
		/// @brief Initializes and allocates everything necessary for the memory map.
		/// @param iop Pointer to the IOP
		void Initialize(IOP* iop);

		/// @brief Reads a 32-bit value from the specified address.
		/// @param address Address to read from.
		/// @return Value at the address.
		u32 ReadVirtualMemory32(u32 address);

		/// @brief Writes a 32-bit value to the specified address.
		/// @param address Address to write to.
		/// @param word Value to write.
		void WriteVirtualMemory32(u32 address, u32 word);

		/// @brief Reads a 16-bit value from the specified address.
		/// @param address Address to read from.
		/// @return Value at the address.
		u16 ReadVirtualMemory16(u32 address);

		/// @brief Writes a 16-bit value to the specified address.
		/// @param address Address to write to.
		/// @param word Value to write.
		void WriteVirtualMemory16(u32 address, u16 hword);

		/// @brief Reads an 8-bit value from the specified address.
		/// @param address Address to read from.
		/// @return Value at the address.
		u8 ReadVirtualMemory8(u32 address);

		/// @brief Writes an 8-bit value to the specified address.
		/// @param address Address to write to.
		/// @param word Value to write.
		void WriteVirtualMemory8(u32 address, u8 byte);
		
		/// @brief Releases the resources used by the memory map. Called by IOP::Release();
		void Release();

		void LoadBIOS(std::filesystem::path path);

		u32 VirtualToPhysical(u32 vaddr) {
			return vaddr & 0x1fffffff;
		}

	public:
		u8* ram;
		u8* bios;

	private:
		IOP* m_IOP;
	};

}

#endif