// Tests of the constructor: specification links when switching operators,
// validation of equipment against the structure type, reports.
#include "BSConstructor.h"
#include "Report.h"
#include "TestRunner.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace bsc::test {
namespace {

constexpr std::string_view kDemoAct =
    "Оператор: МТС\n"
    "Номер БС: 78-04546\n"
    "Тип площадки: Наземная АМС\n"
    "Высота подвеса: +36,100 м\n"
    "Количество секторов: 3\n"
    "Кабельная трасса: оптика\n"
    "Аппаратная: ТКШ\n";

SiteConfiguration groundSite(Operator op, Element structure) {
    SiteConfiguration config;
    config.siteId = "78-04546";
    config.op = op;
    config.position = PositionType::GroundMast;
    config.cableRoute = Element::Optics;
    config.hardwareRoom = Element::ClimateCabinet;
    config.structure = structure;
    config.antennaHeightM = 36.1;
    return config;
}

SiteConfiguration rooftopSite(Operator op) {
    SiteConfiguration config;
    config.siteId = "50-12345";
    config.op = op;
    config.position = PositionType::Rooftop;
    config.cableRoute = Element::Feeder;
    config.hardwareRoom = Element::Enclosure;
    config.structure = Element::UnloadingTripod;
    return config;
}

bool startsWith(std::string_view value, std::string_view prefix) {
    return value.substr(0, prefix.size()) == prefix;
}

bool isOperatorItem(std::string_view code) {
    for (const Operator op : kAllOperators) {
        std::string prefix(codeOf(op));
        prefix += '-';
        if (startsWith(code, prefix)) {
            return true;
        }
    }
    return false;
}

// Codes expected from the link matrix for the resolved choices of a project.
std::vector<std::string> expectedCodes(const LinkMatrix& matrix, const SiteConfiguration& resolved) {
    std::vector<std::string> codes;
    const auto append = [&codes](const std::vector<SpecItem>& items) {
        for (const SpecItem& item : items) {
            codes.push_back(item.code);
        }
    };
    append(matrix.itemsFor(*resolved.op, *resolved.position));
    for (const Aspect aspect : kAllAspects) {
        append(matrix.itemsFor(*resolved.op, *resolved.choice(aspect)));
        append(matrix.itemsFor(*resolved.position, *resolved.choice(aspect)));
    }
    return codes;
}

std::vector<std::string> actualCodes(const Project& project) {
    std::vector<std::string> codes;
    for (const SpecLine& line : project.specification) {
        codes.push_back(line.item.code);
    }
    return codes;
}

void demoActBuildsValidProject(Check& check) {
    BSConstructor constructor;
    const Project& project = constructor.loadAct(kDemoAct);
    check.isTrue(project.isValid(), "demo project is valid");
    check.isTrue(project.resolved.structure == Element::CustomPipeStand, "default structure for a ground mast");
    check.isTrue(project.hasConflict("I_DEFAULT_APPLIED"), "default is reported");
    check.isTrue(project.hasConflict("I_KM_LOAD"), "load check is reported");
    check.near(project.cableRouteLengthM, 47.0, 1e-9, "route = ceil(36.1 + 10)");
    check.near(project.quantityOf("MTS-TX-RRU"), 6.0, 1e-9, "2 RRU per sector");
    check.near(project.quantityOf("MTS-TX-OPT"), 141.0, 1e-9, "optics = sectors x route");
    check.near(project.quantityOf("MTS-KM-PIPE"), 3.0, 1e-9, "pipe stand per sector");
    check.near(project.quantityOf("AMS-MZ-BUS"), 3.0, 1e-9, "grounding bus of the mast");
    check.near(project.quantityOf("ROOF-KM-TRAY"), 0.0, 1e-9, "no rooftop items on a mast");
}

void switchingOperatorsRelinksSpecification(Check& check) {
    BSConstructor constructor;
    constructor.configure(groundSite(Operator::Mts, Element::UnloadingTripod));
    const std::vector<std::string> positionItemsBefore = [&] {
        std::vector<std::string> codes;
        for (const SpecLine& line : constructor.current()->specification) {
            if (!isOperatorItem(line.item.code)) {
                codes.push_back(line.item.code);
            }
        }
        return codes;
    }();

    for (const Operator op : kAllOperators) {
        const Project& project = constructor.switchOperator(op);
        const std::string label(codeOf(op));
        std::string prefix = label + "-";

        check.isTrue(project.resolved.op == op, label + ": operator switched");
        check.equal(project.resolved.siteId, std::string("78-04546"), label + ": site id kept");
        check.isTrue(project.resolved.position == PositionType::GroundMast, label + ": position kept");
        check.isTrue(project.resolved.cableRoute == Element::Optics, label + ": cable route kept");
        check.isTrue(project.resolved.structure == Element::UnloadingTripod, label + ": structure kept");

        std::vector<std::string> positionItems;
        int operatorItems = 0;
        for (const SpecLine& line : project.specification) {
            if (isOperatorItem(line.item.code)) {
                ++operatorItems;
                check.isTrue(startsWith(line.item.code, prefix), label + ": no stale item " + line.item.code);
            } else {
                positionItems.push_back(line.item.code);
            }
        }
        check.isTrue(operatorItems > 0, label + ": operator items present");
        check.isTrue(positionItems == positionItemsBefore, label + ": position items do not depend on operator");
        check.isTrue(actualCodes(project) == expectedCodes(constructor.matrix(), project.resolved),
                     label + ": specification equals the links of the matrix");
        check.near(project.quantityOf(prefix + "TX-TKSH"), 1.0, 1e-9, label + ": operator cabinet");
    }
}

void switchingOperatorChangesQuantities(Check& check) {
    BSConstructor constructor;
    constructor.loadAct(kDemoAct);
    check.near(constructor.current()->quantityOf("MTS-TX-RRU"), 6.0, 1e-9, "MTS: 2 RRU per sector");
    const Project& t2 = constructor.switchOperator(Operator::T2);
    check.near(t2.quantityOf("T2-TX-RRU"), 3.0, 1e-9, "T2: 1 RRU per sector");
    check.near(t2.quantityOf("MTS-TX-RRU"), 0.0, 1e-9, "MTS RRU removed");
    check.equal(t2.specification.size(), constructor.build(t2.requested).specification.size(),
                "switch equals a fresh build");
}

void switchingToHeavyOperatorOverloadsPipeStand(Check& check) {
    BSConstructor constructor;
    check.isTrue(constructor.configure(groundSite(Operator::Mts, Element::CustomPipeStand)).isValid(),
                 "MTS on pipe stand is valid (72 kg x 1.3 = 93.6 <= 120)");
    const Project& megafon = constructor.switchOperator(Operator::Megafon);
    check.isFalse(megafon.isValid(), "Megafon on pipe stand is invalid");
    check.isTrue(megafon.hasConflict("E_KM_OVERLOAD"), "95 kg x 1.3 = 123.5 > 120");
    const Project& back = constructor.switchOperator(Operator::Mts);
    check.isTrue(back.isValid(), "switching back clears the overload");
    check.isFalse(back.hasConflict("E_KM_OVERLOAD"), "no overload");
}

void heavyOperatorOnTripodIsValid(Check& check) {
    const BSConstructor constructor;
    const Project project = constructor.build(rooftopSite(Operator::Megafon));
    check.isTrue(project.isValid(), "Megafon on a rooftop tripod is valid");
    check.isTrue(project.hasConflict("I_KM_LOAD"), "load is reported");
    check.near(project.quantityOf("MEGAFON-TX-TMA"), 6.0, 1e-9, "feeder brings TMA");
    check.near(project.quantityOf("ROOF-KM-BAL"), 12.0, 1e-9, "tripod ballast");
    check.near(project.quantityOf("MEGAFON-TX-FDR"), 180.0, 1e-9, "2 feeders x 3 sectors x 30 m");
}

void incompatibleEquipmentIsDetected(Check& check) {
    const BSConstructor constructor;

    const Project tripodOnMast = constructor.build(groundSite(Operator::T2, Element::UnloadingTripod));
    check.isTrue(tripodOnMast.hasConflict("E_POSITION_INCOMPATIBLE"), "tripod on a ground mast");

    SiteConfiguration containerOnRoof = rooftopSite(Operator::Vimpelcom);
    containerOnRoof.hardwareRoom = Element::Container;
    check.isTrue(constructor.build(containerOnRoof).hasConflict("E_POSITION_INCOMPATIBLE"), "container on a roof");

    SiteConfiguration enclosureOnMast = groundSite(Operator::Mts, Element::CustomPipeStand);
    enclosureOnMast.hardwareRoom = Element::Enclosure;
    const Project enclosure = constructor.build(enclosureOnMast);
    check.isFalse(enclosure.isValid(), "enclosure on a ground mast");
    check.isTrue(enclosure.hasConflict("E_POSITION_INCOMPATIBLE"), "enclosure conflict code");

    check.isTrue(constructor.build(rooftopSite(Operator::T2)).isValid(), "compatible rooftop is valid");
}

void operatorRegulationIsEnforced(Check& check) {
    auto matrix = std::make_shared<LinkMatrix>();
    for (const Operator op : kAllOperators) {
        for (const Element element : kAllElements) {
            if (!(op == Operator::T2 && element == Element::Feeder)) {
                matrix->allow(op, element);
            }
        }
    }
    for (const Element element : kAllElements) {
        matrix->makeCompatible(PositionType::Rooftop, element);
    }
    BSConstructor constructor(matrix);
    check.isTrue(constructor.configure(rooftopSite(Operator::Mts)).isValid(), "MTS allows feeder");
    const Project& t2 = constructor.switchOperator(Operator::T2);
    check.isTrue(t2.hasConflict("E_OPERATOR_REGULATION"), "T2 regulation forbids feeder");
    check.isTrue(constructor.sharedMatrix() == matrix, "custom matrix is shared, not copied");
}

void defaultsAndMissingData(Check& check) {
    const BSConstructor constructor;
    SiteConfiguration rooftop;
    rooftop.siteId = "1";
    rooftop.op = Operator::T2;
    rooftop.position = PositionType::Rooftop;
    const Project project = constructor.build(rooftop);
    check.isTrue(project.isValid(), "rooftop defaults are valid");
    check.isTrue(project.resolved.cableRoute == Element::Optics, "default cable route");
    check.isTrue(project.resolved.hardwareRoom == Element::ClimateCabinet, "default hardware room");
    check.isTrue(project.resolved.structure == Element::UnloadingTripod, "default rooftop structure");
    check.isFalse(project.requested.structure.has_value(), "request is kept untouched");

    const Project empty = constructor.build(SiteConfiguration{});
    check.isTrue(empty.hasConflict("E_NO_SITE_ID"), "no site id");
    check.isTrue(empty.hasConflict("E_NO_OPERATOR"), "no operator");
    check.isTrue(empty.hasConflict("E_NO_POSITION"), "no position");
    check.isTrue(empty.specification.empty(), "no specification without operator and position");

    SiteConfiguration noSectors = rooftop;
    noSectors.sectors = 0;
    check.isTrue(constructor.build(noSectors).hasConflict("E_NO_SECTORS"), "zero sectors");

    const auto bare = std::make_shared<LinkMatrix>();
    const Project noChoice = BSConstructor(bare).build(rooftop);
    check.isTrue(noChoice.hasConflict("E_NO_CHOICE"), "no choice and no default");
}

void longFeederRouteIsWarned(Check& check) {
    const BSConstructor constructor;
    SiteConfiguration config = rooftopSite(Operator::Mts);
    config.cableRouteLengthM = 65.0;
    const Project project = constructor.build(config);
    check.isTrue(project.hasConflict("W_FEEDER_LENGTH"), "feeder > 50 m");
    check.isTrue(project.isValid(), "warning does not invalidate the project");
}

void constructorStateIsSafe(Check& check) {
    BSConstructor constructor;
    bool thrown = false;
    try {
        constructor.switchOperator(Operator::T2);
    } catch (const std::logic_error&) {
        thrown = true;
    }
    check.isTrue(thrown, "switching without a project throws");
    check.isTrue(constructor.current() == nullptr, "no project yet");

    thrown = false;
    try {
        BSConstructor broken(nullptr);
    } catch (const std::invalid_argument&) {
        thrown = true;
    }
    check.isTrue(thrown, "null matrix is rejected");

    const Project& loaded = constructor.loadAct("Номер БС: 1\nОператор: Т2\nТип площадки: крыша\nабракадабра: 1\n");
    check.isTrue(loaded.parserWarnings.empty(), "unknown keys are ignored");
    constructor.loadAct("Номер БС: 1\nОператор: Т2\nТип площадки: крыша\nКабельная трасса: медь\n");
    const Project& switched = constructor.switchOperator(Operator::Mts);
    check.equal(switched.parserWarnings.size(), std::size_t{1}, "parser warnings survive a switch");
    constructor.reset();
    check.isTrue(constructor.current() == nullptr, "reset");

    const BSConstructor first;
    const BSConstructor second;
    check.isTrue(first.sharedMatrix() == second.sharedMatrix(), "typical matrix is shared");
}

void reportsCarrySiteData(Check& check) {
    BSConstructor constructor;
    const Project& project = constructor.loadAct(kDemoAct);
    const std::string csv = specificationCsv(project);
    check.isTrue(startsWith(csv, "\xEF\xBB\xBF"), "CSV has a UTF-8 BOM for Excel");
    check.isTrue(csv.find("Раздел;Код;Наименование;Ед. изм.;Кол-во;Связь") != std::string::npos, "CSV header");
    check.isTrue(csv.find("MTS-TX-OPT") != std::string::npos, "CSV item");
    check.isTrue(csv.find("МТС × Оптика") != std::string::npos, "CSV keeps the link of the line");

    const Attributes attributes = dwgAttributes(project);
    const auto valueOf = [&attributes](std::string_view key) {
        for (const auto& [name, value] : attributes) {
            if (name == key) {
                return value;
            }
        }
        return std::string();
    };
    check.equal(valueOf("SITE_ID"), std::string("78-04546"), "dwg site id");
    check.equal(valueOf("OPERATOR"), std::string("MTS"), "dwg operator");
    check.equal(valueOf("POSITION_TYPE"), std::string("GROUND_AMS"), "dwg position type");
    check.equal(valueOf("ROUTE_LENGTH"), std::string("47"), "dwg route length");

    const Project& megafon = constructor.switchOperator(Operator::Megafon);
    check.equal(dwgAttributes(megafon).front().second, std::string("78-04546"), "site id after switch");
    check.isTrue(technicalAssignment(megafon).find("78-04546") != std::string::npos, "ТЗ mentions the site");
    check.isTrue(formatReport(megafon).find("E_KM_OVERLOAD") != std::string::npos, "report lists conflicts");
}

}  // namespace

void registerConstructorTests(Registry& registry) {
    registry.push_back({"constructor/demo act", demoActBuildsValidProject});
    registry.push_back({"constructor/switching operators relinks the specification",
                        switchingOperatorsRelinksSpecification});
    registry.push_back({"constructor/switching operator changes quantities", switchingOperatorChangesQuantities});
    registry.push_back({"constructor/heavy operator overloads pipe stand", switchingToHeavyOperatorOverloadsPipeStand});
    registry.push_back({"constructor/heavy operator on tripod", heavyOperatorOnTripodIsValid});
    registry.push_back({"validation/incompatible equipment", incompatibleEquipmentIsDetected});
    registry.push_back({"validation/operator regulation", operatorRegulationIsEnforced});
    registry.push_back({"validation/defaults and missing data", defaultsAndMissingData});
    registry.push_back({"validation/long feeder route", longFeederRouteIsWarned});
    registry.push_back({"constructor/state is safe", constructorStateIsSafe});
    registry.push_back({"report/site data", reportsCarrySiteData});
}

}  // namespace bsc::test
