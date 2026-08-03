#include "memory.hpp"
#include "../../elf.hpp"
#include "../../utils.hpp"

#include "../emotion.hpp"
#include "../DMAC/dmac.hpp"

using namespace EmotionEngine;

u32 Memory::ReadVirtualMemory32(u32 address) {
	address &= 0x1fffffff;

	// main memory
	if (address <= RDRAM_LAST_ADDR) {
		return *(reinterpret_cast<u32*>(&rdram[address]));
	}

	// dmac addresses
	if (address >= 0x10008000 && address <= 0x1000e060) {
		return m_DMAC->ReadMemory32(address);
	}

	// D_ENABLER
	if (address == 0x1000f520) return m_DMAC->ReadMemory32(address);

	error_log("unknown physical address {:08x}", address);
	return 0;
}

void Memory::WriteVirtualMemory32(u32 address, u32 word) {
	address &= 0x1fffffff;
	
	// main memory
	if (address <= RDRAM_LAST_ADDR) {
		*(reinterpret_cast<u32*>(&rdram[address])) = word;
		m_JitBackend->Invalidate(address);
		return;
	}

	// dmac addresses
	if (address >= 0x10008000 && address <= 0x1000e060) {
		m_DMAC->WriteMemory32(address, word);
		return;
	}

	// D_ENABLEW
	if (address == 0x1000f590) return m_DMAC->WriteMemory32(address, word);

	error_log("{:08x} -> unknown physical address {:08x}", word, address);
}

u64 Memory::ReadVirtualMemory64(u32 address) {
	u32 lo = ReadVirtualMemory32(address);
	u32 hi = ReadVirtualMemory32(address + 4);
	return ((u64)hi << 32) | lo;
}

void Memory::WriteVirtualMemory64(u32 address, u64 dword) {
	WriteVirtualMemory32(address, dword & 0xffffffff);
	WriteVirtualMemory32(address + 4, (dword >> 32) & 0xffffffff);
}