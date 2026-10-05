#include "Project.h"

namespace bsc {

std::string_view nameOf(Severity value) noexcept {
    switch (value) {
        case Severity::Info: return "INFO";
        case Severity::Warning: return "WARNING";
        case Severity::Error: return "ERROR";
    }
    return "?";
}

bool Project::isValid() const noexcept {
    for (const Conflict& conflict : conflicts) {
        if (conflict.severity == Severity::Error) {
            return false;
        }
    }
    return true;
}

bool Project::hasConflict(std::string_view code) const noexcept {
    for (const Conflict& conflict : conflicts) {
        if (conflict.code == code) {
            return true;
        }
    }
    return false;
}

double Project::quantityOf(std::string_view itemCode) const noexcept {
    double total = 0.0;
    for (const SpecLine& line : specification) {
        if (line.item.code == itemCode) {
            total += line.quantity;
        }
    }
    return total;
}

}  // namespace bsc
