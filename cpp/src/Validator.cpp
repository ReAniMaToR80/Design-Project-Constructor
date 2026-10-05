#include "Validator.h"

#include "SafeString.h"

#include <algorithm>
#include <limits>
#include <string>
#include <utility>

namespace bsc {
namespace {

Conflict makeConflict(Severity severity, std::string code, std::string message) {
    return Conflict{severity, std::move(code), std::move(message)};
}

std::string quoted(std::string_view name) {
    std::string result = "«";
    result += name;
    result += "»";
    return result;
}

}  // namespace

Validator::Validator(const LinkMatrix& matrix) noexcept : matrix_(matrix) {}

std::vector<Conflict> Validator::checkConfiguration(const SiteConfiguration& resolved,
                                                    double cableRouteLengthM) const {
    std::vector<Conflict> conflicts;
    for (const Aspect aspect : kAllAspects) {
        const auto element = resolved.choice(aspect);
        if (!element) {
            continue;
        }
        if (resolved.op && !matrix_.isAllowed(*resolved.op, *element)) {
            conflicts.push_back(makeConflict(Severity::Error, "E_OPERATOR_REGULATION",
                                             "Элемент " + quoted(nameOf(*element)) +
                                                 " не предусмотрен регламентом оператора " +
                                                 quoted(nameOf(*resolved.op))));
        }
        if (resolved.position && !matrix_.isCompatible(*resolved.position, *element)) {
            conflicts.push_back(makeConflict(Severity::Error, "E_POSITION_INCOMPATIBLE",
                                             "Элемент " + quoted(nameOf(*element)) +
                                                 " несовместим с типом площадки " +
                                                 quoted(nameOf(*resolved.position))));
        }
    }
    if (resolved.cableRoute == Element::Feeder && cableRouteLengthM > kMaxFeederRouteM) {
        conflicts.push_back(makeConflict(Severity::Warning, "W_FEEDER_LENGTH",
                                         "Длина фидерной трассы " + text::formatNumber(cableRouteLengthM) +
                                             " м превышает " + text::formatNumber(kMaxFeederRouteM) +
                                             " м: рекомендуется оптика"));
    }
    return conflicts;
}

std::vector<Conflict> Validator::checkStructureLoad(const Specification& specification) const {
    double units = 0.0;
    double capacityKg = std::numeric_limits<double>::max();
    double mountedKg = 0.0;
    for (const SpecLine& line : specification) {
        if (line.item.capacityKg > 0.0) {
            units += line.quantity;
            capacityKg = std::min(capacityKg, line.item.capacityKg);
        }
        if (line.item.mountedOnStructure) {
            mountedKg += line.quantity * line.item.massKg;
        }
    }
    std::vector<Conflict> conflicts;
    if (units <= 0.0) {
        return conflicts;
    }
    const double requiredKg = mountedKg / units * kLoadSafetyFactor;
    const double utilization = requiredKg / capacityKg * 100.0;
    const std::string details = "расчетная нагрузка на единицу КМ " + text::formatNumber(requiredKg, 1) +
                                " кг (k=" + text::formatNumber(kLoadSafetyFactor) + ") при несущей способности " +
                                text::formatNumber(capacityKg, 1) + " кг, использование " +
                                text::formatNumber(utilization, 0) + "%";
    if (requiredKg > capacityKg) {
        conflicts.push_back(makeConflict(Severity::Error, "E_KM_OVERLOAD", "Перегрузка конструктива: " + details));
    } else {
        conflicts.push_back(makeConflict(Severity::Info, "I_KM_LOAD", "Несущая способность обеспечена: " + details));
    }
    return conflicts;
}

}  // namespace bsc
