#include "EmotionEngine/x64/jit_x64.hpp"
#include "EmotionEngine/emotion.hpp"
#include "EmotionEngine/DMAC/dmac.hpp"
#include "GraphicsSynthesizer/gs.hpp"
#include "memory.hpp"
#include "elf.hpp"

int main(int argc, char** argv) {
	//if (argc < 2) {
	//	std::println(stderr, "usage: {} [elf file]", argv[0]);
	//	return 1;
	//}

	GraphicsSynthesizer::GS gs;
	EmotionEngine::Core::JitX64 backend;
	EmotionEngine::EE cpu(&backend, &gs);
	
	Memory::Initialize(&backend, &cpu.GetDMAC());

	cpu.Reset();

	ElfFile elf("demo2a.elf");
	cpu.GetR5900().pc = elf.LoadElf();

	while (true) {
		cpu.RunOnce();
	}

	cpu.Release();
	Memory::Release();
	return 0;
}