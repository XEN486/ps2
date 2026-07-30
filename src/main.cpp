#include "emotion/x64/jit_x64.hpp"
#include "emotion/emotion.hpp"
#include "memory.hpp"
#include "elf.hpp"

int main() {
	EmotionEngine::MIPS::JitX64 backend;
	Memory::Initialize(&backend);

	EmotionEngine::EE cpu(&backend);
	cpu.Reset();

	ElfFile elf("demo1.elf");
	cpu.GetR5900().pc = elf.LoadElf();

	while (true) {
		cpu.RunOnce();
	}

	cpu.Release();
	return 0;
}