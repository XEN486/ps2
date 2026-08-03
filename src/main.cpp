/// @file
#include "EmotionEngine/x64/jit_x64.hpp"
#include "EmotionEngine/emotion.hpp"
#include "EmotionEngine/DMAC/dmac.hpp"
#include "GraphicsSynthesizer/gs.hpp"
#include "memory.hpp"
#include "elf.hpp"

// #define ENABLE_DEBUGGER

int main(int argc, char** argv) {
#ifndef ENABLE_DEBUGGER
	if (argc < 2) {
		std::println(stderr, "usage: {} [elf file]", argv[0]);
		return 1;
	}
#endif

	GraphicsSynthesizer::GS gs;
	EmotionEngine::Core::JitX64 backend;
	EmotionEngine::EE cpu(&backend, &gs);
	
	Memory::Initialize(&backend, &cpu.GetDMAC());

	cpu.Reset();

#ifndef ENABLE_DEBUGGER
	ElfFile elf(argv[1]);
#else
	ElfFile elf("demo2a.elf");
#endif
	cpu.GetR5900().pc = elf.LoadElf();

	while (true) {
		cpu.RunOnce();
	}

	cpu.Release();
	Memory::Release();
	return 0;
}