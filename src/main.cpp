/// @file
#include "EmotionEngine/x64/jit_x64.hpp"
#include "EmotionEngine/emotion.hpp"
#include "GraphicsSynthesizer/gs.hpp"
#include "scheduler.hpp"

int main(int argc, char** argv) {
	if (argc < 2) {
		std::println(stderr, "usage: {} [bios bin]", argv[0]);
		return 1;
	}

	GraphicsSynthesizer::GS gs;
	EmotionEngine::Core::JitX64 backend;
	EmotionEngine::EE cpu(&backend, &gs);

	gs.Reset();
	cpu.Reset();
	cpu.GetMemory().LoadBIOS(argv[1]);

	//ElfFile elf("demo1.elf");
	//cpu.GetR5900().pc = elf.LoadElf(cpu.GetMemory());
	
	Scheduler scheduler;
	scheduler.SetComponents(&cpu, &gs);
	
	while (true) {
		scheduler.Run();

		if (scheduler.FrameReady()) {
			// gs.Render();
		}
	}

	cpu.Release();
	return 0;
}