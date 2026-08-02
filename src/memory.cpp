#include "memory.hpp"
#include "elf.hpp"
#include "utils.hpp"

#include "EmotionEngine/emotion.hpp"
#include "DMAC/dmac.hpp"

u32 Memory::ReadVirtualMemory32(u32 address) {
	// main memory
	if (address <= RDRAM_LAST_ADDR) {
		return *(reinterpret_cast<u32*>(&m_Memory[address]));
	}

	// dmac addresses
	if (address >= 0x10008000 && address <= 0x1000f590) {
		return m_DMAC->ReadMemory32(address);
	}

	error_log("unknown physical address {:08x}", address);
	return 0;
}

void Memory::WriteVirtualMemory32(u32 address, u32 word) {
	// main memory
	if (address <= RDRAM_LAST_ADDR) {
		*(reinterpret_cast<u32*>(&m_Memory[address])) = word;
		m_JitBackend->Invalidate(address);
		return;
	}

	// dmac addresses
	if (address >= 0x10008000 && address <= 0x1000f590) {
		m_DMAC->WriteMemory32(address, word);
		return;
	}

	error_log("{:08x} -> unknown physical address {:08x}", word, address);
}

u64 Memory::ReadVirtualMemory64(u32 address) {
	u32 lo = ReadVirtualMemory32(address);
	u32 hi = ReadVirtualMemory32(address + 1);
	return ((u64)hi << 32) | lo;
}

void Memory::WriteVirtualMemory64(u32 address, u64 dword) {
	WriteVirtualMemory32(address, dword & 0xffffffff);
	WriteVirtualMemory32(address + 1, (dword >> 32) & 0xffffffff);
}