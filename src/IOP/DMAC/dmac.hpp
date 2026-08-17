#ifndef IOP_DMAC_HPP
#define IOP_DMAC_HPP

#include "../INTC/intc.hpp"
#include "../Memory/memory.hpp"
#include "../../utils.hpp"

#include <memory>
#include <utility>

namespace IOProcessor::DMA {
	enum class Direction : u8 {
		ToRam = 0,
		FromRam = 1,
	};

	enum class Step : u8 {
		Increment = 0,
		Decrement = 1,
	};

	enum class Mode : u8 {
		Burst = 0,
		Slice = 1,
		LinkedList = 2,
		ChainMode = 3,
	};

	enum class Port {
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

	struct Channel {
		virtual ~Channel() = default;
		virtual void Write(u32 word) = 0;
		virtual u32 Read(u32 address, u32 remaining_words) = 0;

		bool enable;
		Direction direction;
		Step step;
		Mode mode;
		bool trigger;
		bool chop;
		u8 chop_dma_size;
		u8 chop_cpu_size;

		u32 base;
		u32 tag_address;

		u16 block_size;
		u16 block_count;

		Port port;

		u32 GetControl();
		void SetControl(u32 value);

		u32 GetBlockControl() const {
			return (block_count << 16) | block_size;
		}

		void SetBlockControl(u32 value) {
			block_size = value & 0xffff;
			block_count = (value >> 16) & 0xffff;
		}

		bool IsActive() const {
			bool triggered = (mode == Mode::Burst) ? trigger : true;
			return enable && triggered;
		}

		u32 GetTransferSize() {
			switch (mode) {
				case Mode::Burst: return block_size;
				case Mode::Slice: return block_size * block_count;
				case Mode::LinkedList: return 0; // this shouldnt even be called
			}

			std::unreachable();
		}

		void TransferDone() {
			enable = false;
			trigger = false;
		}
	};

	struct InterruptRegister {
		bool enable_irq;
		u8 channel_enable_irq;
		u8 channel_irq_flags;
		bool force_irq;
		u8 dummy;

		u32 GetValue();
		void SetValue(u32 value);
		bool GetIRQStatus();
		void TryInterrupt(Interrupt::INTC* intc);
	};

	struct InterruptRegister2 {
		InterruptRegister* dicr;

		u16 tag_irq_flags; // only 4, 9 and 10 can be set
		u8 channel_enable_irq;
		u8 channel_irq_flags;

		u32 GetValue();
		void SetValue(u32 value);
		void TryInterrupt(Interrupt::INTC* intc);
		void TryTagInterrupt(Interrupt::INTC* intc);
	};

	class DMAC {
	public:
		DMAC(IOProcessor::Memory* memory, Interrupt::INTC* intc) : m_Memory(memory), m_INTC(intc) {
			Reset();
		}

		void Reset();

		u32 Read(u32 address);
		void Write(u32 address, u32 word);

		std::shared_ptr<Channel> GetChannel(Port port) {
			return m_Channels[static_cast<u8>(port)];
		}

		void SetChannel(Port port, std::shared_ptr<Channel> channel) {
			channel->port = port;
			m_Channels[static_cast<u8>(port)] = channel;
		}

	private:
		std::shared_ptr<Channel> GetChannel(u32 address);

		void DoDMATransfer(std::shared_ptr<Channel> channel);
		void DoBlockCopy(std::shared_ptr<Channel> channel);
		void DoLinkedList(std::shared_ptr<Channel> channel);
		void RaiseInterrupt(Port port);
		void RaiseTagInterrupt(Port port);

	private:
		u32 m_Control;
		u32 m_Control2;
		bool m_EnableDMA;
		bool m_DisableInterrupt;

		IOProcessor::Memory* m_Memory;
		Interrupt::INTC* m_INTC;

		std::shared_ptr<Channel> m_Channels[13];
		InterruptRegister m_Interrupt;
		InterruptRegister2 m_Interrupt2;
	};
}

#endif