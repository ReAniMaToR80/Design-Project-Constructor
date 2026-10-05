#include "Types.h"

#include <cctype>

namespace bsc {
namespace {

bool equalsIgnoreCase(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        const auto ca = static_cast<unsigned char>(a[i]);
        const auto cb = static_cast<unsigned char>(b[i]);
        if (std::tolower(ca) != std::tolower(cb)) {
            return false;
        }
    }
    return true;
}

template <typename Enum, std::size_t N>
std::optional<Enum> findByCode(const std::array<Enum, N>& values, std::string_view code) noexcept {
    for (const Enum value : values) {
        if (equalsIgnoreCase(codeOf(value), code)) {
            return value;
        }
    }
    return std::nullopt;
}

}  // namespace

std::string_view codeOf(Operator value) noexcept {
    switch (value) {
        case Operator::Mts: return "MTS";
        case Operator::T2: return "T2";
        case Operator::Vimpelcom: return "VIMPELCOM";
        case Operator::Megafon: return "MEGAFON";
    }
    return "?";
}

std::string_view codeOf(PositionType value) noexcept {
    switch (value) {
        case PositionType::Rooftop: return "ROOF";
        case PositionType::GroundMast: return "GROUND_AMS";
    }
    return "?";
}

std::string_view codeOf(Element value) noexcept {
    switch (value) {
        case Element::Feeder: return "FEEDER";
        case Element::Optics: return "OPTICS";
        case Element::Enclosure: return "ENCLOSURE";
        case Element::Container: return "CONTAINER";
        case Element::ClimateCabinet: return "CLIMATE_CABINET";
        case Element::CustomPipeStand: return "PIPE_STAND";
        case Element::UnloadingTripod: return "TRIPOD";
    }
    return "?";
}

std::string_view nameOf(Operator value) noexcept {
    switch (value) {
        case Operator::Mts: return "МТС";
        case Operator::T2: return "Т2";
        case Operator::Vimpelcom: return "Вымпелком";
        case Operator::Megafon: return "Мегафон";
    }
    return "?";
}

std::string_view nameOf(PositionType value) noexcept {
    switch (value) {
        case PositionType::Rooftop: return "Крышная";
        case PositionType::GroundMast: return "Наземная АМС";
    }
    return "?";
}

std::string_view nameOf(Aspect value) noexcept {
    switch (value) {
        case Aspect::CableRoute: return "Кабельная трасса";
        case Aspect::HardwareRoom: return "Аппаратная";
        case Aspect::Structure: return "Конструктив КМ";
    }
    return "?";
}

std::string_view nameOf(Element value) noexcept {
    switch (value) {
        case Element::Feeder: return "Фидер";
        case Element::Optics: return "Оптика";
        case Element::Enclosure: return "Выгородка";
        case Element::Container: return "Контейнер";
        case Element::ClimateCabinet: return "ТКШ";
        case Element::CustomPipeStand: return "Трубостойка кастомная";
        case Element::UnloadingTripod: return "Трипод разгрузочный";
    }
    return "?";
}

std::string_view nameOf(Section value) noexcept {
    switch (value) {
        case Section::TX: return "ТХ";
        case Section::KM: return "КМ";
        case Section::EM: return "ЭМ";
        case Section::MZ: return "МЗ";
    }
    return "?";
}

std::optional<Operator> operatorFromCode(std::string_view code) noexcept {
    return findByCode(kAllOperators, code);
}

std::optional<PositionType> positionTypeFromCode(std::string_view code) noexcept {
    return findByCode(kAllPositionTypes, code);
}

std::optional<Element> elementFromCode(std::string_view code) noexcept {
    return findByCode(kAllElements, code);
}

std::optional<Element> SiteConfiguration::choice(Aspect aspect) const noexcept {
    switch (aspect) {
        case Aspect::CableRoute: return cableRoute;
        case Aspect::HardwareRoom: return hardwareRoom;
        case Aspect::Structure: return structure;
    }
    return std::nullopt;
}

void SiteConfiguration::setChoice(Aspect aspect, std::optional<Element> element) noexcept {
    switch (aspect) {
        case Aspect::CableRoute: cableRoute = element; break;
        case Aspect::HardwareRoom: hardwareRoom = element; break;
        case Aspect::Structure: structure = element; break;
    }
}

}  // namespace bsc
