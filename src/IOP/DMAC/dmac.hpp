#ifndef IOP_DMAC_HPP
#define IOP_DMAC_HPP

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

	enum class Sync : u8 {
		Manual = 0,
		Request = 1,
		LinkedList = 2,
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
		Sync sync;
		bool trigger;
		bool chop;
		u8 chop_dma_size;
		u8 chop_cpu_size;
		u8 dummy;

		u32 base;

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
			bool triggered = (sync == Sync::Manual) ? trigger : true;
			return enable && triggered;
		}

		u32 GetTransferSize() {
			switch (sync) {
				case Sync::Manual: return block_size;
				case Sync::Request: return block_size * block_count;
				case Sync::LinkedList: return 0; // this shouldnt even be called
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
	};

	class DMAC {
	public:
		DMAC(IOProcessor::Memory* memory) : m_Memory(memory) {
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
		void DoDMATransfer(std::shared_ptr<Channel> channel);
		void DoBlockCopy(std::shared_ptr<Channel> channel);
		void DoLinkedList(std::shared_ptr<Channel> channel);

	private:
		u32 m_Control;

		IOProcessor::Memory* m_Memory;
		std::shared_ptr<Channel> m_Channels[13];
		InterruptRegister m_Interrupt;
	};
}

#endif