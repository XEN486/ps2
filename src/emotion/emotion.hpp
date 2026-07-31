#ifndef EMOTION_HPP
#define EMOTION_HPP

#include "../memory.hpp"
#include "../utils.hpp"

#include <asmjit/core.h>
#include <unordered_map>
#include <vector>

namespace EmotionEngine::MIPS {
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

	struct R5900 {
		// 32 GPRs (128-bit)
		GPR regs[32];

		// special registers
		u32 pc;
		u64 hi[2]; // hi[0] = hi, hi[1] = hi1
		u64 lo[2]; // hi[0] = lo, lo[1] = lo1

		// stuff for jit
		u32 next_pc;
	};

	using BlockFunc = void (*)();

	struct CompiledBlock {
		bool valid = false;		// block has been compiled
		size_t execution_count;	// number of times this block has been executed
		size_t instructions;	// number of instructions in block
		u32 start_pc;		// start address of the block
		u32 end_pc;		// end address of the block
		u32 after_end_pc;	// instruction after the block ends
		BlockFunc fn;			// function pointer to recompiled code
	};

	enum class InstructionType {
		Normal,
		Branch,
		Syscall,
		// BranchLikely, etc. later
	};

	class JitBackend;
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
	};

	class JitBackend {
	public:
		virtual bool InitJit(R5900* cpu);
		void Release();
		void Invalidate(u32 pc);

		CompiledBlock& GetOrCompileBlock(u32 pc);
		
	protected:
		[[nodiscard]] u32 Fetch() {
			u32 value = Memory::ReadVirtualMemory32(m_CompilePC);
			m_CompilePC += 4;
			return value;
		}

	protected:
		virtual void EmitBeginBlock() = 0;
		virtual void EmitEndBlock() = 0;
		
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

	protected:
		constexpr void VirtualToPhysical(u32& address) {
			address &= 0x1fffffff;
		}

	protected:
		R5900* m_R5900;

		asmjit::JitRuntime m_Runtime;
		asmjit::CodeHolder m_CodeHolder;
		asmjit::FileLogger m_Logger;

		// PC used internally by the jit to track where it is in mips code
		u32 m_CompilePC;

	private:
		CompiledBlock& RecompileBlock(u32 pc);
		inline InstructionData AnalyzeOp(u32 opcode);
		void DecodeOp(InstructionData& data, u32 instruction);

	private:
		std::unordered_map<u32, CompiledBlock> m_BlockCache {};
	};
}

namespace EmotionEngine {
	class EE {
	public:
		EE(MIPS::JitBackend* jit);
		MIPS::R5900& GetR5900() { return m_R5900; }

		void Reset();
		size_t RunOnce();
		void Release();

	private:
		MIPS::R5900 m_R5900;
		MIPS::JitBackend* m_JitBackend;
	};
}


#endif