#include "EmotionEngine/x64/jit_x64.hpp"
#include "EmotionEngine/emotion.hpp"
#include "memory.hpp"
#include "elf.hpp"

int main(int argc, char** argv) {
	if (argc < 2) {
		std::println(stderr, "usage: {} [elf file]", argv[0]);
		return 1;
	}

	EmotionEngine::Core::JitX64 backend;
	Memory::Initialize(&backend);

	EmotionEngine::EE cpu(&backend);
	cpu.Reset();

	ElfFile elf(argv[1]);
	cpu.GetR5900().pc = elf.LoadElf();

	while (true) {
		cpu.RunOnce();
	}

	cpu.Release();
	return 0;
}