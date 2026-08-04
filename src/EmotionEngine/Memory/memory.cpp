#include "memory.hpp"
#include "../../elf.hpp"
#include "../../utils.hpp"

#include "../emotion.hpp"
#include "../DMAC/dmac.hpp"

using namespace EmotionEngine;

u32 Memory::ReadVirtualMemory32(u32 address) {
	if (address >= 0x70000000 && address <= 0x70003fff) {
		return *(reinterpret_cast<u32*>(&scratchpad[address - 0x70000000]));
	}

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

	// readable GS privileged registers
	if (address == 0x12001000) return m_GS->ReadLoCSR();
	if (address == 0x12001004) return m_GS->ReadHiCSR();
	if (address == 0x12001080) return m_GS->ReadLoSIGLBLID();
	if (address == 0x12001084) return m_GS->ReadHiSIGLBLID();

	// undocumented registers
	if (address == 0x1f803204) return 0x00000020;

	// BIOS
	if (address >= 0x1fc00000 && address <= 0x1fffffff) {
		return *(reinterpret_cast<u32*>(&bios[address - 0x1fc00000]));
	}

	error_log("unknown physical address {:08x}", address);
	return 0;
}

void Memory::WriteVirtualMemory32(u32 address, u32 word) {
	if (address >= 0x70000000 && address <= 0x70003fff) {
		*(reinterpret_cast<u32*>(&scratchpad[address - 0x70000000])) = word;
		return;
	}

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

	// GS privileged registers
	if (address >= 0x12000000 && address <= 0x12001080) {
		m_GS->WritePrivilegedRegEE(address, word);
		return;
	}

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

u16 Memory::ReadVirtualMemory16(u32 address) {
	return ReadVirtualMemory32(address) & 0xffff;
}

void Memory::WriteVirtualMemory16(u32 address, u16 hword) {
	WriteVirtualMemory32(address, hword);
}

u8 Memory::ReadVirtualMemory8(u32 address) {
	return ReadVirtualMemory32(address) & 0xff;
}

void Memory::WriteVirtualMemory8(u32 address, u8 byte) {
	WriteVirtualMemory32(address, byte);
}

void Memory::LoadBIOS(std::filesystem::path path) {
	std::ifstream file;
	file.open(path, std::ios::binary | std::ios::ate);
	
	size_t size = file.tellg();
	if (size != 0x400000) {
		error_log("BIOS is not valid");
		exit(1);
	}

	file.seekg(0);
	file.read(reinterpret_cast<char*>(bios), size);
	file.close();
}