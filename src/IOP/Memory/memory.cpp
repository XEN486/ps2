#include "memory.hpp"
#include "../iop.hpp"

#include <fstream>
using namespace IOProcessor;

void Memory::Initialize(IOP* iop) {
	m_IOP = iop;

	// allocate memory for stuff on IOP bus
	ram			= static_cast<u8*>(malloc(2 * MiB));
	bios		= static_cast<u8*>(malloc(4 * MiB));
}

u32 Memory::ReadVirtualMemory32(u32 address) {
	// virtual -> physical
	address = VirtualToPhysical(address);

	// RAM
	if (address < 2 * MiB) {
		return *reinterpret_cast<u32*>(ram + address);
	}

	// BIOS
	if (address >= 0x1fc00000 && address <= 0x1fffffff) {
		u32 index = address - 0x1fc00000;
		return *reinterpret_cast<u32*>(bios + index);
	}

	// INTC
	if (address == 0x1f801070) return m_IOP->GetINTC().GetSTAT();
	if (address == 0x1f801074) return m_IOP->GetINTC().GetMASK();
	if (address == 0x1f801078) return m_IOP->GetINTC().GetCTRL();

	// DMAC
	if (address >= 0x1f801080 && address <= 0x1f8010ff) return m_IOP->GetDMAC().Read(address);
	if (address >= 0x1f801500 && address <= 0x1f8015ff) return m_IOP->GetDMAC().Read(address);

	error_log("32-bit read <- unknown address {:08x}", address);
	return 0;
}

void Memory::WriteVirtualMemory32(u32 address, u32 word) {
	// virtual -> physical
	address = VirtualToPhysical(address);

	// RAM
	if (address < 2 * MiB) {
		*reinterpret_cast<u32*>(ram + address) = word;
		m_IOP->GetBackend()->Invalidate(address);
		return;
	}

	// INTC
	if (address == 0x1f801070) return m_IOP->GetINTC().SetSTAT(word);
	if (address == 0x1f801074) return m_IOP->GetINTC().SetMASK(word);
	if (address == 0x1f801078) return m_IOP->GetINTC().SetCTRL(word);

	// DMAC
	if (address >= 0x1f801080 && address <= 0x1f8010ff) return m_IOP->GetDMAC().Write(address, word);
	if (address >= 0x1f801500 && address <= 0x1f8015ff) return m_IOP->GetDMAC().Write(address, word);

	error_log("32-bit write {:08x} -> unknown address {:08x}", word, address);
}

u16 Memory::ReadVirtualMemory16(u32 address) {
	// virtual -> physical
	address = VirtualToPhysical(address);

	// RAM
	if (address < 2 * MiB) {
		return *reinterpret_cast<u16*>(ram + address);
	}

	// BIOS
	if (address >= 0x1fc00000 && address <= 0x1fffffff) {
		u32 index = address - 0x1fc00000;
		return *reinterpret_cast<u16*>(bios + index);
	}

	// INTC
	if (address == 0x1f801070) return m_IOP->GetINTC().GetSTAT() & 0xffff;
	if (address == 0x1f801074) return m_IOP->GetINTC().GetMASK() & 0xffff;
	if (address == 0x1f801078) return m_IOP->GetINTC().GetCTRL() & 0xffff;

	// SPU stubs
	if (address == 0x1f801da6) return stub_1f801da6;
	if (address == 0x1f801da8) return stub_1f801da8;
	if (address == 0x1f801daa) return stub_1f801daa;
	if (address == 0x1f801dae) return 0;
	if (address == 0x1f801dac) return 0;

	error_log("16-bit read <- unknown address {:08x}", address);
	return 0;
}

void Memory::WriteVirtualMemory16(u32 address, u16 hword) {
	// virtual -> physical
	address = VirtualToPhysical(address);

	// RAM
	if (address < 2 * MiB) {
		*reinterpret_cast<u16*>(ram + address) = hword;
		m_IOP->GetBackend()->Invalidate(address);
		return;
	}

	// INTC
	if (address == 0x1f801070) return m_IOP->GetINTC().SetSTAT(hword);
	if (address == 0x1f801074) return m_IOP->GetINTC().SetMASK(hword);
	if (address == 0x1f801078) return m_IOP->GetINTC().SetCTRL(hword);

	// SPU stubs
	if (address == 0x1f801da6) { stub_1f801da6 = hword; return; }
	if (address == 0x1f801da8) { stub_1f801da8 = hword; return; }
	if (address == 0x1f801daa) { stub_1f801daa = hword; return; }

	error_log("16-bit write {:04x} -> unknown address {:08x}", hword, address);
}

u8 Memory::ReadVirtualMemory8(u32 address) {
	// virtual -> physical
	address = VirtualToPhysical(address);

	// RAM
	if (address < 2 * MiB) {
		return *reinterpret_cast<u8*>(ram + address);
	}

	// BIOS
	if (address >= 0x1fc00000 && address <= 0x1fffffff) {
		u32 index = address - 0x1fc00000;
		return *reinterpret_cast<u8*>(bios + index);
	}

	error_log("8-bit read <- unknown address {:08x}", address);
	return 0;
}

void Memory::WriteVirtualMemory8(u32 address, u8 byte) {
	// virtual -> physical
	address = VirtualToPhysical(address);

	// RAM
	if (address < 2 * MiB) {
		*reinterpret_cast<u8*>(ram + address) = byte;
		m_IOP->GetBackend()->Invalidate(address);
		return;
	}

	error_log("8-bit write {:02x} -> unknown address {:08x}", byte, address);
}

void Memory::Release() {
	free(ram);
	free(bios);
}

void Memory::LoadBIOS(std::filesystem::path path) {
	std::ifstream file;
	file.open(path, std::ios::binary | std::ios::ate);
	
	size_t size = file.tellg();
	if (size != 4 * MiB) {
		error_log("BIOS is not valid");
		exit(1);
	}

	file.seekg(0);
	file.read(reinterpret_cast<char*>(bios), size);
	file.close();
}