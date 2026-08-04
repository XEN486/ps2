/// @file
#include "EmotionEngine/x64/jit_x64.hpp"
#include "EmotionEngine/emotion.hpp"
#include "EmotionEngine/DMAC/dmac.hpp"
#include "GraphicsSynthesizer/gs.hpp"
#include "elf.hpp"

int main(int argc, char** argv) {
	//if (argc < 2) {
	//	std::println(stderr, "usage: {} [bios bin]", argv[0]);
	//	return 1;
	//}

	GraphicsSynthesizer::GS gs;
	EmotionEngine::Core::JitX64 backend;
	EmotionEngine::EE cpu(&backend, &gs);

	cpu.Reset();
	cpu.GetMemory().LoadBIOS("scph39001.bin");

	//ElfFile elf("demo1.elf");
	//cpu.GetR5900().pc = elf.LoadElf(cpu.GetMemory());
	
	while (true) {
		cpu.RunOnce();
	}

	cpu.Release();
	return 0;
}