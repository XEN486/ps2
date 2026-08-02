#ifndef ELF_HPP
#define ELF_HPP

#include "memory.hpp"
#include "utils.hpp"

#include <vector>

/// @brief Structures and types used by the ELF32 file format.
namespace Elf32 {
	using Addr		= u32;
	using Half		= u16;
	using Off		= u32;
	using Sword		= i32;
	using Word		= u32;
	using Versym	= u16;

	enum ElfIdent {
		Mag0		= 0,
		Mag1		= 1,
		Mag2		= 2,
		Mag3		= 3,
		Class		= 4,
		Data		= 5,
		Version		= 6,
		OsAbi		= 7,
		AbiVersion	= 8,
	};

	enum ElfClass {
		ClassNone	= 0,
		Class32		= 1,
		Class64		= 2,
	};

	enum ElfData {
		DataNone	= 0,
		Data2LSB	= 1,
		Data2MSB	= 2,
	};

	enum class ElfHeaderType : Half {
		None	= 0,
		Rel		= 1,
		Exec	= 2,
		Dyn		= 3,
		Core	= 4,
		LoOS	= 0xfe00,
		HiOS	= 0xfeff,
		LoProc	= 0xff00,
		HiProc	= 0xffff,
	};

	enum class ProgramHeaderType : Word {
		Null	= 0,
		Load	= 1,
		Dynamic	= 2,
		Interp	= 3,
		Note	= 4,
		ShLib	= 5,
		PHdr	= 6,
		Tls		= 7,
		LoOS	= 0x60000000,
		HiOS	= 0x6fffffff,
		LoProc	= 0x70000000,
		HiProc	= 0x7fffffff,
	};

	enum SegmentFlags : Word {
		X			= 0x1,
		W			= 0x2,
		R			= 0x4,
		MaskOS		= 0x0ff00000,
		MaskProc	= 0xf0000000,
	};

	/// @brief ELF32 header (ehdr).
	struct ElfHeader {
		unsigned char		e_ident[16];
		ElfHeaderType		e_type;
		Half				e_machine;
		Word				e_version;
		Addr				e_entry;
		Off					e_phoff;
		Off					e_shoff;
		Word				e_flags;
		Half				e_ehsize;
		Half				e_phentsize;
		Half				e_phnum;
		Half				e_shentsize;
		Half				e_shnum;
		Half				e_shstrndx;
	};

	/// @brief ELF32 program header (phdr).
	struct ProgramHeader {
		ProgramHeaderType	p_type;
		Off					p_offset;
		Addr				p_vaddr;
		Addr				p_paddr;
		Word				p_filesz;
		Word				p_memsz;
		SegmentFlags		p_flags;
		Word				p_align;
	};
}

/// @brief Class to contain and load an ELF file into memory.
class ElfFile {
public:
	ElfFile(std::filesystem::path path);
	u32 LoadElf();

private:
	void ReadSegments();

private:
	Elf32::ElfHeader m_ElfHeader;

private:
	std::ifstream m_File;
	std::vector<Elf32::ProgramHeader> m_Segments;
};

#endif