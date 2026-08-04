#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include "EmotionEngine/emotion.hpp"
#include "GraphicsSynthesizer/gs.hpp"

/// @brief Manages the clocks of every component
class Scheduler {
public:
	void SetComponents(EmotionEngine::EE* ee, GraphicsSynthesizer::GS* gs) {
		m_EE = ee;
		m_GS = gs;
	}

	void Run();

	bool FrameReady() {
		bool ready = m_FrameReady;
		m_FrameReady = false;
		return ready;
	}

private:
	EmotionEngine::EE* m_EE;
	GraphicsSynthesizer::GS* m_GS;

	bool m_FrameReady = false;
};

#endif