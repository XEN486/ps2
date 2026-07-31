#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <cstdint>
#include <vector>
#include <filesystem>
#include <fstream>

#include "utils.hpp"

// forward declaration
namespace EmotionEngine::MIPS {
	class JitBackend;
}

#define RDRAM_END 0x01ffffff

// memory map
class Memory {
public:
	static void Initialize(EmotionEngine::MIPS::JitBackend* backend) {
		m_JitBackend = backend;
		m_Memory = static_cast<u8*>(calloc(1, 0x2000000)); // 32MiB
	}

	static u64 ReadVirtualMemory64(u32 address);
	static void WriteVirtualMemory64(u32 address, u64 qword);

	static u32 ReadVirtualMemory32(u32 address);
	static void WriteVirtualMemory32(u32 address, u32 dword);

	static void Release() {
		free(m_Memory);
	}

	inline static u8* m_Memory;

private:
	inline static EmotionEngine::MIPS::JitBackend* m_JitBackend;
	inline static bool m_ExposeBootROM;
};

#endif