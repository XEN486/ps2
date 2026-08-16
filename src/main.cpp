/// @file
#include "EmotionEngine/x64/jit_x64.hpp"
#include "EmotionEngine/emotion.hpp"
#include "GraphicsSynthesizer/gs.hpp"
#include "scheduler.hpp"
#include "elf.hpp"

int main(int argc, char** argv) {
	bool has_elf = argc > 2;
	if (argc < 2) {
		std::println(stderr, "usage: {} [bios bin] <elf file>", argv[0]);
		return 1;
	}

	GraphicsSynthesizer::GS gs;
	EmotionEngine::Core::JitX64 backend;
	EmotionEngine::EE cpu(&backend, &gs);

	gs.Reset();
	cpu.Reset();
	cpu.GetMemory().LoadBIOS(argv[1]);
	
	Scheduler scheduler;
	scheduler.SetComponents(&cpu, &gs);

	// sideload elf
	if (has_elf) {
		while (cpu.GetR5900().pc != 0x82000) scheduler.Run();
		ElfFile elf(argv[2]);
		cpu.GetR5900().pc = elf.LoadElf(&cpu.GetMemory());
	}
	
	while (true) {
		scheduler.Run();

		if (scheduler.FrameReady()) {
			// gs.Render();
		}
	}

	cpu.Release();
	return 0;
}