#include "ActParser.h"

#include "SafeString.h"

#include <algorithm>
#include <array>
#include <string>
#include <utility>

namespace bsc {
namespace {

enum class Field { SiteId, Operator, Position, RouteLength, CableRoute, HardwareRoom, Structure, Sectors, Height };

struct KeySynonym {
    Field field;
    std::string_view key;  // folded
    bool exact;            // whole key must match (for short abbreviations)
};

// Order matters: the first matching synonym wins ("длина трассы" before "трасса").
constexpr std::array<KeySynonym, 31> kKeys{{
    {Field::SiteId, "site_id", false},
    {Field::SiteId, "site id", false},
    {Field::SiteId, "номер бс", false},
    {Field::SiteId, "бс №", false},
    {Field::SiteId, "номер позиции", false},
    {Field::SiteId, "номер объекта", false},
    {Field::Operator, "оператор", false},
    {Field::Operator, "operator", false},
    {Field::Operator, "заказчик", false},
    {Field::Position, "тип площадки", false},
    {Field::Position, "тип позиции", false},
    {Field::Position, "position_type", false},
    {Field::Position, "position type", false},
    {Field::Position, "тип", true},
    {Field::RouteLength, "длина", false},
    {Field::RouteLength, "route_length", false},
    {Field::CableRoute, "кабельная трасса", false},
    {Field::CableRoute, "cable_route", false},
    {Field::CableRoute, "трасса", true},
    {Field::HardwareRoom, "аппаратная", false},
    {Field::HardwareRoom, "hardware_room", false},
    {Field::HardwareRoom, "cabinet_type", false},
    {Field::HardwareRoom, "кш", true},
    {Field::Structure, "конструктив", false},
    {Field::Structure, "structure", false},
    {Field::Structure, "км", true},
    {Field::Sectors, "сектор", false},
    {Field::Sectors, "sectors", false},
    {Field::Height, "высота подвеса", false},
    {Field::Height, "antenna_height", false},
    {Field::Height, "подвес", true},
}};

struct ValueSynonym {
    std::string_view word;  // folded stem
    bool wholeWord;         // must not be a part of a longer word
};

struct OperatorWords {
    Operator op;
    std::array<ValueSynonym, 4> words;
};

constexpr std::array<OperatorWords, 4> kOperatorWords{{
    {Operator::Mts, {{{"мтс", true}, {"mts", true}, {"", true}, {"", true}}}},
    {Operator::T2, {{{"т2", true}, {"t2", true}, {"tele2", true}, {"теле2", true}}}},
    {Operator::Vimpelcom, {{{"вымпелком", true}, {"билайн", true}, {"beeline", true}, {"vimpelcom", true}}}},
    {Operator::Megafon, {{{"мегафон", true}, {"megafon", true}, {"", true}, {"", true}}}},
}};

struct ElementWords {
    Element element;
    std::array<ValueSynonym, 4> words;
};

// Order matters: "климатический контейнер" is a container, not a cabinet.
constexpr std::array<ElementWords, 7> kElementWords{{
    {Element::Feeder, {{{"фидер", false}, {"feeder", false}, {"коакс", false}, {"", false}}}},
    {Element::Optics, {{{"оптик", false}, {"optic", false}, {"fiber", false}, {"вок", true}}}},
    {Element::Enclosure, {{{"выгородк", false}, {"enclosure", false}, {"", false}, {"", false}}}},
    {Element::Container, {{{"контейнер", false}, {"container", false}, {"", false}, {"", false}}}},
    {Element::ClimateCabinet, {{{"ткш", true}, {"шкаф", false}, {"cabinet", false}, {"кш", true}}}},
    {Element::CustomPipeStand, {{{"трубостойк", false}, {"pipe", false}, {"", false}, {"", false}}}},
    {Element::UnloadingTripod, {{{"трипод", false}, {"tripod", false}, {"треног", false}, {"", false}}}},
}};

constexpr std::array<std::string_view, 3> kRooftopWords{"крыш", "кровл", "roof"};
constexpr std::array<std::string_view, 10> kGroundWords{"амс",  "наземн", "одн",  "опор",  "мачт",
                                                        "башн", "ground", "mast", "tower", "odn"};

template <std::size_t N>
bool matches(std::string_view folded, const std::array<ValueSynonym, N>& words) noexcept {
    for (const ValueSynonym& synonym : words) {
        if (synonym.word.empty()) {
            continue;
        }
        const bool found = synonym.wholeWord ? text::containsWord(folded, synonym.word)
                                             : folded.find(synonym.word) != std::string_view::npos;
        if (found) {
            return true;
        }
    }
    return false;
}

template <std::size_t N>
bool containsAny(std::string_view folded, const std::array<std::string_view, N>& stems) noexcept {
    for (const std::string_view stem : stems) {
        if (folded.find(stem) != std::string_view::npos) {
            return true;
        }
    }
    return false;
}

std::optional<Field> classifyKey(std::string_view rawKey) {
    const std::string key = text::foldCase(text::trim(rawKey));
    for (const KeySynonym& synonym : kKeys) {
        const bool hit = synonym.exact ? key == synonym.key : key.find(synonym.key) != std::string::npos;
        if (hit) {
            return synonym.field;
        }
    }
    return std::nullopt;
}

// Strips JSON-like decoration: quotes, trailing commas.
std::string_view stripDecoration(std::string_view value) noexcept {
    value = text::trim(value);
    while (!value.empty() && (value.back() == ',' || value.back() == ';')) {
        value.remove_suffix(1);
        value = text::trim(value);
    }
    while (!value.empty() && (value.front() == '"' || value.front() == '\'' || value.front() == '{')) {
        value.remove_prefix(1);
    }
    while (!value.empty() && (value.back() == '"' || value.back() == '\'' || value.back() == '}')) {
        value.remove_suffix(1);
    }
    return text::trim(value);
}

bool isSiteIdChar(char c) noexcept {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-' || c == '_' ||
           c == '/' || c == '.';
}

class Parser {
public:
    explicit Parser(ParseResult& result) : result_(result) {}

    void parseSegment(std::string_view segment) {
        const std::size_t separator = segment.find_first_of(":=");
        if (separator == std::string_view::npos) {
            return;
        }
        const auto field = classifyKey(stripDecoration(segment.substr(0, separator)));
        if (!field) {
            return;
        }
        apply(*field, stripDecoration(segment.substr(separator + 1)));
    }

    void apply(Field field, std::string_view value) {
        SiteConfiguration& config = result_.config;
        switch (field) {
            case Field::SiteId:
                if (config.siteId.empty()) {
                    text::safeSet(config.siteId, ActParser::extractSiteId(value), ActParser::kMaxSiteIdBytes);
                }
                break;
            case Field::Operator:
                assign(config.op, ActParser::detectOperator(value), "оператор", value);
                break;
            case Field::Position:
                assign(config.position, ActParser::detectPositionType(value), "тип площадки", value);
                break;
            case Field::CableRoute:
                assign(config.cableRoute, ActParser::detectElement(value, Aspect::CableRoute), "кабельная трасса",
                       value);
                break;
            case Field::HardwareRoom:
                assign(config.hardwareRoom, ActParser::detectElement(value, Aspect::HardwareRoom), "аппаратная",
                       value);
                break;
            case Field::Structure:
                assign(config.structure, ActParser::detectElement(value, Aspect::Structure), "конструктив КМ",
                       value);
                break;
            case Field::Sectors: {
                const auto number = text::parseNumber(value);
                if (number && *number >= 1.0 && *number <= ActParser::kMaxSectors) {
                    config.sectors = static_cast<unsigned>(*number);
                } else {
                    warn("количество секторов", value);
                }
                break;
            }
            case Field::Height:
                assignLength(config.antennaHeightM, value, "высота подвеса");
                break;
            case Field::RouteLength:
                assignLength(config.cableRouteLengthM, value, "длина кабельной трассы");
                break;
        }
    }

private:
    template <typename T>
    void assign(std::optional<T>& target, std::optional<T> value, std::string_view what, std::string_view raw) {
        if (!value) {
            warn(what, raw);
        } else if (!target) {
            target = value;
        }
    }

    void assignLength(std::optional<double>& target, std::string_view value, std::string_view what) {
        const auto number = text::parseNumber(value);
        if (!number || *number <= 0.0 || *number > 1000.0) {
            warn(what, value);
        } else if (!target) {
            target = number;
        }
    }

    void warn(std::string_view what, std::string_view raw) {
        std::string message = "Не распознано значение поля «";
        message += what;
        message += "»: '";
        message += text::sanitize(raw, 80);
        message += "'";
        result_.warnings.push_back(std::move(message));
    }

    ParseResult& result_;
};

// "БС №78-04546", "BS 78-04546", "бс№ 78-04546" anywhere in the folded text.
std::string findSiteIdInFreeText(std::string_view folded) {
    for (const std::string_view marker : {std::string_view("бс"), std::string_view("bs")}) {
        for (std::size_t pos = folded.find(marker); pos != std::string_view::npos;
             pos = folded.find(marker, pos + 1)) {
            if (!text::startsWord(folded, pos)) {
                continue;  // "jobs", "собс..." are not markers
            }
            std::string_view rest = folded.substr(pos + marker.size());
            while (!rest.empty() && (rest.front() == ' ' || rest.front() == '#' || rest.front() == '-')) {
                rest.remove_prefix(1);
            }
            if (rest.substr(0, 3) == "№") {
                rest.remove_prefix(3);
            }
            rest = text::trim(rest);
            if (!rest.empty() && rest.front() >= '0' && rest.front() <= '9') {
                std::string id = ActParser::extractSiteId(rest);
                if (!id.empty()) {
                    return id;
                }
            }
        }
    }
    return {};
}

std::optional<Operator> findOperatorInFreeText(std::string_view folded) {
    std::optional<Operator> found;
    for (const OperatorWords& entry : kOperatorWords) {
        if (matches(folded, entry.words)) {
            if (found) {
                return std::nullopt;  // ambiguous: several operators are mentioned
            }
            found = entry.op;
        }
    }
    return found;
}

}  // namespace

ParseResult ActParser::parse(const char* data, std::size_t size) const {
    if (data == nullptr) {
        ParseResult result;
        result.warnings.emplace_back("Пустой указатель на текст акта");
        return result;
    }
    return parse(std::string_view(data, size));
}

ParseResult ActParser::parse(std::string_view text) const {
    ParseResult result;
    if (text.size() > kMaxActBytes) {
        text = text.substr(0, kMaxActBytes);
        result.warnings.emplace_back("Текст акта обрезан до 1 МиБ");
    }
    // Replaces broken UTF-8 and control characters except line breaks.
    std::string clean;
    clean.reserve(text.size());
    std::size_t lineStart = 0;
    while (lineStart <= text.size()) {
        const std::size_t lineEnd = std::min(text.find('\n', lineStart), text.size());
        clean += text::sanitize(text.substr(lineStart, lineEnd - lineStart), kMaxActBytes);
        clean += '\n';
        lineStart = lineEnd + 1;
    }
    std::string_view view = clean;
    if (view.substr(0, 3) == "\xEF\xBB\xBF") {
        view.remove_prefix(3);
    }

    Parser parser(result);
    std::size_t pos = 0;
    while (pos < view.size()) {
        const std::size_t lineEnd = view.find('\n', pos);
        const std::string_view line = view.substr(pos, lineEnd - pos);
        std::size_t segmentStart = 0;
        while (segmentStart <= line.size()) {
            const std::size_t bar = std::min(line.find('|', segmentStart), line.size());
            parser.parseSegment(line.substr(segmentStart, bar - segmentStart));
            segmentStart = bar + 1;
        }
        pos = lineEnd == std::string_view::npos ? view.size() : lineEnd + 1;
    }

    const std::string folded = text::foldCase(view);
    if (result.config.siteId.empty()) {
        text::safeSet(result.config.siteId, findSiteIdInFreeText(folded), kMaxSiteIdBytes);
    }
    if (!result.config.op) {
        result.config.op = findOperatorInFreeText(folded);
    }
    return result;
}

std::optional<Operator> ActParser::detectOperator(std::string_view value) {
    const std::string folded = text::foldCase(value);
    if (const auto byCode = operatorFromCode(text::trim(value))) {
        return byCode;
    }
    return findOperatorInFreeText(folded);
}

std::optional<PositionType> ActParser::detectPositionType(std::string_view value) {
    const std::string folded = text::foldCase(value);
    if (const auto byCode = positionTypeFromCode(text::trim(value))) {
        return byCode;
    }
    // "АМС на крыше" is a rooftop position: the roof keyword has priority.
    if (containsAny(folded, kRooftopWords)) {
        return PositionType::Rooftop;
    }
    if (containsAny(folded, kGroundWords)) {
        return PositionType::GroundMast;
    }
    return std::nullopt;
}

std::optional<Element> ActParser::detectElement(std::string_view value, Aspect aspect) {
    const std::string folded = text::foldCase(value);
    if (const auto byCode = elementFromCode(text::trim(value))) {
        if (aspectOf(*byCode) == aspect) {
            return byCode;
        }
    }
    for (const ElementWords& entry : kElementWords) {
        if (aspectOf(entry.element) == aspect && matches(folded, entry.words)) {
            return entry.element;
        }
    }
    return std::nullopt;
}

std::string ActParser::extractSiteId(std::string_view value) {
    // Skip everything up to the first digit/latin letter (e.g. "БС №").
    std::size_t start = 0;
    while (start < value.size()) {
        const char c = value[start];
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            break;
        }
        ++start;
    }
    std::size_t end = start;
    while (end < value.size() && isSiteIdChar(value[end])) {
        ++end;
    }
    std::string_view token = value.substr(start, end - start);
    while (!token.empty() && (token.back() == '.' || token.back() == '-' || token.back() == '/')) {
        token.remove_suffix(1);
    }
    std::string id;
    text::safeSet(id, token, kMaxSiteIdBytes);
    return id;
}

}  // namespace bsc
