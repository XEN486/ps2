#include "EmotionEngine/x64/jit_x64.hpp"
#include "EmotionEngine/emotion.hpp"
#include "EmotionEngine/DMAC/dmac.hpp"
#include "memory.hpp"
#include "elf.hpp"

int main(int argc, char** argv) {
	if (argc < 2) {
		std::println(stderr, "usage: {} [elf file]", argv[0]);
		return 1;
	}

	EmotionEngine::Core::JitX64 backend;
	EmotionEngine::DMA::DMAC dmac;
	Memory::Initialize(&backend, &dmac);

	EmotionEngine::EE cpu(&backend);

	// reset everything
	dmac.Reset();
	cpu.Reset();

	ElfFile elf(argv[1]);
	cpu.GetR5900().pc = elf.LoadElf();

	while (true) {
		size_t instructions = cpu.RunOnce();

		// assume 1 instruction = 1 clock cycle.
		// tick dmac every other cycle
		for (size_t i = 0; i < (instructions / 2); i++) {
			dmac.Tick();
		}
	}

	cpu.Release();
	Memory::Release();
	return 0;
}