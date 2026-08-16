#include "memory.hpp"
#include "../../elf.hpp"
#include "../../utils.hpp"

#include "../emotion.hpp"

using namespace EmotionEngine;

u32 Memory::ReadVirtualMemory32(u32 address) {
	u32 virt = address;
	if (virt >= 0x70000000 && virt <= 0x70003fff) {
		return *(reinterpret_cast<u32*>(&scratchpad[virt - 0x70000000]));
	}

	address &= 0x1fffffff;

	// main memory
	if (address <= RDRAM_LAST_ADDR) {
		return *(reinterpret_cast<u32*>(&rdram[address]));
	}

	// timer addresses
	if (address >= 0x10000000 && address <= 0x10002030) {
		return m_EE->GetTimers().Read(address);
	}

	// dmac addresses
	if (address >= 0x10008000 && address <= 0x1000e060) {
		return m_EE->GetDMAC().ReadMemory32(address);
	}

	// D_ENABLER
	if (address == 0x1000f520) return m_EE->GetDMAC().ReadMemory32(address);

	// readable GS privileged registers
	if (address == 0x12001000) return m_GS->ReadLoCSR();
	if (address == 0x12001004) return m_GS->ReadHiCSR();
	if (address == 0x12001080) return m_GS->ReadLoSIGLBLID();
	if (address == 0x12001084) return m_GS->ReadHiSIGLBLID();
	
	// intc addresses
	if (address == 0x1000f000) return m_EE->GetINTC().GetSTAT();
	if (address == 0x1000f010) return m_EE->GetINTC().GetMASK();

	// GIF addresses
	if (address == 0x10003020) return m_EE->GetGIF().ReadStat();

	// undocumented registers
	if (address == 0x1f803204) return 0x00000020;
	if (address == 0x1a000004) return 0x00020000;

	// BIOS
	if (address >= 0x1fc00000 && address <= 0x1fffffff) {
		return *(reinterpret_cast<u32*>(&bios[address - 0x1fc00000]));
	}

	// RDRAM stuff (copied directly from PS2TEK)
	if (address == 0x1000f130) return 0;
	if (address == 0x1000f180) return 0;
	if (address == 0x1000f400) return 0;
	if (address == 0x1000f410) return 0;
	if (address == 0x1000f430) return 0;
	if (address == 0x1000f440) {
		u8 SOP = (MCH_RICM >> 6) & 0xf;
		u8 SA = (MCH_RICM >> 16) & 0xfff;
		if (!SOP) {
			switch (SA) {
				case 0x21: {
					if (rdram_sdevid < 2) {
						rdram_sdevid++;
						return 0x1f;
					}

					return 0;
				}

				case 0x23: return 0x0d0d;
				case 0x24: return 0x0090;
				case 0x40: return MCH_RICM & 0x1f;
			}
		}

		return 0;
	}

	error_log("unknown address v:{:08x} p:{:08x}", virt, address);
	return 0;
}

void Memory::WriteVirtualMemory32(u32 address, u32 word) {
	u32 virt = address;
	if (virt >= 0x70000000 && virt <= 0x70003fff) {
		*(reinterpret_cast<u32*>(&scratchpad[virt - 0x70000000])) = word;
		return;
	}

	address &= 0x1fffffff;
	
	// main memory
	if (address <= RDRAM_LAST_ADDR) {
		*(reinterpret_cast<u32*>(&rdram[address])) = word;
		m_JitBackend->Invalidate(address);
		return;
	}

	// timer addresses
	if (address >= 0x10000000 && address <= 0x10002030) {
		m_EE->GetTimers().Write(address, word);
		return;
	}

	// dmac addresses
	if (address >= 0x10008000 && address <= 0x1000e060) {
		m_EE->GetDMAC().WriteMemory32(address, word);
		return;
	}

	// D_ENABLEW
	if (address == 0x1000f590) return m_EE->GetDMAC().WriteMemory32(address, word);

	// GS privileged registers
	if (address >= 0x12000000 && address <= 0x12001080) {
		m_GS->WritePrivilegedRegEE(address, word);
		return;
	}

	// intc addresses
	if (address == 0x1000f000) return m_EE->GetINTC().SetSTAT(word);
	if (address == 0x1000f010) return m_EE->GetINTC().SetMASK(word);

	// GIF addresses
	if (address == 0x10003000) return m_EE->GetGIF().WriteCtrl(word);
	if (address == 0x10003010) return m_EE->GetGIF().WriteMode(word);
	if (address >= 0x10006000 && address <= 0x1000600c) return m_EE->GetGIF().WriteFifo(address, word);

	// console
	if (address == 0x1000f180) {
		std::print("{}", (char)(word & 0xff));
		return;
	}

	// RDRAM stuff (copied directly from PS2TEK)
	if (address == 0x1000f430) {
		u8 SA = (word >> 16) & 0xfff;
		u8 SBC = (word >> 6) & 0xf;
		if (SA == 0x21 && SBC == 0x1 && ((MCH_DRD >> 7) & 1) == 0) {
			rdram_sdevid = 0;
		}

		MCH_RICM = word & ~0x80000000;
		return;
	}

	if (address == 0x1000f440) {
		MCH_DRD = word;
		return;
	}

	if (address == 0x1000f100) return;
	if (address == 0x1000f120) return;
	if (address == 0x1000f140) return;
	if (address == 0x1000f150) return;
	if (address == 0x1000f400) return;
	if (address == 0x1000f410) return;
	if (address == 0x1000f420) return;
	if (address == 0x1000f450) return;
	if (address == 0x1000f460) return;
	if (address == 0x1000f480) return;
	if (address == 0x1000f490) return;
	if (address == 0x1000f500) return;
	if (address == 0x1000f510) return;

	error_log("{:08x} -> unknown address v:{:08x} p:{:08x}", word, virt, address);
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
	u32 phys = address;
	if (phys >= 0x70000000 && phys <= 0x70003fff) {
		return *(reinterpret_cast<u16*>(&scratchpad[phys - 0x70000000]));
	}

	u32 addr = address & 0x1fffffff;
	if (addr <= RDRAM_LAST_ADDR) {
		return *(reinterpret_cast<u16*>(&rdram[addr]));
	}

	u32 aligned = address & ~3;
	return (ReadVirtualMemory32(aligned) >> ((address & 2) * 8)) & 0xffff;
}

void Memory::WriteVirtualMemory16(u32 address, u16 hword) {
	u32 phys = address;
	if (phys >= 0x70000000 && phys <= 0x70003fff) {
		*(reinterpret_cast<u16*>(&scratchpad[phys - 0x70000000])) = hword;
		return;
	}

	u32 addr = address & 0x1fffffff;
	if (addr <= RDRAM_LAST_ADDR) {
		*(reinterpret_cast<u16*>(&rdram[addr])) = hword;
		m_JitBackend->Invalidate(addr);
		return;
	}

	u32 aligned = address & ~3;
	u32 word = ReadVirtualMemory32(aligned);
	u32 shift = (address & 2) * 8;
	word = (word & ~(0xffffu << shift)) | (static_cast<u32>(hword) << shift);
	WriteVirtualMemory32(aligned, word);
}

u8 Memory::ReadVirtualMemory8(u32 address) {
	u32 phys = address;
	if (phys >= 0x70000000 && phys <= 0x70003fff) {
		return scratchpad[phys - 0x70000000];
	}

	u32 addr = address & 0x1fffffff;
	if (addr <= RDRAM_LAST_ADDR) {
		return rdram[addr];
	}

	u32 aligned = address & ~3;
	return (ReadVirtualMemory32(aligned) >> ((address & 3) * 8)) & 0xff;
}

void Memory::WriteVirtualMemory8(u32 address, u8 byte) {
	u32 phys = address;
	if (phys >= 0x70000000 && phys <= 0x70003fff) {
		scratchpad[phys - 0x70000000] = byte;
		return;
	}

	u32 addr = address & 0x1fffffff;
	if (addr <= RDRAM_LAST_ADDR) {
		rdram[addr] = byte;
		m_JitBackend->Invalidate(addr);
		return;
	}

	u32 aligned = address & ~3;
	u32 word = ReadVirtualMemory32(aligned);
	u32 shift = (address & 3) * 8;
	word = (word & ~(0xffu << shift)) | (static_cast<u32>(byte) << shift);
	WriteVirtualMemory32(aligned, word);
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