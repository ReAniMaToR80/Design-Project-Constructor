// Types.h — orthogonal dimensions of the BS Constructor domain.
//
// Every dimension (operator, position type, aspect, element, section) is an
// independent enum. Dimensions never reference each other directly: all
// dependencies between them live in the LinkMatrix as L -> L^2 links.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace bsc {

// Mobile network operator (Заказчик).
enum class Operator : std::uint8_t { Mts, T2, Vimpelcom, Megafon };

// Type of the site (Тип позиции).
enum class PositionType : std::uint8_t {
    Rooftop,    // Крышная
    GroundMast  // Наземная АМС
};

// Aspect of the typical solution — one choice per aspect per project.
enum class Aspect : std::uint8_t {
    CableRoute,    // Кабельная трасса
    HardwareRoom,  // Аппаратная
    Structure      // Конструктив КМ
};

// Concrete variant of an aspect.
enum class Element : std::uint8_t {
    Feeder,           // Фидер
    Optics,           // Оптика
    Enclosure,        // Выгородка
    Container,        // Контейнер
    ClimateCabinet,   // ТКШ
    CustomPipeStand,  // Трубостойка кастомная
    UnloadingTripod   // Трипод разгрузочный
};

// Section of the design documentation a specification item belongs to.
enum class Section : std::uint8_t { TX, KM, EM, MZ };  // ТХ, КМ, ЭМ, МЗ

// How the quantity of a specification item scales with the site parameters.
enum class QuantityBasis : std::uint8_t {
    Fixed,               // quantity as is
    PerSector,           // quantity * sectors
    PerRouteMeter,       // quantity * cable route length
    PerSectorRouteMeter  // quantity * sectors * cable route length
};

inline constexpr std::array<Operator, 4> kAllOperators{Operator::Mts, Operator::T2, Operator::Vimpelcom,
                                                       Operator::Megafon};
inline constexpr std::array<PositionType, 2> kAllPositionTypes{PositionType::Rooftop, PositionType::GroundMast};
inline constexpr std::array<Aspect, 3> kAllAspects{Aspect::CableRoute, Aspect::HardwareRoom, Aspect::Structure};
inline constexpr std::array<Element, 7> kAllElements{Element::Feeder,          Element::Optics,
                                                     Element::Enclosure,       Element::Container,
                                                     Element::ClimateCabinet,  Element::CustomPipeStand,
                                                     Element::UnloadingTripod};

constexpr Aspect aspectOf(Element element) noexcept {
    switch (element) {
        case Element::Feeder:
        case Element::Optics:
            return Aspect::CableRoute;
        case Element::Enclosure:
        case Element::Container:
        case Element::ClimateCabinet:
            return Aspect::HardwareRoom;
        case Element::CustomPipeStand:
        case Element::UnloadingTripod:
            return Aspect::Structure;
    }
    return Aspect::Structure;
}

// Stable ASCII codes (for CSV, CLI options, DWG attributes).
std::string_view codeOf(Operator value) noexcept;
std::string_view codeOf(PositionType value) noexcept;
std::string_view codeOf(Element value) noexcept;

// Human readable Russian names (UTF-8).
std::string_view nameOf(Operator value) noexcept;
std::string_view nameOf(PositionType value) noexcept;
std::string_view nameOf(Aspect value) noexcept;
std::string_view nameOf(Element value) noexcept;
std::string_view nameOf(Section value) noexcept;

// Case-insensitive lookup by ASCII code ("MTS", "t2", "ROOF", ...).
std::optional<Operator> operatorFromCode(std::string_view code) noexcept;
std::optional<PositionType> positionTypeFromCode(std::string_view code) noexcept;
std::optional<Element> elementFromCode(std::string_view code) noexcept;

// Input data of one base station project. Unknown values stay empty and are
// either completed from the typical solution defaults or reported as conflicts.
struct SiteConfiguration {
    std::string siteId;
    std::optional<Operator> op;
    std::optional<PositionType> position;
    std::optional<Element> cableRoute;
    std::optional<Element> hardwareRoom;
    std::optional<Element> structure;
    unsigned sectors = 3;
    std::optional<double> antennaHeightM;
    std::optional<double> cableRouteLengthM;

    std::optional<Element> choice(Aspect aspect) const noexcept;
    void setChoice(Aspect aspect, std::optional<Element> element) noexcept;
};

}  // namespace bsc
