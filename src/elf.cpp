#include "elf.hpp"
using namespace Elf32;

ElfFile::ElfFile(std::filesystem::path path) {
	m_File.open(path, std::ios::binary);
	m_File.read(reinterpret_cast<char*>(&m_ElfHeader), sizeof(ElfHeader));
	
	// check magic
	if (m_ElfHeader.e_ident[ElfIdent::Mag0] != 0x7f
		|| m_ElfHeader.e_ident[ElfIdent::Mag1] != 'E'
		|| m_ElfHeader.e_ident[ElfIdent::Mag2] != 'L'
		|| m_ElfHeader.e_ident[ElfIdent::Mag3] != 'F') {
		goto error;
	}

	// check if this ELF is 32-bit
	if (m_ElfHeader.e_ident[ElfIdent::Class] != ElfClass::Class32) goto error;

	// check if this ELF is little endian
	if (m_ElfHeader.e_ident[ElfIdent::Data] != ElfData::Data2LSB) goto error;

	// check if this ELF is executable
	if (m_ElfHeader.e_type != ElfHeaderType::Exec) goto error;

	// check if this ELF is for MIPS
	if (m_ElfHeader.e_machine != 8) goto error;
	
	return;

error:
	error_log("must be ELF32 MIPS LE executable");
	exit(1);
}

u32 ElfFile::LoadElf(EmotionEngine::Memory* memory) {
	ReadSegments();

	for (ProgramHeader& phdr : m_Segments) {
		if (phdr.p_type == ProgramHeaderType::Load) {
			debug_log("load {} bytes @ {:08x} -> {:08x}", phdr.p_filesz, phdr.p_offset, phdr.p_vaddr);

			m_File.seekg(phdr.p_offset);
			m_File.read(reinterpret_cast<char*>(memory->rdram + (phdr.p_vaddr & 0x1fffffff)), phdr.p_filesz);
		} else {
			error_log("unknown program header type");
			exit(1);
		}
	}
	return m_ElfHeader.e_entry;
}

void ElfFile::ReadSegments() {
	m_File.seekg(m_ElfHeader.e_phoff);

	for (size_t i = 0; i < m_ElfHeader.e_phnum; i++) {
		ProgramHeader phdr;
		m_File.read(reinterpret_cast<char*>(&phdr), sizeof(ProgramHeader));
		m_Segments.push_back(phdr);
	}
}