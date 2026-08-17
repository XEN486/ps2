#include "ps2.hpp"
#include "elf.hpp"

void PlayStation2::Create(EmotionEngine::Core::JitBackend* ee_backend, IOProcessor::JitBackend* iop_backend) {
	m_GS = new GraphicsSynthesizer::GS();
	m_EE = new EmotionEngine::EE(ee_backend, m_GS);
	m_IOP = new IOProcessor::IOP(iop_backend);

	m_Scheduler.SetComponents(m_EE, m_GS, m_IOP);
}

void PlayStation2::LoadBIOS(std::filesystem::path path) {
	m_EE->GetMemory().LoadBIOS(path);
	m_IOP->GetMemory().LoadBIOS(path);
}

void PlayStation2::Reset() {
	m_GS->Reset();
	m_EE->Reset();
	m_IOP->Reset();
}

void PlayStation2::SideloadElf(std::filesystem::path path) {
	while (m_EE->GetR5900().pc != 0x82000) m_Scheduler.Run();
	ElfFile elf(path);
	m_EE->GetR5900().pc = elf.LoadElf(&m_EE->GetMemory());
}

void PlayStation2::Run() {
	m_Scheduler.Run();
}

void PlayStation2::Release() {
	m_EE->Release();
	m_IOP->Release();

	delete m_IOP;
	delete m_EE;
	delete m_GS;
}