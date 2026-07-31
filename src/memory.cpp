#include "memory.hpp"
#include "elf.hpp"
#include "utils.hpp"
#include "emotion/emotion.hpp"

u32 Memory::ReadVirtualMemory32(u32 address) {
	// main memory
	if (address <= RDRAM_END) {
		return *(reinterpret_cast<u32*>(&m_Memory[address]));
	}

	error_log("unknown physical address {:08x}", address);
	return 0;
}

void Memory::WriteVirtualMemory32(u32 address, u32 dword) {
	// main memory
	if (address <= RDRAM_END) {
		*(reinterpret_cast<u32*>(&m_Memory[address])) = dword;
		m_JitBackend->Invalidate(address);
		return;
	}

	error_log("unknown physical address {:08x}", address);
}

u64 Memory::ReadVirtualMemory64(u32 address) {
	u32 lo = ReadVirtualMemory32(address);
	u32 hi = ReadVirtualMemory32(address + 1);
	return (hi << 32) | lo;
}

void Memory::WriteVirtualMemory64(u32 address, u64 qword) {
	WriteVirtualMemory32(address, qword & 0xffffffff);
	WriteVirtualMemory32(address + 1, (qword >> 32) & 0xffffffff);
}