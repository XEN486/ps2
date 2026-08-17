/// @file
#include "EmotionEngine/x64/jit_x64.hpp"
#include "IOP/x64/jit_x64.hpp"
#include "ps2.hpp"

#ifdef NDEBUG
int main(int argc, char** argv) {
	bool has_elf = argc > 2;
	if (argc < 2) {
		std::println(stderr, "usage: {} [bios bin] <elf file>", argv[0]);
		return 1;
	}

	EmotionEngine::Core::JitX64 ee_backend;
	IOProcessor::JitX64 iop_backend;

	PlayStation2 ps2;
	ps2.Create(&ee_backend, &iop_backend);
	ps2.LoadBIOS(argv[1]);
	ps2.Reset();
	if (has_elf) ps2.SideloadElf(argv[2]);
	
	while (true) {
		ps2.Run();

		if (ps2.FrameReady()) {
			// ps2.Render();
		}
	}

	ps2.Release();
	return 0;
}
#else
int main() {
	EmotionEngine::Core::JitX64 ee_backend;
	IOProcessor::JitX64 iop_backend;

	PlayStation2 ps2;
	ps2.Create(&ee_backend, &iop_backend);
	ps2.LoadBIOS("scph39001.bin");
	ps2.Reset();
	
	while (true) {
		ps2.Run();

		if (ps2.FrameReady()) {
			// ps2.Render();
		}
	}

	ps2.Release();
	return 0;
}
#endif