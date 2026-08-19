#ifndef EMOTIONENGINE_DMAC_HPP
#define EMOTIONENGINE_DMAC_HPP

#include "ids.hpp"

#include "../Memory/memory.hpp"
#include "../../utils.hpp"

namespace EmotionEngine { class EE; }

/// @brief The EmotionEngine's DMA subsystem.
namespace EmotionEngine::DMA {
	enum class ChannelID : u8 {
		VIF0,
		VIF1,
		GIF,
		IPU_FROM,
		IPU_TO,
		SIF0,
		SIF1,
		SIF2,
		SPR_FROM,
		SPR_TO,
	};

	enum class ChannelReg : u8 {
		CHCR	= 0x00,
		MADR	= 0x10,
		TADR	= 0x30,
		QWC		= 0x20,
		ASR0	= 0x40,
		ASR1	= 0x50,
		SADR	= 0x80,
	};

	enum class DmacReg : u16 {
		CTRL	= 0xe000,
		STAT	= 0xe010,
		PCR		= 0xe020,
		SQWC	= 0xe030,
		RBSR	= 0xe040,
		RBOR	= 0xe050,
		ENABLER	= 0xf520,
		ENABLEW	= 0xf590,
	};

	// lo 9-bit value of Dn_CHCR
	enum CHCRBits : u16 {
		DIR		= 0b000000001,
		MOD		= 0b000001100,
		ASP		= 0b000110000,
		TTE		= 0b001000000,
		TIE		= 0b010000000,
		STR		= 0b100000000,
	};

	enum class Mode : u8 {
		Normal		= 0,
		Chain		= 1,
		Interleave	= 2,
	};

	/// @brief Structure describing a DMAtag.
	struct DMAtag {
		u16 qword_count;
		bool enable_priority_control;
		u8 id;
		bool irq;
		u32 addr;
		bool scratchpad;
		
		// only if CHCR.TTE==1
		u64 data;
	};

	enum class ChainState {
		ReadDMAtag,
		ReadData
	};

	/// @brief Structure describing a DMA channel.
	struct Channel {
		ChannelID id;
		u32 chcr;		// channel control
		u32 madr;		// channel address
		u32 tadr;		// channel tag address
		u32 qwc;		// quadword count
		u32 asr0;		// channel saved tag address 0
		u32 asr1;		// channel saved tag address 1
		u32 sadr;		// channel scratchpad address

		ChainState chain;
		DMAtag last_tag;
		bool tag_end;
	};

	/// @brief Structure containing the DMA channels.
	struct Channels {
		Channel channels[10];
	};

	/// @brief Memory-mapped registers used to control the DMAC.
	struct DmacRegisters {
		u32 ctrl;
		u32 stat;
		u32 pcr;
		u32 sqwc;
		u32 rbsr;
		u32 rbor;
		u32 enable;
	};

	/// @brief The EmotionEngine's intelligent DMA controller.
	/// The DMAC is used to access most of the system except for main memory.
	class DMAC {
	public:
		void Initialize(EmotionEngine::EE* ee) {
			m_EE = ee;
		}
		
		void Reset();
		void Tick();

		void Write(u32 address, u32 word);
		u32 Read(u32 address);

	private:
		void WriteToChannel(ChannelID channel, u32 address, u32 word);
		u32 ReadFromChannel(ChannelID channel, u32 address);

		void WriteToReg(u32 address, u32 word);
		u32 ReadFromReg(u32 address);

		ChannelID GetChannelFromAddress(u8 addr);
		void ReadSourceTag();

		void DoTransfer();
		void DoNormalTransfer();
		void DoSourceChainTransfer();
		void DoDestChainTransfer();
		void DoInterleaveTransfer();

		void SendQword(u128 qword);
		void ProcessSourceChainTagID();
		void ProcessDestChainTagID();

		void FinishTransfer();
		void CheckInterrupt();

	private:
		DmacRegisters m_Regs;
		Channels m_Channels;

		bool m_InTransfer;
		Channel* m_TransferChannel;

		EmotionEngine::EE* m_EE;
	};
}

#include "dmac.inl"
#endif