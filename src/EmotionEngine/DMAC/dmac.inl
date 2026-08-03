#include "dmac.hpp"
#include <format>

template <>
struct std::formatter<EmotionEngine::DMA::ChannelID> {
	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}

	auto format(const EmotionEngine::DMA::ChannelID& id, std::format_context& ctx) const {
		switch (id) {
			case EmotionEngine::DMA::ChannelID::VIF0:		return std::format_to(ctx.out(), "VIF0");
			case EmotionEngine::DMA::ChannelID::VIF1:		return std::format_to(ctx.out(), "VIF1");
			case EmotionEngine::DMA::ChannelID::GIF:		return std::format_to(ctx.out(), "GIF");
			case EmotionEngine::DMA::ChannelID::IPU_FROM:	return std::format_to(ctx.out(), "IPU_FROM");
			case EmotionEngine::DMA::ChannelID::IPU_TO:		return std::format_to(ctx.out(), "IPU_TO");
			case EmotionEngine::DMA::ChannelID::SIF0:		return std::format_to(ctx.out(), "SIF0");
			case EmotionEngine::DMA::ChannelID::SIF1:		return std::format_to(ctx.out(), "SIF1");
			case EmotionEngine::DMA::ChannelID::SIF2:		return std::format_to(ctx.out(), "SIF2");
			case EmotionEngine::DMA::ChannelID::SPR_FROM:	return std::format_to(ctx.out(), "SPR_FROM");
			case EmotionEngine::DMA::ChannelID::SPR_TO:		return std::format_to(ctx.out(), "SPR_TO");
		}

		std::unreachable();
	}
};

template <>
struct std::formatter<EmotionEngine::DMA::ChannelReg> {
	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}

	auto format(const EmotionEngine::DMA::ChannelReg reg, std::format_context& ctx) const {
		switch (reg) {
			case EmotionEngine::DMA::ChannelReg::CHCR:		return std::format_to(ctx.out(), "CHCR");
			case EmotionEngine::DMA::ChannelReg::MADR:		return std::format_to(ctx.out(), "MADR");
			case EmotionEngine::DMA::ChannelReg::TADR:		return std::format_to(ctx.out(), "TADR");
			case EmotionEngine::DMA::ChannelReg::QWC:		return std::format_to(ctx.out(), "QWC");
			case EmotionEngine::DMA::ChannelReg::ASR0:		return std::format_to(ctx.out(), "ASR0");
			case EmotionEngine::DMA::ChannelReg::ASR1:		return std::format_to(ctx.out(), "ASR1");
			case EmotionEngine::DMA::ChannelReg::SADR:		return std::format_to(ctx.out(), "SADR");
		}

		std::unreachable();
	}
};

template <>
struct std::formatter<EmotionEngine::DMA::DmacReg> {
	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}

	auto format(const EmotionEngine::DMA::DmacReg reg, std::format_context& ctx) const {
		switch (reg) {
			case EmotionEngine::DMA::DmacReg::CTRL:		return std::format_to(ctx.out(), "CTRL");
			case EmotionEngine::DMA::DmacReg::STAT:		return std::format_to(ctx.out(), "STAT");
			case EmotionEngine::DMA::DmacReg::PCR:		return std::format_to(ctx.out(), "PCR");
			case EmotionEngine::DMA::DmacReg::SQWC:		return std::format_to(ctx.out(), "SQWC");
			case EmotionEngine::DMA::DmacReg::RBSR:		return std::format_to(ctx.out(), "RBSR");
			case EmotionEngine::DMA::DmacReg::RBOR:		return std::format_to(ctx.out(), "RBOR");
			case EmotionEngine::DMA::DmacReg::ENABLER:	return std::format_to(ctx.out(), "ENABLER");
			case EmotionEngine::DMA::DmacReg::ENABLEW:	return std::format_to(ctx.out(), "ENABLEW");
		}

		std::unreachable();
	}
};