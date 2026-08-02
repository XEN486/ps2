#include "dmac.hpp"
#include <format>

template <>
struct std::formatter<DMA::ChannelID> {
	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}

	auto format(const DMA::ChannelID& id, std::format_context& ctx) const {
		switch (id) {
			case DMA::ChannelID::VIF0:		return std::format_to(ctx.out(), "VIF0");
			case DMA::ChannelID::VIF1:		return std::format_to(ctx.out(), "VIF1");
			case DMA::ChannelID::GIF:		return std::format_to(ctx.out(), "GIF");
			case DMA::ChannelID::IPU_FROM:	return std::format_to(ctx.out(), "IPU_FROM");
			case DMA::ChannelID::IPU_TO:	return std::format_to(ctx.out(), "IPU_TO");
			case DMA::ChannelID::SIF0:		return std::format_to(ctx.out(), "SIF0");
			case DMA::ChannelID::SIF1:		return std::format_to(ctx.out(), "SIF1");
			case DMA::ChannelID::SIF2:		return std::format_to(ctx.out(), "SIF2");
			case DMA::ChannelID::SPR_FROM:	return std::format_to(ctx.out(), "SPR_FROM");
			case DMA::ChannelID::SPR_TO:	return std::format_to(ctx.out(), "SPR_TO");
		}
		
		std::unreachable();
	}
};

template <>
struct std::formatter<DMA::ChannelReg> {
	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}

	auto format(const DMA::ChannelReg reg, std::format_context& ctx) const {
		switch (reg) {
			case DMA::ChannelReg::CHCR:		return std::format_to(ctx.out(), "CHCR");
			case DMA::ChannelReg::MADR:		return std::format_to(ctx.out(), "MADR");
			case DMA::ChannelReg::TADR:		return std::format_to(ctx.out(), "TADR");
			case DMA::ChannelReg::QWC:		return std::format_to(ctx.out(), "QWC");
			case DMA::ChannelReg::ASR0:		return std::format_to(ctx.out(), "ASR0");
			case DMA::ChannelReg::ASR1:		return std::format_to(ctx.out(), "ASR1");
			case DMA::ChannelReg::SADR:		return std::format_to(ctx.out(), "SADR");
		}

		std::unreachable();
	}
};