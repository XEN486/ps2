#ifndef IOP_DMAC_HPP
#define IOP_DMAC_HPP

#include "../Memory/memory.hpp"
#include "../../utils.hpp"

namespace IOProcessor { class IOP; }

/// @brief The IOP's DMA subsystem.
namespace IOProcessor::DMA {
	enum class ChannelID : u8 {
		MDECin,
		MDECout,
		SIF2,
		CDVD,
		SPU1,
		PIO,
		OTC,
		SPU2,
		DEV9,
		SIF0,
		SIF1,
		SIO2in,
		SIO2out
	};

	enum class ChannelReg : u8 {
		MADR		= 0x0,
		BCR			= 0x4,
		CHCR		= 0x8,
		TADR		= 0xc,
	};

	enum class DmacReg : u16 {
		DPCR		= 0x10f0,
		DPCR2		= 0x1570,
		DICR		= 0x10f4,
		DICR2		= 0x1574,
		DMACEN		= 0x1578,
		DMACINTEN	= 0x157c,
	};

	enum class CHCRBits : u32 {
		TransferDirection		= 0b00000000000000000000000000000001, // 0=to RAM, 1=from RAM
		DecrementMADR			= 0b00000000000000000000000000000010, // 0=MADR+4, 1=MADR-4
		TransferDataBeforeTag	= 0b00000000000000000000000100000000, // transfer tag before data
		Mode					= 0b00000000000000000000011000000000, // transfer mode
		StartTransfer			= 0b00000001000000000000000000000000, // start transfer when DREQ happens and this bit is enabled
		ForceStartTransfer		= 0b00010000000000000000000000000000, // force start transfer
		DontClear28				= 0b00100000000000000000000000000000, // dont clear bit 28 after slice
	};

	enum class Mode : u8 {
		Burst,
		Slice,
		LinkedList,
		ChainMode
	};

	/// @brief Structure describing an IOP DMAtag.
	struct DMAtag {
		u32 start_address;
		bool irq; // raise IQE in DICR2 when words are transferred
		bool end; // raise transfer complete interrupt
		u32 size;
	};

	enum class ChainState {
		ReadDMAtag,
		ReadData
	};

	/// @brief Structure describing a DMA channel.
	struct Channel {
		ChannelID id;
		u32 madr;
		u32 bcr;
		u32 chcr;
		u32 tadr;

		ChainState chain;
		DMAtag last_tag;
	};

	/// @brief Structure containing the DMA channels.
	struct Channels {
		Channel channels[13];
	};

	struct ICR2 {
		u16 int_on_tag;
		u8 channel_int_mask;
		u8 channel_int_flags;

		u32 Read() const;
		void Write(u32 word);
	};

	struct ICR {
		u8* dmacinten;
		IOP* iop;
		ICR2* icr2;

		u8 channel_int_on_slice_and_ll;
		u8 channel_int_mask;
		bool master_channel_int_enable;
		u8 channel_int_flags;
		bool master_int_flag;

		u32 Read() const;
		void Write(u32 word);
		void RecalculateMIF();
	};
	
	/// @brief Memory-mapped registers used to control the DMAC.
	struct DmacRegisters {
		u32 dpcr;
		u32 dpcr2;
		ICR dicr;
		ICR2 dicr2;
		bool dmacen;
		u8 dmacinten;
	};

	/// @brief The IOP's DMA controller.
	class DMAC {
	public:
		void Initialize(IOProcessor::IOP* iop) {
			m_IOP = iop;
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

		ChannelID GetChannelFromAddress(u16 addr);
		u8 GetChannelPriority(ChannelID channel);

		void DoTransfer();

		void RaiseInterrupt(ChannelID channel);
		void FinishTransfer();

	private:
		DmacRegisters m_Regs;
		Channels m_Channels;
		std::vector<Channel> m_SortedChannels;

		bool m_InTransfer;
		Channel* m_TransferChannel;

		IOProcessor::IOP* m_IOP;
	};
}

#include "dmac.inl"
#endif