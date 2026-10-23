// Tests of the safe string helpers and of the act (ППО) parser.
#include "ActParser.h"
#include "SafeString.h"
#include "TestRunner.h"

#include <string>

namespace bsc::test {
namespace {

void safeSetKeepsValueOnNullPointer(Check& check) {
    std::string value = "78-04546";
    const char* broken = nullptr;
    check.isFalse(text::safeSet(value, broken), "nullptr is rejected");
    check.equal(value, std::string("78-04546"), "value is kept");
    check.isTrue(text::safeSet(value, "  50-1234\r\n"), "valid C string is accepted");
    check.equal(value, std::string("50-1234"), "value is trimmed");
}

void safeSetIsBoundedForUnterminatedBuffers(Check& check) {
    const char buffer[4] = {'A', 'B', 'C', 'D'};  // no terminating zero
    std::string value;
    check.isTrue(text::safeSet(value, buffer, sizeof(buffer)), "bounded read");
    check.equal(value, std::string("ABCD"), "reads exactly maxBytes");
}

void sanitizeRepairsBrokenText(Check& check) {
    check.equal(text::sanitize("БС\x01\x7F№1", 64), std::string("БС  №1"), "control characters become spaces");
    check.equal(text::sanitize("A\xFF\xC0Z", 64), std::string("A??Z"), "invalid bytes become '?'");
    check.equal(text::sanitize("\xED\xA0\x80", 64), std::string("???"), "surrogates are rejected");
    // "Мачта" is 10 bytes; a limit of 5 must not cut the third letter in half.
    check.equal(text::sanitize("Мачта", 5), std::string("Ма"), "UTF-8 sequence is never cut");
    check.equal(text::sanitize("ОК\xD0", 64), std::string("ОК"), "truncated tail sequence is dropped");
}

void foldCaseHandlesCyrillic(Check& check) {
    check.equal(text::foldCase("МТС Ёлка Tele2 ПРЯМАЯ"), std::string("мтс елка tele2 прямая"), "fold");
    check.isTrue(text::containsWord("пао «мтс»", "мтс"), "word in quotes");
    check.isFalse(text::containsWord("кмтс2", "мтс"), "not inside another word");
}

void parseNumberIsLocaleIndependent(Check& check) {
    check.near(text::parseNumber("+36,100 м").value_or(0), 36.1, 1e-9, "comma decimal");
    check.near(text::parseNumber("H=38м").value_or(0), 38.0, 1e-9, "prefix text");
    check.isFalse(text::parseNumber("нет данных").has_value(), "no number");
}

void parsesStructuredAct(Check& check) {
    const ActParser parser;
    const ParseResult result = parser.parse(
        "АКТ ПРЕДПРОЕКТНОГО ОБСЛЕДОВАНИЯ\r\n"
        "Оператор: ПАО «МТС»\r\n"
        "Номер БС: 78-04546 (Орово)\r\n"
        "Тип площадки: Наземная АМС (опора ОДН)\r\n"
        "Высота подвеса АФУ: +36,100 м\r\n"
        "Количество секторов: 3\r\n"
        "Кабельная трасса: оптика\r\n"
        "Аппаратная: ТКШ\r\n"
        "Конструктив КМ: трубостойка\r\n");
    const SiteConfiguration& c = result.config;
    check.equal(c.siteId, std::string("78-04546"), "site id");
    check.isTrue(c.op == Operator::Mts, "operator");
    check.isTrue(c.position == PositionType::GroundMast, "position type");
    check.isTrue(c.cableRoute == Element::Optics, "cable route");
    check.isTrue(c.hardwareRoom == Element::ClimateCabinet, "hardware room");
    check.isTrue(c.structure == Element::CustomPipeStand, "structure");
    check.equal(c.sectors, 3U, "sectors");
    check.near(c.antennaHeightM.value_or(0), 36.1, 1e-9, "antenna height");
    check.isTrue(result.warnings.empty(), "no warnings");
}

void parsesRooftopAct(Check& check) {
    const ParseResult result = ActParser().parse(
        "Заказчик = Мегафон\nБС № 50-12345\nТип позиции = Крышная\nКонструктив = Трипод разгрузочный\n"
        "Аппаратная = Выгородка на техэтаже\nКабельная трасса = Фидер 7/8\nДлина кабельной трассы = 42 м\n");
    const SiteConfiguration& c = result.config;
    check.equal(c.siteId, std::string("50-12345"), "site id");
    check.isTrue(c.op == Operator::Megafon, "operator");
    check.isTrue(c.position == PositionType::Rooftop, "rooftop");
    check.isTrue(c.structure == Element::UnloadingTripod, "tripod");
    check.isTrue(c.hardwareRoom == Element::Enclosure, "enclosure");
    check.isTrue(c.cableRoute == Element::Feeder, "feeder");
    check.near(c.cableRouteLengthM.value_or(0), 42.0, 1e-9, "route length");
}

void parsesInputParserJson(Check& check) {
    // Output format of the existing input_parser (LLM) script.
    const ParseResult result = ActParser().parse(
        "{\n  \"Operator\": \"VIMPELCOM\",\n  \"Site_ID\": \"77-0012\",\n  \"Position_Type\": \"Roof\",\n"
        "  \"Cabinet_Type\": \"Климатический контейнер\",\n  \"Antenna_Height\": \"24.5\"\n}\n");
    const SiteConfiguration& c = result.config;
    check.equal(c.siteId, std::string("77-0012"), "site id");
    check.isTrue(c.op == Operator::Vimpelcom, "operator");
    check.isTrue(c.position == PositionType::Rooftop, "rooftop");
    check.isTrue(c.hardwareRoom == Element::Container, "container wins over cabinet");
    check.near(c.antennaHeightM.value_or(0), 24.5, 1e-9, "height");
}

void parsesTrackerSummaryLine(Check& check) {
    // Format of the tech summary written to the tracker by input_parser.
    const ParseResult result = ActParser().parse("БС 78-04546 Т2 | Тип: ODN | Подвес: +31.1 | КШ: шкаф Т2");
    const SiteConfiguration& c = result.config;
    check.equal(c.siteId, std::string("78-04546"), "site id from free text");
    check.isTrue(c.op == Operator::T2, "operator from free text");
    check.isTrue(c.position == PositionType::GroundMast, "ODN is a ground mast");
    check.isTrue(c.hardwareRoom == Element::ClimateCabinet, "cabinet");
    check.near(c.antennaHeightM.value_or(0), 31.1, 1e-9, "height");
}

void freeTextAmbiguityIsNotGuessed(Check& check) {
    const ParseResult result = ActParser().parse("Совмещение МТС и Мегафон на одной опоре, БС №1-2");
    check.isFalse(result.config.op.has_value(), "several operators -> unknown");
    check.equal(result.config.siteId, std::string("1-2"), "site id");
}

void unknownValuesProduceWarnings(Check& check) {
    const ParseResult result = ActParser().parse("Оператор: Рога и копыта\nКоличество секторов: 99\n");
    check.isFalse(result.config.op.has_value(), "operator unknown");
    check.equal(result.config.sectors, 3U, "sectors keep the default");
    check.equal(result.warnings.size(), std::size_t{2}, "two warnings");
}

void survivesBrokenInput(Check& check) {
    const ActParser parser;
    const ParseResult fromNull = parser.parse(nullptr, 100);
    check.isTrue(fromNull.config.siteId.empty(), "nullptr gives an empty result");
    check.equal(fromNull.warnings.size(), std::size_t{1}, "nullptr is reported");

    std::string garbage;
    for (int i = 0; i < 4096; ++i) {
        garbage.push_back(static_cast<char>((i * 37) % 256));
    }
    const ParseResult fromGarbage = parser.parse(garbage);
    check.isTrue(fromGarbage.config.siteId.size() <= ActParser::kMaxSiteIdBytes, "garbage is bounded");

    const std::string huge(ActParser::kMaxActBytes + 10, 'x');
    check.equal(parser.parse(huge).warnings.size(), std::size_t{1}, "oversized act is truncated with a warning");

    const ParseResult withBom = parser.parse("\xEF\xBB\xBFОператор: Т2\nНомер БС: 12\xFF-3");
    check.isTrue(withBom.config.op == Operator::T2, "BOM is skipped");
    check.equal(withBom.config.siteId, std::string("12"), "invalid byte ends the site id");
}

}  // namespace

void registerParserTests(Registry& registry) {
    registry.push_back({"safe/safeSet keeps value on nullptr", safeSetKeepsValueOnNullPointer});
    registry.push_back({"safe/safeSet is bounded", safeSetIsBoundedForUnterminatedBuffers});
    registry.push_back({"safe/sanitize repairs broken text", sanitizeRepairsBrokenText});
    registry.push_back({"safe/foldCase handles Cyrillic", foldCaseHandlesCyrillic});
    registry.push_back({"safe/parseNumber is locale independent", parseNumberIsLocaleIndependent});
    registry.push_back({"parser/structured act", parsesStructuredAct});
    registry.push_back({"parser/rooftop act", parsesRooftopAct});
    registry.push_back({"parser/input_parser JSON", parsesInputParserJson});
    registry.push_back({"parser/tracker summary line", parsesTrackerSummaryLine});
    registry.push_back({"parser/free text ambiguity", freeTextAmbiguityIsNotGuessed});
    registry.push_back({"parser/unknown values produce warnings", unknownValuesProduceWarnings});
    registry.push_back({"parser/survives broken input", survivesBrokenInput});
}

}  // namespace bsc::test
