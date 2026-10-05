// TypicalSolutions.cpp — default base of typical solutions.
//
// The values below are a typical-solution template: item names partly come
// from the existing project scripts (MTS: Huawei RRU5502, Broadradio,
// Интеркросс Тип 5, Кр-1, КУСИЛнг-FRHF 2х6), the rest are placeholders to be
// replaced with the current revision of every operator regulation. Only this
// file has to change when a regulation changes.
#include "LinkMatrix.h"

#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace bsc {
namespace {

// Operator-specific parameters of the typical solution.
struct OperatorProfile {
    Operator op;
    std::string_view antenna;
    double antennaKg;
    std::string_view rru;
    double rruPerSector;
    double rruKg;
    std::string_view cabinet;
    std::string_view cabinetFrame;
    double pipeStandCapacityKg;
    double tripodCapacityKg;
};

constexpr OperatorProfile kProfiles[] = {
    {Operator::Mts, "Антенна секторная Broadradio, 2,0 м", 22.0, "Радиомодуль Huawei RRU5502", 2.0, 25.0,
     "Климатический шкаф Интеркросс Тип 5", "Рама подвесная Кр-1", 120.0, 300.0},
    {Operator::T2, "Антенна секторная 1,4 м (типовая Т2)", 18.0, "Радиомодуль RRU (типовой Т2)", 1.0, 20.0,
     "ТКШ уличный (типовой Т2)", "Рама напольная под ТКШ (типовая Т2)", 120.0, 300.0},
    {Operator::Vimpelcom, "Антенна секторная 2,0 м (типовая Вымпелком)", 26.0,
     "Радиомодуль RRU (типовой Вымпелком)", 2.0, 24.0, "ТКШ уличный (типовой Вымпелком)",
     "Рама подвесная под ТКШ (типовая Вымпелком)", 120.0, 300.0},
    {Operator::Megafon, "Антенна секторная 2,6 м (типовая Мегафон)", 35.0, "Радиомодуль RRU (типовой Мегафон)",
     2.0, 30.0, "ТКШ уличный (типовой Мегафон)", "Рама подвесная под ТКШ (типовая Мегафон)", 120.0, 300.0},
};

std::string makeCode(std::string_view prefix, std::string_view suffix) {
    std::string code(prefix);
    code += '-';
    code += suffix;
    return code;
}

SpecItem item(std::string code, std::string_view name, std::string_view unit, Section section,
              QuantityBasis basis, double quantity) {
    SpecItem result;
    result.code = std::move(code);
    result.name = std::string(name);
    result.unit = std::string(unit);
    result.section = section;
    result.basis = basis;
    result.quantity = quantity;
    return result;
}

SpecItem mounted(SpecItem base, double massKg) {
    base.massKg = massKg;
    base.mountedOnStructure = true;
    return base;
}

SpecItem loadBearing(SpecItem base, double massKg, double capacityKg) {
    base.massKg = massKg;
    base.capacityKg = capacityKg;
    return base;
}

void addOperatorItems(LinkMatrix& matrix, const OperatorProfile& p) {
    using B = QuantityBasis;
    const std::string_view prefix = codeOf(p.op);

    for (const Element element : kAllElements) {
        matrix.allow(p.op, element);
    }

    for (const PositionType position : kAllPositionTypes) {
        matrix.addItem(p.op, position,
                       mounted(item(makeCode(prefix, "AFU-ANT"), p.antenna, "шт", Section::TX, B::PerSector, 1.0),
                               p.antennaKg));
        matrix.addItem(p.op, position,
                       item(makeCode(prefix, "EM-GND"), "Комплект заземления оборудования", "компл", Section::EM,
                            B::Fixed, 1.0));
    }

    // Кабельная трасса — Оптика: RRU at the antennas, optics + DC power along the route.
    matrix.addItem(p.op, Element::Optics,
                   mounted(item(makeCode(prefix, "TX-RRU"), p.rru, "шт", Section::TX, B::PerSector, p.rruPerSector),
                           p.rruKg));
    matrix.addItem(p.op, Element::Optics,
                   item(makeCode(prefix, "TX-OPT"), "Кабель оптический SM DLC-DLC", "м", Section::TX,
                        B::PerSectorRouteMeter, 1.0));
    matrix.addItem(p.op, Element::Optics,
                   item(makeCode(prefix, "EM-PWR"), "Кабель питания КУСИЛнг-FRHF 2х6", "м", Section::EM,
                        B::PerSectorRouteMeter, 1.0));

    // Кабельная трасса — Фидер: radio units in the hardware room, coax up to the antennas.
    matrix.addItem(p.op, Element::Feeder,
                   item(makeCode(prefix, "TX-FDR"), "Фидер коаксиальный 7/8\"", "м", Section::TX,
                        B::PerSectorRouteMeter, 2.0));
    matrix.addItem(p.op, Element::Feeder,
                   item(makeCode(prefix, "TX-JMP"), "Джампер 1/2\" L=2 м", "шт", Section::TX, B::PerSector, 4.0));
    matrix.addItem(p.op, Element::Feeder,
                   mounted(item(makeCode(prefix, "TX-TMA"), "Мачтовый усилитель TMA", "шт", Section::TX,
                                B::PerSector, 2.0),
                           6.0));
    matrix.addItem(p.op, Element::Feeder,
                   item(makeCode(prefix, "MZ-FGK"), "Комплект заземления фидера", "компл", Section::MZ,
                        B::PerSector, 4.0));

    // Аппаратная.
    matrix.addItem(p.op, Element::Enclosure,
                   item(makeCode(prefix, "TX-ENC"), "Выгородка аппаратная из сэндвич-панелей", "компл", Section::TX,
                        B::Fixed, 1.0));
    matrix.addItem(p.op, Element::Enclosure,
                   item(makeCode(prefix, "EM-AC"), "Кондиционер 2,5 кВт", "шт", Section::EM, B::Fixed, 1.0));
    matrix.addItem(p.op, Element::Container,
                   item(makeCode(prefix, "TX-CNT"), "Контейнер аппаратный 3,0x2,4 м", "шт", Section::TX, B::Fixed,
                        1.0));
    matrix.addItem(p.op, Element::Container,
                   item(makeCode(prefix, "EM-AC"), "Кондиционер 2,5 кВт", "шт", Section::EM, B::Fixed, 2.0));
    matrix.addItem(p.op, Element::ClimateCabinet,
                   item(makeCode(prefix, "TX-TKSH"), p.cabinet, "шт", Section::TX, B::Fixed, 1.0));
    matrix.addItem(p.op, Element::ClimateCabinet,
                   item(makeCode(prefix, "KM-FRM"), p.cabinetFrame, "шт", Section::KM, B::Fixed, 1.0));

    // Конструктив КМ — one load-bearing unit per sector.
    matrix.addItem(p.op, Element::CustomPipeStand,
                   loadBearing(item(makeCode(prefix, "KM-PIPE"), "Трубостойка кастомная Ø89x4, L=3,0 м", "шт",
                                    Section::KM, B::PerSector, 1.0),
                               35.0, p.pipeStandCapacityKg));
    matrix.addItem(p.op, Element::UnloadingTripod,
                   loadBearing(item(makeCode(prefix, "KM-TRI"), "Трипод разгрузочный, H=3,0 м", "шт", Section::KM,
                                    B::PerSector, 1.0),
                               60.0, p.tripodCapacityKg));
}

void addPositionRules(LinkMatrix& matrix) {
    using B = QuantityBasis;
    using P = PositionType;

    // Крышная позиция: no container on the roof slab, tripods and pipe stands allowed.
    for (const Element element : {Element::Feeder, Element::Optics, Element::Enclosure, Element::ClimateCabinet,
                                  Element::CustomPipeStand, Element::UnloadingTripod}) {
        matrix.makeCompatible(P::Rooftop, element);
    }
    // Наземная АМС: no building — no enclosure; no roof — no unloading tripod.
    for (const Element element : {Element::Feeder, Element::Optics, Element::Container, Element::ClimateCabinet,
                                  Element::CustomPipeStand}) {
        matrix.makeCompatible(P::GroundMast, element);
    }

    matrix.setDefault(P::Rooftop, Element::Optics);
    matrix.setDefault(P::Rooftop, Element::ClimateCabinet);
    matrix.setDefault(P::Rooftop, Element::UnloadingTripod);
    matrix.setDefault(P::GroundMast, Element::Optics);
    matrix.setDefault(P::GroundMast, Element::ClimateCabinet);
    matrix.setDefault(P::GroundMast, Element::CustomPipeStand);

    // Position specific КМ / МЗ items.
    for (const Element element : {Element::Feeder, Element::Optics}) {
        matrix.addItem(P::Rooftop, element,
                       item("ROOF-KM-TRAY", "Кабельрост кровельный на опорах", "м", Section::KM, B::PerRouteMeter, 1.0));
        matrix.addItem(P::GroundMast, element,
                       item("AMS-KM-TRAY", "Кабельный лоток по стволу АМС", "м", Section::KM, B::PerRouteMeter, 1.0));
    }
    matrix.addItem(P::Rooftop, Element::CustomPipeStand,
                   item("ROOF-KM-ANCH", "Узел крепления трубостойки к парапету", "компл", Section::KM, B::PerSector,
                        1.0));
    matrix.addItem(P::Rooftop, Element::CustomPipeStand,
                   item("ROOF-MZ-ROD", "Молниеприемник стержневой L=2 м на трубостойке", "шт", Section::MZ,
                        B::PerSector, 1.0));
    matrix.addItem(P::Rooftop, Element::UnloadingTripod,
                   item("ROOF-KM-BAL", "Балласт трипода (бетонный блок 50 кг)", "шт", Section::KM, B::PerSector, 4.0));
    matrix.addItem(P::Rooftop, Element::UnloadingTripod,
                   item("ROOF-MZ-ROD", "Молниеприемник стержневой L=2 м на триподе", "шт", Section::MZ, B::PerSector,
                        1.0));
    for (const Element element : {Element::CustomPipeStand, Element::UnloadingTripod}) {
        matrix.addItem(P::Rooftop, element,
                       item("ROOF-MZ-DWN", "Токоотвод Ø8 мм к молниезащитной сетке здания", "м", Section::MZ,
                            B::Fixed, 20.0));
    }
    matrix.addItem(P::GroundMast, Element::CustomPipeStand,
                   item("AMS-KM-CLMP", "Узел крепления трубостойки к площадке АМС", "компл", Section::KM,
                        B::PerSector, 1.0));
    matrix.addItem(P::GroundMast, Element::CustomPipeStand,
                   item("AMS-MZ-BUS", "Шина заземления ШЗ-1", "шт", Section::MZ, B::Fixed, 3.0));
    matrix.addItem(P::GroundMast, Element::CustomPipeStand,
                   item("AMS-MZ-WIRE", "Провод заземления ПуГВ 1х16 к контуру АМС", "м", Section::MZ, B::Fixed,
                        15.0));
    matrix.addItem(P::Rooftop, Element::ClimateCabinet,
                   item("ROOF-KM-RAMA", "Рама разгрузочная под ТКШ на кровлю", "шт", Section::KM, B::Fixed, 1.0));
    matrix.addItem(P::GroundMast, Element::ClimateCabinet,
                   item("AMS-KM-FND", "Фундамент под ТКШ (ж/б плита)", "шт", Section::KM, B::Fixed, 1.0));
    matrix.addItem(P::GroundMast, Element::Container,
                   item("AMS-KM-FNDC", "Фундамент под контейнер (ж/б плиты)", "компл", Section::KM, B::Fixed, 1.0));
}

std::shared_ptr<LinkMatrix> buildTypical() {
    auto matrix = std::make_shared<LinkMatrix>();
    for (const OperatorProfile& profile : kProfiles) {
        addOperatorItems(*matrix, profile);
    }
    addPositionRules(*matrix);
    return matrix;
}

}  // namespace

std::shared_ptr<const LinkMatrix> LinkMatrix::makeTypical() {
    // Built once (thread-safe static initialization) and shared read-only by
    // every constructor instance.
    static const std::shared_ptr<const LinkMatrix> typical = buildTypical();
    return typical;
}

}  // namespace bsc
