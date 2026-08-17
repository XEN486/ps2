#ifndef PS2_HPP
#define PS2_HPP

#include "EmotionEngine/emotion.hpp"
#include "GraphicsSynthesizer/gs.hpp"
#include "IOP/iop.hpp"
#include "scheduler.hpp"

class PlayStation2 {
public:
	void Create(EmotionEngine::Core::JitBackend* ee_backend, IOProcessor::JitBackend* iop_backend);
	void LoadBIOS(std::filesystem::path path);
	void Reset();
	void SideloadElf(std::filesystem::path path);
	void Run();
	void Release();

	bool FrameReady() {
		return m_Scheduler.FrameReady();
	}

private:
	EmotionEngine::EE* m_EE;
	GraphicsSynthesizer::GS* m_GS;
	IOProcessor::IOP* m_IOP;

	Scheduler m_Scheduler;
};

#endif