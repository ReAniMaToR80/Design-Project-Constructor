// Tests of the L -> L^2 links store and of the typical solutions matrix.
#include "LinkMatrix.h"
#include "LinksStore.h"
#include "TestRunner.h"

#include <stdexcept>
#include <string>

namespace bsc::test {
namespace {

void pointsReferToThemselves(Check& check) {
    LinksStore store;
    const LinkId a = store.createPoint();
    const LinkId b = store.createPoint();
    check.isTrue(store.isPoint(a) && store.isPoint(b), "created links are points");
    check.equal(store.size(), std::size_t{2}, "two links stored");
    check.isFalse(store.exists(kNullLink), "null link does not exist");
    check.isFalse(store.get(42).has_value(), "unknown link is not returned");
}

void doubletsAreUnique(Check& check) {
    LinksStore store;
    const LinkId a = store.createPoint();
    const LinkId b = store.createPoint();
    const LinkId ab = store.getOrCreate(a, b);
    check.equal(store.getOrCreate(a, b), ab, "same pair gives the same link");
    check.isFalse(store.search(b, a).has_value(), "pairs are ordered");
    const auto link = store.get(ab);
    check.isTrue(link && link->source == a && link->target == b, "doublet keeps source and target");
    check.isFalse(store.isPoint(ab), "doublet is not a point");

    // L -> L^2: a link to a link.
    const LinkId nested = store.getOrCreate(ab, a);
    check.equal(store.get(nested)->source, ab, "nested doublet refers to a doublet");
    check.equal(store.outgoing(ab).size(), std::size_t{1}, "outgoing index of the doublet");
}

void invalidLinksAreRejected(Check& check) {
    LinksStore store;
    const LinkId a = store.createPoint();
    bool thrown = false;
    try {
        store.getOrCreate(a, 1000);
    } catch (const std::invalid_argument&) {
        thrown = true;
    }
    check.isTrue(thrown, "dangling target throws std::invalid_argument");
    check.equal(store.size(), std::size_t{1}, "store is unchanged after a failed insert");
    check.isTrue(store.outgoing(999).empty(), "outgoing of an unknown link is empty");
}

void typicalMatrixAllowsAllVariantsForEveryOperator(Check& check) {
    const auto matrix = LinkMatrix::makeTypical();
    for (const Operator op : kAllOperators) {
        for (const Element element : kAllElements) {
            check.isTrue(matrix->isAllowed(op, element),
                         std::string(codeOf(op)) + " allows " + std::string(codeOf(element)));
        }
    }
}

void positionCompatibility(Check& check) {
    const auto matrix = LinkMatrix::makeTypical();
    check.isTrue(matrix->isCompatible(PositionType::Rooftop, Element::UnloadingTripod), "roof + tripod");
    check.isTrue(matrix->isCompatible(PositionType::Rooftop, Element::Enclosure), "roof + enclosure");
    check.isFalse(matrix->isCompatible(PositionType::Rooftop, Element::Container), "roof + container");
    check.isTrue(matrix->isCompatible(PositionType::GroundMast, Element::Container), "ground + container");
    check.isFalse(matrix->isCompatible(PositionType::GroundMast, Element::UnloadingTripod), "ground + tripod");
    check.isFalse(matrix->isCompatible(PositionType::GroundMast, Element::Enclosure), "ground + enclosure");
    for (const PositionType position : kAllPositionTypes) {
        for (const Aspect aspect : kAllAspects) {
            const auto fallback = matrix->defaultFor(position, aspect);
            check.isTrue(fallback && aspectOf(*fallback) == aspect && matrix->isCompatible(position, *fallback),
                         "default exists, has the right aspect and is compatible");
        }
    }
}

void operatorItemsAreOrthogonal(Check& check) {
    const auto matrix = LinkMatrix::makeTypical();
    for (const Operator op : kAllOperators) {
        const std::string prefix = std::string(codeOf(op)) + "-";
        for (const Element element : kAllElements) {
            const auto items = matrix->itemsFor(op, element);
            check.isFalse(items.empty(), prefix + std::string(codeOf(element)) + " has items");
            for (const SpecItem& item : items) {
                check.isTrue(item.code.rfind(prefix, 0) == 0, item.code + " belongs to " + prefix);
            }
        }
    }
    const auto mts = matrix->itemsFor(Operator::Mts, Element::ClimateCabinet);
    const auto megafon = matrix->itemsFor(Operator::Megafon, Element::ClimateCabinet);
    check.isTrue(mts.front().name != megafon.front().name, "different operators give different cabinets");
    check.equal(mts.front().name, std::string("Климатический шкаф Интеркросс Тип 5"), "MTS cabinet");
}

void itemsAreStoredAsNestedDoublets(Check& check) {
    LinkMatrix matrix;
    SpecItem item;
    item.code = "X-1";
    matrix.addItem(Operator::T2, Element::Feeder, item);
    const LinksStore& links = matrix.links();
    const auto pair = links.search(matrix.pointOf(Operator::T2), matrix.pointOf(Element::Feeder));
    check.isTrue(pair.has_value(), "(operator, element) doublet exists");
    check.equal(links.outgoing(*pair).size(), std::size_t{1}, "one ((operator, element), item) link");
    check.isTrue(links.isPoint(links.get(links.outgoing(*pair).front())->target), "item is a point");
    check.isFalse(matrix.isAllowed(Operator::T2, Element::Feeder), "an item does not imply an allowance");
    check.equal(matrix.itemsFor(Operator::T2, Element::Feeder).size(), std::size_t{1}, "item is found");
    check.isTrue(matrix.itemsFor(Operator::Mts, Element::Feeder).empty(), "other operator has no items");
}

void defaultCannotBeRedefined(Check& check) {
    LinkMatrix matrix;
    matrix.setDefault(PositionType::Rooftop, Element::Optics);
    matrix.setDefault(PositionType::Rooftop, Element::Optics);  // idempotent
    bool thrown = false;
    try {
        matrix.setDefault(PositionType::Rooftop, Element::Feeder);
    } catch (const std::logic_error&) {
        thrown = true;
    }
    check.isTrue(thrown, "a different default throws");
    check.isTrue(matrix.defaultFor(PositionType::Rooftop, Aspect::CableRoute) == Element::Optics, "default kept");
}

}  // namespace

void registerLinksTests(Registry& registry) {
    registry.push_back({"links/points refer to themselves", pointsReferToThemselves});
    registry.push_back({"links/doublets are unique and ordered", doubletsAreUnique});
    registry.push_back({"links/invalid links are rejected", invalidLinksAreRejected});
    registry.push_back({"matrix/all operators allow all variants", typicalMatrixAllowsAllVariantsForEveryOperator});
    registry.push_back({"matrix/position compatibility and defaults", positionCompatibility});
    registry.push_back({"matrix/operator items are orthogonal", operatorItemsAreOrthogonal});
    registry.push_back({"matrix/items are nested doublets", itemsAreStoredAsNestedDoublets});
    registry.push_back({"matrix/default cannot be redefined", defaultCannotBeRedefined});
}

}  // namespace bsc::test
