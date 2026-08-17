#include "sif.hpp"
using namespace SubsystemInterface;

void SIF::Initialize(u8* iop_ram) {
	m_IopRam = iop_ram;
}

u32 SIF::EE_ReadIOPMemory(u32 address) {
	return *reinterpret_cast<u32*>(m_IopRam + (address - 0x1c000000));
}

void SIF::EE_WriteIOPMemory(u32 address, u32 word) {
	*reinterpret_cast<u32*>(m_IopRam + (address - 0x1c000000)) = word;
}

u32 SIF::EE_ReadMailbox(u32 address) {
	if (address == 0x1000f200) return m_MailboxRegs.mscom;
	if (address == 0x1000f210) return m_MailboxRegs.smcom;
	if (address == 0x1000f220) return m_MailboxRegs.msflg;
	if (address == 0x1000f230) return m_MailboxRegs.smflg;
	if (address == 0x1000f240) return 0xffffffff;
	if (address == 0x1000f260) return m_MailboxRegs.bd6;
	return 0;
}

void SIF::EE_WriteMailbox(u32 address, u32 word) {
	if (address == 0x1000f200) { m_MailboxRegs.mscom = word; return; }
	if (address == 0x1000f220) { m_MailboxRegs.msflg = word; return; }
	if (address == 0x1000f230) { m_MailboxRegs.smflg &= word; return; }
	if (address == 0x1000f260) { m_MailboxRegs.bd6 = word; return; }
}

u32 SIF::IOP_ReadMailbox(u32 address) {
	if (address == 0x1d000000) return m_MailboxRegs.mscom;
	if (address == 0x1d000010) return m_MailboxRegs.smcom;
	if (address == 0x1d000020) return m_MailboxRegs.msflg;
	if (address == 0x1d000030) return m_MailboxRegs.smflg;
	if (address == 0x1d000040) return 0xfffffeff;
	if (address == 0x1d000060) return m_MailboxRegs.bd6;
	return 0;
}

void SIF::IOP_WriteMailbox(u32 address, u32 word) {
	if (address == 0x1d000010) { m_MailboxRegs.smcom = word; return; }
	if (address == 0x1d000020) { m_MailboxRegs.msflg &= word; return; }
	if (address == 0x1d000030) { m_MailboxRegs.smflg = word; return; }
	if (address == 0x1d000060) { m_MailboxRegs.bd6 = word; return; }
}