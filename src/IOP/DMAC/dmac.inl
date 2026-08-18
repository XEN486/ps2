#include "dmac.hpp"
#include <format>

template <>
struct std::formatter<IOProcessor::DMA::ChannelID> {
	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}

	auto format(const IOProcessor::DMA::ChannelID& id, std::format_context& ctx) const {
		switch (id) {
			case IOProcessor::DMA::ChannelID::MDECin:	return std::format_to(ctx.out(), "MDECin");
			case IOProcessor::DMA::ChannelID::MDECout:	return std::format_to(ctx.out(), "MDECout");
			case IOProcessor::DMA::ChannelID::SIF2:		return std::format_to(ctx.out(), "SIF2");
			case IOProcessor::DMA::ChannelID::CDVD:		return std::format_to(ctx.out(), "CDVD");
			case IOProcessor::DMA::ChannelID::SPU1:		return std::format_to(ctx.out(), "SPU1");
			case IOProcessor::DMA::ChannelID::PIO:		return std::format_to(ctx.out(), "PIO");
			case IOProcessor::DMA::ChannelID::OTC:		return std::format_to(ctx.out(), "OTC");
			case IOProcessor::DMA::ChannelID::SPU2:		return std::format_to(ctx.out(), "SPU2");
			case IOProcessor::DMA::ChannelID::DEV9:		return std::format_to(ctx.out(), "DEV9");
			case IOProcessor::DMA::ChannelID::SIF0:		return std::format_to(ctx.out(), "SIF0");
			case IOProcessor::DMA::ChannelID::SIF1:		return std::format_to(ctx.out(), "SIF1");
			case IOProcessor::DMA::ChannelID::SIO2in:	return std::format_to(ctx.out(), "SIO2in");
			case IOProcessor::DMA::ChannelID::SIO2out:	return std::format_to(ctx.out(), "SIO2out");
		}

		std::unreachable();
	}
};

template <>
struct std::formatter<IOProcessor::DMA::ChannelReg> {
	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}

	auto format(const IOProcessor::DMA::ChannelReg reg, std::format_context& ctx) const {
		switch (reg) {
			case IOProcessor::DMA::ChannelReg::MADR:		return std::format_to(ctx.out(), "MADR");
			case IOProcessor::DMA::ChannelReg::BCR:			return std::format_to(ctx.out(), "BCR");
			case IOProcessor::DMA::ChannelReg::CHCR:		return std::format_to(ctx.out(), "CHCR");
			case IOProcessor::DMA::ChannelReg::TADR:		return std::format_to(ctx.out(), "TADR");
		}

		std::unreachable();
	}
};

template <>
struct std::formatter<IOProcessor::DMA::DmacReg> {
	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}

	auto format(const IOProcessor::DMA::DmacReg reg, std::format_context& ctx) const {
		switch (reg) {
			case IOProcessor::DMA::DmacReg::DPCR:		return std::format_to(ctx.out(), "DPCR");
			case IOProcessor::DMA::DmacReg::DPCR2:		return std::format_to(ctx.out(), "DPCR2");
			case IOProcessor::DMA::DmacReg::DICR:		return std::format_to(ctx.out(), "DICR");
			case IOProcessor::DMA::DmacReg::DICR2:		return std::format_to(ctx.out(), "DICR2");
			case IOProcessor::DMA::DmacReg::DMACEN:		return std::format_to(ctx.out(), "DMACEN");
			case IOProcessor::DMA::DmacReg::DMACINTEN:	return std::format_to(ctx.out(), "DMACINTEN");
		}

		std::unreachable();
	}
};