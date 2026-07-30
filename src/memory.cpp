#include "memory.hpp"
#include "elf.hpp"
#include "utils.hpp"
#include "emotion/emotion.hpp"

u32 Memory::ReadVirtualMemory32(u32 address) {
	address = VirtualToPhysical(address);

	// main memory
	if (address <= 0x1ffffff) {
		return *(reinterpret_cast<u32*>(&m_Memory[address]));
	}

	error_log("unknown physical address {:08x}", address);
	return 0;
}

void Memory::WriteVirtualMemory32(u32 address, u32 dword) {
	address = VirtualToPhysical(address);

	// main memory
	if (address <= 0x1ffffff) {
		*(reinterpret_cast<u32*>(&m_Memory[address])) = dword;
		m_JitBackend->Invalidate(address);
		return;
	}

	error_log("unknown physical address {:08x}", address);
}