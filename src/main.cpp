/// @file
#include "EmotionEngine/x64/jit_x64.hpp"
#include "EmotionEngine/emotion.hpp"
#include "EmotionEngine/DMAC/dmac.hpp"
#include "GraphicsSynthesizer/gs.hpp"
#include "elf.hpp"

//#define ENABLE_DEBUGGER

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

	cpu.Reset();

#ifndef ENABLE_DEBUGGER
	ElfFile elf(argv[1]);
#else
	ElfFile elf("3stars.elf");
#endif
	cpu.GetR5900().pc = elf.LoadElf(&cpu.GetMemory());

	while (true) {
		cpu.RunOnce();
	}

	cpu.Release();
	return 0;
}