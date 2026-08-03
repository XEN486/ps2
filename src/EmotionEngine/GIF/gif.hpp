#ifndef EMOTIONENGINE_GIF_HPP
#define EMOTIONENGINE_GIF_HPP

#include "../../GraphicsSynthesizer/gs.hpp"
#include "../../utils.hpp"
#include <queue>

/// @brief The EmotionEngine's graphics interface.
namespace EmotionEngine::Graphics {
	enum DataFormat : u8 {
		Packed,
		RegList,
		Image1,
		Image2,
	};
	
	enum class ActivePath {
		None,
		Path1,
		Path2,
		Path3
	};

	enum class GIFstate {
		ReceiveTag,		// waiting for a GIFtag
		ReceiveData,	// waiting for more data
	};

	struct GIFtag {
		u16 nloop;
		bool eop;
		bool enable_prim;
		u16 prim_data;
		DataFormat data_format;
		u8 nregs;
	};

	/// @brief Structure to store PATH1 and PATH2 state
	struct Path {
		u128 qword;
		bool queued;
	};

	class GIF {
	public:
		void SetGS(GraphicsSynthesizer::GS* gs) {
			m_GS = gs;
		}

		void Reset();

		void ReceivePath1(u128 qword);
		void ReceivePath2(u128 qword);
		void ReceivePath3(u128 qword);
		void ProcessTag();

	private:
		ActivePath m_ActivePath;
		std::queue<u128> m_Path3Fifo;
		Path m_Path2;
		Path m_Path1;

		u128 m_RecentGIFtag;

		size_t m_CurrentReg = 0;

		GIFstate m_State;
		GIFtag m_LastTag;
		GraphicsSynthesizer::GS* m_GS;
	};
}

#endif