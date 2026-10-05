#include "Report.h"

#include "SafeString.h"

#include <sstream>
#include <string_view>

namespace bsc {
namespace {

template <typename Enum>
std::string nameOrDash(const std::optional<Enum>& value) {
    return value ? std::string(nameOf(*value)) : std::string("-");
}

template <typename Enum>
std::string codeOrDash(const std::optional<Enum>& value) {
    return value ? std::string(codeOf(*value)) : std::string("-");
}

std::string csvField(std::string_view value) {
    if (value.find_first_of(";\"\n") == std::string_view::npos) {
        return std::string(value);
    }
    std::string escaped = "\"";
    for (const char c : value) {
        if (c == '"') escaped += '"';
        escaped += c;
    }
    escaped += '"';
    return escaped;
}

std::string heightText(const SiteConfiguration& config) {
    return config.antennaHeightM ? "+" + text::formatNumber(*config.antennaHeightM, 3) + " м" : std::string("-");
}

}  // namespace

std::string specificationCsv(const Project& project) {
    std::ostringstream out;
    out << "\xEF\xBB\xBF" << "Раздел;Код;Наименование;Ед. изм.;Кол-во;Связь\n";
    for (const SpecLine& line : project.specification) {
        out << nameOf(line.item.section) << ';' << csvField(line.item.code) << ';' << csvField(line.item.name)
            << ';' << csvField(line.item.unit) << ';' << text::formatNumber(line.quantity) << ';'
            << csvField(line.origin) << '\n';
    }
    return out.str();
}

std::string technicalAssignment(const Project& project) {
    const SiteConfiguration& c = project.resolved;
    std::ostringstream out;
    out << "ТЕХНИЧЕСКОЕ ЗАДАНИЕ НА ПРОЕКТИРОВАНИЕ БС №" << (c.siteId.empty() ? "-" : c.siteId) << "\n";
    out << "1. Заказчик: " << nameOrDash(c.op) << "\n";
    out << "2. Тип позиции: " << nameOrDash(c.position) << "\n";
    out << "3. Количество секторов: " << c.sectors << ", высота подвеса АФУ: " << heightText(c) << "\n";
    out << "4. Кабельная трасса: " << nameOrDash(c.cableRoute) << ", длина "
        << text::formatNumber(project.cableRouteLengthM) << " м\n";
    out << "5. Аппаратная: " << nameOrDash(c.hardwareRoom) << "\n";
    out << "6. Конструктив КМ: " << nameOrDash(c.structure) << "\n";
    out << "7. Состав проекта: ТХ, КМ, ЭМ, МЗ; спецификация — " << project.specification.size() << " поз.\n";
    if (!project.isValid()) {
        out << "ВНИМАНИЕ: есть неразрешенные конфликты, ТЗ не может быть выдано в работу.\n";
    }
    return out.str();
}

Attributes dwgAttributes(const Project& project) {
    const SiteConfiguration& c = project.resolved;
    return {
        {"SITE_ID", c.siteId},
        {"OPERATOR", codeOrDash(c.op)},
        {"OPERATOR_NAME", nameOrDash(c.op)},
        {"POSITION_TYPE", codeOrDash(c.position)},
        {"CABLE_ROUTE", codeOrDash(c.cableRoute)},
        {"HARDWARE_ROOM", codeOrDash(c.hardwareRoom)},
        {"STRUCTURE", codeOrDash(c.structure)},
        {"SECTORS", std::to_string(c.sectors)},
        {"ANTENNA_HEIGHT", c.antennaHeightM ? text::formatNumber(*c.antennaHeightM, 3) : std::string("-")},
        {"ROUTE_LENGTH", text::formatNumber(project.cableRouteLengthM)},
    };
}

std::string formatReport(const Project& project) {
    std::ostringstream out;
    out << technicalAssignment(project);
    if (!project.parserWarnings.empty()) {
        out << "\nЗамечания разбора акта:\n";
        for (const std::string& warning : project.parserWarnings) {
            out << "  - " << warning << "\n";
        }
    }
    out << "\nКонфликты и проверки:\n";
    for (const Conflict& conflict : project.conflicts) {
        out << "  [" << nameOf(conflict.severity) << "] " << conflict.code << ": " << conflict.message << "\n";
    }
    out << "\nСпецификация:\n";
    for (const SpecLine& line : project.specification) {
        out << "  " << nameOf(line.item.section) << "  " << line.item.code << "  " << line.item.name << " — "
            << text::formatNumber(line.quantity) << " " << line.item.unit << "  (" << line.origin << ")\n";
    }
    return out.str();
}

}  // namespace bsc
