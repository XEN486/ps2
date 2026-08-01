#include "jit_x64.hpp"
#include "../../config.hpp"

using namespace EmotionEngine::MIPS;
using namespace asmjit;

bool JitX64::InitJit(R5900* cpu) {
	if (!JitBackend::InitJit(cpu)) return false;
	if (m_CodeHolder.attach(&cc) != Error::kOk) return false;

#ifdef ENABLE_ASMJIT_LOGGING
	m_Logger.set_flags(FormatFlags::kHexImms | FormatFlags::kHexOffsets | FormatFlags::kRegCasts);
	cc.add_diagnostic_options(DiagnosticOptions::kRAAnnotate | DiagnosticOptions::kRADebugAll);
#endif
	return true;
}

void JitX64::EmitBeginBlock() {
	cc.add_func(FuncSignature::build<void>());

	// r5900 = pointer to R5900 struct
	r5900 = cc.new_gp_ptr("r5900");
	cc.mov(r5900, m_R5900);

	// make a few scratch registers so we dont have to keep allocating
	s1 = cc.new_gp64("s1");
	s2 = cc.new_gp64("s2");
	s3 = cc.new_gp64("s3");
	s4 = cc.new_gp64("s4");

	// this can be freely overwritten by Emit*() functions
	t1 = cc.new_gp64("t1");

	// make a few vector registers so we dont have to keep allocating
	v1 = cc.new_vec128("v1");
	v2 = cc.new_vec128("v2");
	v3 = cc.new_vec128("v3");
	v4 = cc.new_vec128("v4");
}

void JitX64::EmitEndBlock() {
	cc.end_func();
	cc.finalize();
}

InvokeNode* JitX64::EmitExternalCall(uintptr_t address, const FuncSignature& sig) {
	InvokeNode* invoke_node;
	cc.invoke(Out(invoke_node), address, sig);
	return invoke_node;
}

void JitX64::EmitLoadRegister128(asmjit::x86::Vec& reg, u8 index) {
	if (index == 0) {
		cc.pxor(reg, reg);
		return;
	}

	cc.movdqu(reg, asmjit::x86::oword_ptr(r5900, index * sizeof(GPR)));
}

void JitX64::EmitStoreRegister128(u8 index, asmjit::x86::Vec& reg) {
	if (index == 0) return;
	cc.movdqu(asmjit::x86::oword_ptr(r5900, index * sizeof(GPR)), reg);
}

void JitX64::EmitReadVirtualMemory128(asmjit::x86::Vec& ret, const asmjit::x86::Gp& address) {
	EmitVirtualToPhysical(address);

	// only allow 128-bit reads from main memory
	cc.cmp(address, RDRAM_LAST_ADDR);

	Label outside_main_memory = cc.new_label();
	Label end = cc.new_label();

	cc.j(x86::CondCode::kA, outside_main_memory);

	// t1 <- &(main_memory[address])
	cc.movabs(t1, Memory::m_Memory);
	cc.add(t1, address);

	// ret <- *t1
	cc.movdqu(ret, x86::oword_ptr(t1));
	cc.jmp(end);

	cc.bind(outside_main_memory); {
		cc.int3();
		cc.nop();
	}

	cc.bind(end);
}

void JitX64::EmitWriteVirtualMemory128(const asmjit::x86::Gp& address, asmjit::x86::Vec& val) {
	EmitVirtualToPhysical(address);

	// only allow 128-bit writes to main memory
	cc.cmp(address, RDRAM_LAST_ADDR);

	Label outside_main_memory = cc.new_label();
	Label end = cc.new_label();

	cc.j(x86::CondCode::kA, outside_main_memory);

	// t1 <- &(main_memory[address])
	cc.movabs(t1, Memory::m_Memory);
	cc.add(t1, address);

	// ret <- *t1
	cc.movdqu(x86::oword_ptr(t1), val);
	cc.jmp(end);

	cc.bind(outside_main_memory); {
		cc.int3();
		cc.nop();
	}

	cc.bind(end);
}

void JitX64::EmitVirtualToPhysical(const asmjit::x86::Gp& address) {
	cc.and_(address, 0x1fffffff);
}

void JitX64::EmitReadVirtualMemory64(const asmjit::x86::Gp& ret, const asmjit::x86::Gp& address) {
	assert(ret.is_gp64());
	EmitVirtualToPhysical(address);

	Label not_main_memory = cc.new_label();
	Label end = cc.new_label();

	// read from main memory
	cc.cmp(address, RDRAM_LAST_ADDR);
	cc.j(x86::CondCode::kA, not_main_memory); {
		cc.movabs(t1, reinterpret_cast<uintptr_t>(Memory::m_Memory));	// t1 <- main memory
		cc.add(t1, address);											// t1 += address
		cc.mov(ret, asmjit::x86::qword_ptr(t1));						// ret <- [t1]

		cc.jmp(end);
	}

	// not in main memory (do external call)
	cc.bind(not_main_memory);
	asmjit::InvokeNode* node = EmitExternalCall(
		reinterpret_cast<uintptr_t>(&Memory::ReadVirtualMemory64),
		asmjit::FuncSignature::build<u64, u32>()
	);

	node->set_arg(0, address);
	node->set_ret(0, ret);

	cc.bind(end);
}

void JitX64::EmitWriteVirtualMemory64(const asmjit::x86::Gp& address, const asmjit::x86::Gp& value) {
	assert(value.is_gp64());
	EmitVirtualToPhysical(address);

	Label not_main_memory = cc.new_label();
	Label end = cc.new_label();

	// write to main memory
	cc.cmp(address, RDRAM_LAST_ADDR);
	cc.j(x86::CondCode::kA, not_main_memory); {
		cc.movabs(t1, reinterpret_cast<uintptr_t>(Memory::m_Memory));	// t1 <- main memory
		cc.add(t1, address);											// t1 += address
		cc.mov(asmjit::x86::qword_ptr(t1), value);						// [t1] <- value

		cc.jmp(end);
	}

	// not in main memory (do external call)
	cc.bind(not_main_memory);
	asmjit::InvokeNode* node = EmitExternalCall(
		reinterpret_cast<uintptr_t>(&Memory::WriteVirtualMemory64),
		asmjit::FuncSignature::build<void, u32, u64>()
	);

	node->set_arg(0, address);
	node->set_arg(1, value);

	cc.bind(end);
}

void JitX64::EmitReadVirtualMemory64(const asmjit::x86::Gp& ret, u32 address) {
	assert(ret.is_gp64());
	VirtualToPhysical(address);

	// optimization: read from main memory without calling external function
	if (address <= RDRAM_LAST_ADDR) {
		cc.mov(ret, asmjit::x86::qword_ptr(reinterpret_cast<uintptr_t>(Memory::m_Memory + address)));
		return;
	}

	asmjit::InvokeNode* node = EmitExternalCall(
		reinterpret_cast<uintptr_t>(&Memory::ReadVirtualMemory64),
		asmjit::FuncSignature::build<u64, u32>()
	);

	node->set_arg(0, address);
	node->set_ret(0, ret);
}

void JitX64::EmitWriteVirtualMemory64(u32 address, const asmjit::x86::Gp& value) {
	assert(value.is_gp64());
	VirtualToPhysical(address);

	// optimization: read from main memory without calling external function
	if (address <= RDRAM_LAST_ADDR) {
		cc.mov(asmjit::x86::qword_ptr(reinterpret_cast<uintptr_t>(Memory::m_Memory + address)), value);
		return;
	}

	asmjit::InvokeNode* node = EmitExternalCall(
		reinterpret_cast<uintptr_t>(&Memory::WriteVirtualMemory64),
		asmjit::FuncSignature::build<void, u32, u64>()
	);

	node->set_arg(0, address);
	node->set_arg(1, value);
}

void JitX64::EmitReadVirtualMemory32(const asmjit::x86::Gp& ret, const asmjit::x86::Gp& address) {
	assert(ret.is_gp32());
	EmitVirtualToPhysical(address);

	asmjit::x86::Gp value = cc.new_gp32("value");
	Label not_main_memory = cc.new_label();
	Label end = cc.new_label();

	// read from main memory
	cc.cmp(address, RDRAM_LAST_ADDR);
	cc.j(x86::CondCode::kA, not_main_memory); {
		cc.movabs(t1, reinterpret_cast<uintptr_t>(Memory::m_Memory));	// t1 <- main memory
		cc.add(t1, address);											// t1 += address
		cc.mov(value.r32(), asmjit::x86::dword_ptr(t1));				// value <- [t1]
		EmitWrite32To64Preserved(ret, value);							// ret <- value

		cc.jmp(end);
	}

	// not in main memory (do external call)
	cc.bind(not_main_memory);
	asmjit::InvokeNode* node = EmitExternalCall(
		reinterpret_cast<uintptr_t>(&Memory::ReadVirtualMemory32),
		asmjit::FuncSignature::build<u32, u32>()
	);

	node->set_arg(0, address);
	node->set_ret(0, value);				// value <- mem
	EmitWrite32To64Preserved(ret, value);	// ret <- value

	cc.bind(end);
}

void JitX64::EmitWriteVirtualMemory32(const asmjit::x86::Gp& address, const asmjit::x86::Gp& value) {
	assert(value.is_gp32());
	EmitVirtualToPhysical(address);

	Label not_main_memory = cc.new_label();
	Label end = cc.new_label();

	// write to main memory
	cc.cmp(address, RDRAM_LAST_ADDR);
	cc.j(x86::CondCode::kA, not_main_memory); {
		cc.movabs(t1, reinterpret_cast<uintptr_t>(Memory::m_Memory));	// t1 <- main memory
		cc.add(t1, address);											// t1 += address
		cc.mov(asmjit::x86::dword_ptr(t1), value.r32());				// [t1] <- value

		cc.jmp(end);
	}

	// not in main memory (do external call)
	cc.bind(not_main_memory);
	asmjit::InvokeNode* node = EmitExternalCall(
		reinterpret_cast<uintptr_t>(&Memory::WriteVirtualMemory32),
		asmjit::FuncSignature::build<void, u32, u32>()
	);

	node->set_arg(0, address);
	node->set_arg(1, value.r32());

	cc.bind(end);
}

void JitX64::EmitReadVirtualMemory32(const asmjit::x86::Gp& ret, u32 address) {
	assert(ret.is_gp32());
	asmjit::x86::Gp value = cc.new_gp32("value");
	VirtualToPhysical(address);

	// optimization: read from main memory without calling external function
	if (address <= RDRAM_LAST_ADDR) {
		cc.mov(value, asmjit::x86::dword_ptr(reinterpret_cast<uintptr_t>(Memory::m_Memory + address)));
		EmitWrite32To64Preserved(ret, value);
		return;
	}

	asmjit::InvokeNode* node = EmitExternalCall(
		reinterpret_cast<uintptr_t>(&Memory::ReadVirtualMemory32),
		asmjit::FuncSignature::build<u32, u32>()
	);

	node->set_arg(0, address);
	node->set_ret(0, value);
	EmitWrite32To64Preserved(ret, value);
}

void JitX64::EmitWriteVirtualMemory32(u32 address, const asmjit::x86::Gp& value) {
	assert(value.is_gp32());
	VirtualToPhysical(address);

	// optimization: read from main memory without calling external function
	if (address <= RDRAM_LAST_ADDR) {
		cc.mov(asmjit::x86::dword_ptr(reinterpret_cast<uintptr_t>(Memory::m_Memory + address)), value.r32());
		return;
	}

	asmjit::InvokeNode* node = EmitExternalCall(
		reinterpret_cast<uintptr_t>(&Memory::WriteVirtualMemory32),
		asmjit::FuncSignature::build<void, u32, u32>()
	);

	node->set_arg(0, address);
	node->set_arg(1, value.r32());
}

void JitX64::EmitWrite32To64Preserved(const asmjit::x86::Gp& dst, const asmjit::x86::Gp& src) {
	assert(!src.is_gp64());

	// writing to lo dword of a 64-bit register clears the hi dword too
	// so we have to do this
	if (dst.is_gp64()) {
		asmjit::x86::Gp mask = cc.new_gp64("mask");
		cc.movabs(mask, 0xffffffff00000000);
		cc.and_(dst, mask);
		cc.or_(dst, src);
	} else {
		cc.mov(dst, src);
	}
}

void JitX64::EmitReadVirtualMemory16(const asmjit::x86::Gp& ret, const asmjit::x86::Gp& address) {
	EmitVirtualToPhysical(address);

	Label not_main_memory = cc.new_label();
	Label end = cc.new_label();

	// read from main memory
	cc.cmp(address, RDRAM_LAST_ADDR);
	cc.j(x86::CondCode::kA, not_main_memory); {
		cc.movabs(t1, reinterpret_cast<uintptr_t>(Memory::m_Memory));	// t1 <- main memory
		cc.add(t1, address);											// t1 += address
		cc.mov(ret.r16(), asmjit::x86::word_ptr(t1));					// value <- [t1]

		cc.jmp(end);
	}

	// not in main memory (trap)
	cc.bind(not_main_memory); {
		cc.int3();
		cc.nop();
	}

	cc.bind(end);
}

void JitX64::EmitWriteVirtualMemory16(const asmjit::x86::Gp& address, const asmjit::x86::Gp& value) {
	EmitVirtualToPhysical(address);

	Label not_main_memory = cc.new_label();
	Label end = cc.new_label();

	// write to main memory
	cc.cmp(address, RDRAM_LAST_ADDR);
	cc.j(x86::CondCode::kA, not_main_memory); {
		cc.movabs(t1, reinterpret_cast<uintptr_t>(Memory::m_Memory));	// t1 <- main memory
		cc.add(t1, address);											// t1 += address
		cc.mov(asmjit::x86::word_ptr(t1), value.r16());					// [t1] <- value

		cc.jmp(end);
	}

	// not in main memory (trap)
	cc.bind(not_main_memory); {
		cc.int3();
		cc.nop();
	}

	cc.bind(end);
}

void JitX64::EmitReadVirtualMemory16(const asmjit::x86::Gp& ret, u32 address) {
	VirtualToPhysical(address);

	assert(address <= RDRAM_LAST_ADDR);
	cc.mov(ret.r16(), asmjit::x86::word_ptr(reinterpret_cast<uintptr_t>(Memory::m_Memory + address)));
}

void JitX64::EmitWriteVirtualMemory16(u32 address, const asmjit::x86::Gp& value) {
	VirtualToPhysical(address);

	assert(address <= RDRAM_LAST_ADDR);
	cc.mov(asmjit::x86::word_ptr(reinterpret_cast<uintptr_t>(Memory::m_Memory + address)), value.r16());
}

void JitX64::EmitStoreSpecialRegister(SpecialRegName dst, const asmjit::x86::Gp& src) {
	asmjit::x86::Mem ptr;
	switch (dst) {
		case SpecialRegName::HI: { ptr = x86::qword_ptr(r5900, offsetof(R5900, hi)); break; };
		case SpecialRegName::LO: { ptr = x86::qword_ptr(r5900, offsetof(R5900, lo)); break; };
		case SpecialRegName::HI1: { ptr = x86::qword_ptr(r5900, offsetof(R5900, hi1)); break; };
		case SpecialRegName::LO1: { ptr = x86::qword_ptr(r5900, offsetof(R5900, lo1)); break; };
	}

	if (src.is_gp32()) {
		cc.movsxd(t1, src);
		cc.mov(ptr, t1);
	} else {
		cc.mov(ptr, src);
	}
}