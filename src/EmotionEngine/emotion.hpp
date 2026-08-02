#ifndef EMOTIONENGINE_EMOTION_HPP
#define EMOTIONENGINE_EMOTION_HPP

#include "../memory.hpp"
#include "../utils.hpp"

#include <asmjit/core.h>
#include <unordered_map>
#include <vector>

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

		/// @brief GPR names in MIPS ABI.
		/// While the CPU does not distinguish between the different GPRs, code running on the CPU does.
		/// The names of each register according to the MIPS ABI are defined here.
		enum class AbiNames : uint8_t {
			zero, at,
			v0, v1,
			a0, a1, a2, a3,
			t0, t1, t2, t3, t4, t5, t6, t7,
			s0, s1, s2, s3, s4, s5, s6, s7,
			t8, t9,
			k0, k1,
			gp, sp, fp,
			ra
		};

		enum class SpecialRegName {
			HI,
			LO,
			HI1,
			LO1,
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

			// cop1 registers
			float fpr[32];
			u32 fcr[32];
			float acc;

			// stuff for jit
			u32 pc;
			u32 next_pc;
		};


		/// @brief Function pointer to recompiled code.
		using BlockFunc = void (*)();

		/// @brief A compiled JIT block.
		/// Contains information about the block itself and a function pointer to the recompiled code.
		struct CompiledBlock {
			bool valid = false;		// block has been compiled
			size_t execution_count;	// number of times this block has been executed
			size_t instructions;	// number of instructions in block
			u32 start_pc;		// start address of the block
			u32 end_pc;		// end address of the block
			u32 after_end_pc;	// instruction after the block ends
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

			u8 rs;
			u8 rt;
			u8 rd;
			u8 sa;
			u8 funct;
			u16 imm;
			u32 addr;

			bool likely; // branch likely instruction
		};

		/// @brief Base class for the JIT recompiler's backend.
		class JitBackend {
		public:
			/// @brief Initializes the JIT backend. Automatically called by the EmotionEngine::EE() constructor.
			/// @param cpu Pointer to the R5900 CPU state.
			/// @return true on success
			virtual bool InitJit(R5900* cpu);

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
				u32 value = Memory::ReadVirtualMemory32(m_CompilePC);
				m_CompilePC += 4;
				return value;
			}

		protected:
			/// @brief Emit any instructions into the block's prologue here.
			virtual void EmitBeginBlock() = 0;

			/// @brief Emit any instructions into the block's epilogue here.
			virtual void EmitEndBlock() = 0;

			/// @brief Emits the stored branch delay slot. Make sure to call this in branch instructions.
			void EmitBranchDelay();
			
		protected:
			virtual void LUI(InstructionData& data) = 0;
			virtual void ADDIU(InstructionData& data) = 0;
			virtual void SLL(InstructionData& data) = 0;
			virtual void SQ(InstructionData& data) = 0;
			virtual void SLTU(InstructionData& data) = 0;
			virtual void JAL(InstructionData& data) = 0;
			virtual void BNE(InstructionData& data) = 0;
			virtual void DADDU(InstructionData& data) = 0;
			virtual void SYSCALL(InstructionData& data) = 0;
			virtual void JR(InstructionData& data) = 0;
			virtual void EI(InstructionData& data) = 0;
			virtual void LW(InstructionData& data) = 0;
			virtual void SD(InstructionData& data) = 0;
			virtual void SW(InstructionData& data) = 0;
			virtual void MULT(InstructionData& data) = 0;
			virtual void ADDU(InstructionData& data) = 0;
			virtual void LHU(InstructionData& data) = 0;
			virtual void SH(InstructionData& data) = 0;
			virtual void ORI(InstructionData& data) = 0;
			virtual void AND(InstructionData& data) = 0;
			virtual void SYNC(InstructionData& data) = 0;
			virtual void LD(InstructionData& data) = 0;
			virtual void DSRL(InstructionData& data) = 0;
			virtual void ANDI(InstructionData& data) = 0;
			virtual void LBU(InstructionData& data) = 0;
			virtual void SRL(InstructionData& data) = 0;
			virtual void DSLL(InstructionData& data) = 0;
			virtual void OR(InstructionData& data) = 0;
			virtual void DSLL32(InstructionData& data) = 0;
			virtual void BEQ(InstructionData& data) = 0;
			virtual void SB(InstructionData& data) = 0;
			virtual void SWC1(InstructionData& data) = 0;
			virtual void SLTIU(InstructionData& data) = 0;
			virtual void DIVU(InstructionData& data) = 0;
			virtual void MFHI(InstructionData& data) = 0;
			virtual void BREAK(InstructionData& data) = 0;
			virtual void BLTZ(InstructionData& data) = 0;
			virtual void LWC1(InstructionData& data) = 0;
			virtual void CVTsw(InstructionData& data) = 0;
			virtual void CVTws(InstructionData& data) = 0;
			virtual void MTC1(InstructionData& data) = 0;
			virtual void MFC1(InstructionData& data) = 0;
			virtual void DIVs(InstructionData& data) = 0;
			virtual void MOVs(InstructionData& data) = 0;
			virtual void MULs(InstructionData& data) = 0;
			virtual void BGEZ(InstructionData& data) = 0;
			virtual void SRA(InstructionData& data) = 0;

		protected:
			/// @brief Converts a virtual address to a physical address at compile-time.
			/// @param address Address to convert.
			constexpr void VirtualToPhysical(u32& address) {
				address &= 0x1fffffff;
			}

		protected:
			R5900* m_R5900;

			asmjit::JitRuntime m_Runtime;
			asmjit::CodeHolder m_CodeHolder;
			asmjit::FileLogger m_Logger;

			/// @brief PC used internally by the JIT to track where it is in MIPS code.
			u32 m_CompilePC;

		private:
			CompiledBlock& RecompileBlock(u32 pc);
			inline InstructionData AnalyzeOp(u32 opcode);
			void DecodeOp(InstructionData& data, u32 instruction);

		private:
			std::unordered_map<u32, CompiledBlock> m_BlockCache {};
			InstructionData m_BranchDelay;
			bool m_InBranchDelay;
		};	
	}

	/// @brief The PlayStation2's main processor unit.
	/// The EmotionEngine includes a MIPS R5900-based CPU core, two Vector Units, a DMA controller, an Image Processing Unit, and interfaces to other parts of the system.
	class EE {
	public:
		EE(Core::JitBackend* jit);

		/// @brief Returns a reference to the MIPS R5900 CPU state.
		/// @return Reference to the CPU state.
		Core::R5900& GetR5900() { return m_R5900; }

		/// @brief Resets the EmotionEngine's state.
		void Reset();

		/// @brief Compiles and runs a single JIT block.
		/// @return Number of instructions inside the JIT block.
		size_t RunOnce();

		/// @brief Releases all resources. Call this before terminating.
		void Release();

	private:
		Core::R5900 m_R5900;
		Core::JitBackend* m_JitBackend;
	};
}


#endif