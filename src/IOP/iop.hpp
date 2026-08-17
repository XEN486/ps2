#ifndef IOP_IOP_HPP
#define IOP_IOP_HPP

#include "INTC/intc.hpp"
#include "DMAC/dmac.hpp"
#include "Memory/memory.hpp"

#include "../utils.hpp"

#include <asmjit/core.h>
#include <unordered_map>
#include <memory>
#include <filesystem>

namespace SubsystemInterface { class SIF; }
namespace IOProcessor {
	static constexpr const char* g_RegNames[32] = {
		"zero", "at",
		"v0", "v1",
		"a0", "a1", "a2", "a3",
		"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
		"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
		"t8", "t9",
		"k0", "k1",
		"gp", "sp", "fp",
		"ra"
	};

	enum Cop0Regs : u8 {
		BPC = 3,
		BDA = 5,
		TAR = 6,
		DCIC = 7,
		BadA = 8,
		BDAM = 9,
		BPCM = 11,
		SR = 12,
		CAUSE = 13,
		EPC = 14,
		PRID = 15,
	};

	enum class ExceptionCause : u8 {
		Interrupt			= 0x0,
		LoadAddressError	= 0x4,
		StoreAddressError	= 0x5,
		Syscall				= 0x8,
		Break				= 0x9,
		IllegalInstruction	= 0xa,
		CoprocessorError	= 0xb,
		Overflow			= 0xc,
	};
	
	struct R3000A {
		// 32 GPRs (32-bit)
		u32 gpr[32];

		// special registers
		u32 hi;
		u32 lo;

		// cop0 registers
		u32 cop0[32];

		// stuff for JIT
		u32 pc;
		u32 next_pc;

		u32 ReadCOP0(u8 reg);
		void WriteCOP0(u8 reg, u32 val);
		void Exception(ExceptionCause cause, u32 epc, bool in_delay_slot = false);
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
		Syscall
	};

	class JitBackend;

	/// @brief Decoded instruction data.
	struct InstructionData {
		InstructionType type;
		void (JitBackend::*ptr)(InstructionData&);

		std::shared_ptr<InstructionData> branch_delay;
		bool in_branch_delay;

		u32 pc;
		u8 rs;
		u8 rt;
		u8 rd;
		u8 sa;
		u8 funct;
		u16 imm;
		u32 addr;
	};

	/// @brief Base class for the JIT recompiler's backend.
	class JitBackend {
	public:
		/// @brief Initializes the JIT backend. Automatically called by the IOP() constructor.
		/// @param cpu Pointer to the R3000A CPU state.
		/// @param memory Pointer to the memory map.
		/// @return true on success
		virtual bool InitJit(R3000A* cpu, Memory* memory);

		/// @brief Resets the JIT backend. Automatically called by EmotionEngine::EE::Reset()
		void Reset();

		/// @brief Releases the JIT backend. Automatically called by EmotionEngine::EE::Release()
		void Release();

		/// @brief Invalidates the JIT block at an address.
		/// @param pc Address to invalidate.
		void Invalidate(u32 pc);

		/// @brief Invalidates the entire block cache.
		void InvalidateAll() {
			m_BlockCache.clear();
		}

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
		virtual void LUI(InstructionData& data) = 0;
		virtual void ORI(InstructionData& data) = 0;
		virtual void SW(InstructionData& data) = 0;
		virtual void SLL(InstructionData& data) = 0;
		virtual void ADDIU(InstructionData& data) = 0;
		virtual void J(InstructionData& data) = 0;
		virtual void LHU(InstructionData& data) = 0;
		virtual void OR(InstructionData& data) = 0;
		virtual void MTC0(InstructionData& data) = 0;
		virtual void BNE(InstructionData& data) = 0;
		virtual void ADDI(InstructionData& data) = 0;
		virtual void LW(InstructionData& data) = 0;
		virtual void SLTU(InstructionData& data) = 0;
		virtual void ADDU(InstructionData& data) = 0;
		virtual void SH(InstructionData& data) = 0;
		virtual void JAL(InstructionData& data) = 0;
		virtual void ANDI(InstructionData& data) = 0;
		virtual void SB(InstructionData& data) = 0;
		virtual void JR(InstructionData& data) = 0;
		virtual void LB(InstructionData& data) = 0;
		virtual void SLLV(InstructionData& data) = 0;
		virtual void BEQ(InstructionData& data) = 0;
		virtual void MFC0(InstructionData& data) = 0;
		virtual void AND(InstructionData& data) = 0;
		virtual void ADD(InstructionData& data) = 0;
		virtual void BGTZ(InstructionData& data) = 0;
		virtual void BLEZ(InstructionData& data) = 0;
		virtual void LBU(InstructionData& data) = 0;
		virtual void JALR(InstructionData& data) = 0;
		virtual void BLTZ(InstructionData& data) = 0;
		virtual void SLTI(InstructionData& data) = 0;
		virtual void SUBU(InstructionData& data) = 0;
		virtual void SRA(InstructionData& data) = 0;
		virtual void DIV(InstructionData& data) = 0;
		virtual void MFLO(InstructionData& data) = 0;
		virtual void BGEZ(InstructionData& data) = 0;
		virtual void SRL(InstructionData& data) = 0;
		virtual void SLTIU(InstructionData& data) = 0;
		virtual void DIVU(InstructionData& data) = 0;
		virtual void MFHI(InstructionData& data) = 0;
		virtual void SLT(InstructionData& data) = 0;
		virtual void SYSCALL(InstructionData& data) = 0;
		virtual void MTLO(InstructionData& data) = 0;
		virtual void MTHI(InstructionData& data) = 0;
		virtual void RFE(InstructionData& data) = 0;
		virtual void LH(InstructionData& data) = 0;
		virtual void NOR(InstructionData& data) = 0;
		virtual void SRAV(InstructionData& data) = 0;
		virtual void SRLV(InstructionData& data) = 0;
		virtual void MULTU(InstructionData& data) = 0;
		virtual void XOR(InstructionData& data) = 0;
		virtual void XORI(InstructionData& data) = 0;
		virtual void MULT(InstructionData& data) = 0;
		virtual void SUB(InstructionData& data) = 0;
		virtual void LWL(InstructionData& data) = 0;
		virtual void LWR(InstructionData& data) = 0;
		virtual void SWL(InstructionData& data) = 0;
		virtual void SWR(InstructionData& data) = 0;
		virtual void BLTZAL(InstructionData& data) = 0;
		virtual void BGEZAL(InstructionData& data) = 0;
		virtual void BREAK(InstructionData& data) = 0;

	protected:
		virtual void MoveImm(InstructionData& data) = 0;
		virtual void Illegal(InstructionData& data) = 0;

	protected:
		R3000A* m_R3000A;
		Memory* m_Memory;

		asmjit::JitRuntime m_Runtime;
		asmjit::CodeHolder m_CodeHolder;
		asmjit::FileLogger m_Logger;

		/// @brief PC used internally by the JIT to track where it is in MIPS code.
		u32 m_CompilePC;
		bool m_UsedRegisters[32];

	private:
		void NOP(InstructionData&) {}

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
	};

	class IOP {
	public:
		IOP(JitBackend* backend, SubsystemInterface::SIF* sif);

		/// @brief Returns a reference to the memory map.
		/// @return Reference to the memory map.
		Memory& GetMemory() { return m_Memory; }

		/// @brief Returns a reference to the R3000A state.
		/// @return Reference to the R3000A state.
		R3000A& GetR3000A() { return m_R3000A; }

		/// @brief Returns a reference to the interrupt controller.
		/// @return Reference to the interrupt controller.
		Interrupt::INTC& GetINTC() { return m_INTC; }

		/// @brief Returns a reference to the DMA controller.
		/// @return Reference to the DMA controller.
		DMA::DMAC& GetDMAC() { return m_DMAC; }

		/// @brief Returns a reference to the JIT backend.
		/// @return Reference to the JIT backend.
		JitBackend* GetBackend() { return m_JitBackend; }

		/// @brief Returns a reference to the SIF.
		/// @return Reference to the SIF.
		SubsystemInterface::SIF* GetSIF() { return m_SIF; }

		/// @brief Resets the IOP's state.
		void Reset();

		/// @brief Compiles and runs a single JIT block.
		/// @return Number of instructions inside the JIT block.
		size_t RunOnce();

		/// @brief Releases all resources. Call this before terminating.
		void Release();

	private:
		R3000A m_R3000A;
		Memory m_Memory;
		Interrupt::INTC m_INTC;
		DMA::DMAC m_DMAC;

		SubsystemInterface::SIF* m_SIF;
		JitBackend* m_JitBackend;
	};
}

#endif