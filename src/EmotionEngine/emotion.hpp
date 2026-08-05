#ifndef EMOTIONENGINE_EMOTION_HPP
#define EMOTIONENGINE_EMOTION_HPP

#include "Memory/memory.hpp"
#include "Timers/timers.hpp"
#include "DMAC/dmac.hpp"
#include "GIF/gif.hpp"

#include "../GraphicsSynthesizer/gs.hpp"
#include "../utils.hpp"

#include <asmjit/core.h>
#include <unordered_map>
#include <vector>
#include <memory>

/// @brief The PlayStation2's main processor unit.
namespace EmotionEngine {
	/// @brief The EmotionEngine's MIPS R5900 CPU core.
	namespace Core {
		/// @brief MIPS general-purpose register.
		/// The EmotionEngine's MIPS core has 128-bit general-purpose registers.
		union GPR {
			GPR() : reg_u128(0) {}
			u128	reg_u128;
			i128	reg_i128;
			u64		reg_u64[2];
			i64		reg_i64[2];
			u32		reg_u32[4];
			i32		reg_i32[4];
			u16		reg_u16[8];
			i16		reg_i16[8];
			u8		reg_u8[16];
			i8		reg_i8[16];
		};

		static constexpr const char* g_RegNames[32] = {
			"$0", "at",
			"v0", "v1",
			"a0", "a1", "a2", "a3",
			"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
			"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
			"t8", "t9",
			"k0", "k1",
			"gp", "sp", "fp",
			"ra"
		};

		enum class SpecialRegName {
			HI,
			LO,
			HI1,
			LO1,
		};

		/// @brief The EmotionEngine's System Control Coprocessor
		struct Cop0 {
			u32 index;		// $0: Index that specifies TLB entry for reading or writing
			u32 entrylo0;	// $2: Lower part of the TLB entry 0
			u32 entrylo1;	// $2: Lower part of the TLB entry 1
			u32 pagemask;	// $5: Page size comparison mask
			u32 wired;		// $6: The number of Wired TLB entries
			u32 count;		// $9: Timer count value
			u32 entryhi;	// $10: Upper parts of a TLB entry
			u32 compare;	// $11: Timer stable value
			u32 status;		// $12: COP0 Status
			u32 prid;		// $15: Processor Revision Identifier
			u32 config;		// $16: Processor Configuration
		};

		/// @brief The EmotionEngine's MIPS CPU core.
		/// This contains the state of both the CPU core itself, aswell as COP1.
		struct R5900 {
			// 32 GPRs (128-bit)
			GPR gpr[32];

			// special registers
			u64 hi;
			u64 lo;
			u64 hi1;
			u64 lo1;

			// cop0
			Cop0 cop0;

			// cop1 registers
			float fpr[32];
			u32 fcr[32];
			float acc;

			// stuff for jit
			u32 pc;
			u32 next_pc;

			u32 ReadCOP0(u8 reg);
			void WriteCOP0(u8 reg, u32 val);
		};


		/// @brief Function pointer to recompiled code.
		using BlockFunc = void (*)();

		/// @brief A compiled JIT block.
		/// Contains information about the block itself and a function pointer to the recompiled code.
		struct CompiledBlock {
			bool valid = false;		// block has been compiled
			size_t execution_count;	// number of times this block has been executed
			size_t instructions;	// number of instructions in block
			u32 start_pc;			// start address of the block
			u32 end_pc;				// end address of the block
			u32 after_end_pc;		// instruction after the block ends
			BlockFunc fn;			// function pointer to recompiled code
		};

		/// @brief Different MIPS instruction types.
		/// The JIT has to handle different types of instructions seperately.
		/// Those types are defined in this enum.
		enum class InstructionType {
			Normal,
			Branch,
			Syscall,
			Sync
		};

		class JitBackend;

		/// @brief Decoded instruction data.
		struct InstructionData {
			InstructionType type;
			void (JitBackend::*ptr)(InstructionData&);

			std::shared_ptr<InstructionData> branch_delay;
			u32 pc;
			u8 rs;
			u8 rt;
			u8 rd;
			u8 sa;
			u8 funct;
			u16 imm;
			u32 addr;

			bool likely; // branch likely instruction
			bool pipeline1; // use logical pipeline 1 (HI1/LO1)
		};

		/// @brief Base class for the JIT recompiler's backend.
		class JitBackend {
		public:
			/// @brief Initializes the JIT backend. Automatically called by the EmotionEngine::EE() constructor.
			/// @param cpu Pointer to the R5900 CPU state.
			/// @param memory Pointer to the memory map.
			/// @return true on success
			virtual bool InitJit(R5900* cpu, Memory* memory);

			/// @brief Resets the JIT backend. Automatically called by EmotionEngine::EE::Reset()
			void Reset();

			/// @brief Releases the JIT backend. Automatically called by EmotionEngine::EE::Release()
			void Release();

			/// @brief Invalidates the JIT block at an address.
			/// @param pc Address to invalidate.
			void Invalidate(u32 pc);

			/// @brief Tries to get a cached block that starts at a specific address, and if it isn't found then compile a new one.
			/// @param pc Address that the block starts at.
			/// @return Reference to the compiled block.
			CompiledBlock& GetOrCompileBlock(u32 pc);
			
		protected:
			/// @brief Fetches a word at the current compile PC, and increments it. (NOTE: this is run at compile-time)
			/// @return The fetched value.
			[[nodiscard]] u32 Fetch() {
				u32 value = m_Memory->ReadVirtualMemory32(m_CompilePC);
				m_CompilePC += 4;
				return value;
			}

		protected:
			/// @brief Emit any instructions into the block's prologue here.
			virtual void EmitBeginBlock() = 0;

			/// @brief Emit any instructions into the block's epilogue here.
			virtual void EmitEndBlock() = 0;

			/// @brief Emits the stored branch delay slot. Make sure to call this in branch instructions.
			void EmitBranchDelay(InstructionData& data);
			
		protected:
			virtual void MFC0(InstructionData& data) = 0;

		protected:
			R5900* m_R5900;
			Memory* m_Memory;

			asmjit::JitRuntime m_Runtime;
			asmjit::CodeHolder m_CodeHolder;
			asmjit::FileLogger m_Logger;

			/// @brief PC used internally by the JIT to track where it is in MIPS code.
			u32 m_CompilePC;
			bool m_UsedRegisters[32];

		private:
			CompiledBlock& RecompileBlock(u32 pc);
			InstructionData AnalyzeOp(u32 opcode);
			void DecodeOp(InstructionData& data, u32 instruction);

			void UseRegisters(std::initializer_list<u8>(args)) {
				for (auto elem : args) {
					m_UsedRegisters[elem] = true;
				}
			}

		private:
			std::unordered_map<u32, CompiledBlock> m_BlockCache {};
			std::vector<InstructionData> m_Instructions;
			bool m_InBranchDelay;
		};	
	}

	/// @brief The PlayStation2's main processor unit.
	/// The EmotionEngine includes a MIPS R5900-based CPU core, two Vector Units, a DMA controller, an Image Processing Unit, and interfaces to other parts of the system.
	class EE {
	public:
		EE(Core::JitBackend* jit, GraphicsSynthesizer::GS* gs);

		/// @brief Returns a reference to the MIPS R5900 CPU state.
		/// @return Reference to the CPU state.
		Core::R5900& GetR5900() { return m_R5900; }

		/// @brief Returns a reference to the EE memory map.
		/// @return Reference to the EE memory map.
		Memory& GetMemory() { return m_Memory; }

		/// @brief Returns a reference to the DMA controller.
		/// @return Reference to the DMAC.
		DMA::DMAC& GetDMAC() { return m_DMAC; }

		/// @brief Returns a reference to the GIF.
		/// @return Reference to the GIF.
		Graphics::GIF& GetGIF() { return m_GIF; }

		/// @brief Returns a reference to the EE timers.
		/// @return Reference to the EE timers.
		Timers::Timers& GetTimers() { return m_Timers; }

		/// @brief Resets the EmotionEngine's state.
		void Reset();

		/// @brief Compiles and runs a single JIT block.
		/// @return Number of instructions inside the JIT block.
		size_t RunOnce();

		/// @brief Releases all resources. Call this before terminating.
		void Release();

	private:
		Core::R5900 m_R5900;
		Memory m_Memory;
		DMA::DMAC m_DMAC;
		Graphics::GIF m_GIF;
		Timers::Timers m_Timers;

		GraphicsSynthesizer::GS* m_GS;
		Core::JitBackend* m_JitBackend;
	};
}

#endif