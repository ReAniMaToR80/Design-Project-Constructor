// LinkMatrix.h — base of typical solutions (матрица связей операторов).
//
// Each enum value of every dimension is a point link. Facts are stored as
// nested doublets ((a, b), c):
//   ((operator, element),  allowedMarker)    — operator regulation allows element
//   ((position, element),  compatibleMarker) — element fits the position type
//   ((position, element),  defaultMarker)    — default choice for the aspect
//   ((operator, element),  item)             — operator spec item for element
//   ((operator, position), item)             — operator base item for position
//   ((position, element),  item)             — position specific item (КМ, МЗ)
#pragma once

#include "LinksStore.h"
#include "Types.h"

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace bsc {

struct SpecItem {
    std::string code;
    std::string name;
    std::string unit;
    Section section = Section::TX;
    QuantityBasis basis = QuantityBasis::Fixed;
    double quantity = 1.0;
    double massKg = 0.0;              // mass of one unit
    bool mountedOnStructure = false;  // loads the КМ structure
    double capacityKg = 0.0;          // > 0 only for load-bearing structures (per unit)
};

class LinkMatrix {
public:
    LinkMatrix();

    // Typical solutions according to the operator regulations (see TypicalSolutions.cpp).
    // The instance is built once and shared: it is immutable after construction.
    static std::shared_ptr<const LinkMatrix> makeTypical();

    // --- Building the regulation -------------------------------------------
    void allow(Operator op, Element element);
    void makeCompatible(PositionType position, Element element);
    // Only one default per (position, aspect): the store is append-only, so a
    // second, different default throws std::logic_error.
    void setDefault(PositionType position, Element element);

    void addItem(Operator op, Element element, SpecItem item);
    void addItem(Operator op, PositionType position, SpecItem item);
    void addItem(PositionType position, Element element, SpecItem item);

    // --- Queries ------------------------------------------------------------
    bool isAllowed(Operator op, Element element) const noexcept;
    bool isCompatible(PositionType position, Element element) const noexcept;
    std::optional<Element> defaultFor(PositionType position, Aspect aspect) const noexcept;

    std::vector<SpecItem> itemsFor(Operator op, Element element) const;
    std::vector<SpecItem> itemsFor(Operator op, PositionType position) const;
    std::vector<SpecItem> itemsFor(PositionType position, Element element) const;

    LinkId pointOf(Operator op) const noexcept;
    LinkId pointOf(PositionType position) const noexcept;
    LinkId pointOf(Element element) const noexcept;

    const LinksStore& links() const noexcept;
    std::size_t itemCount() const noexcept;

private:
    void tag(LinkId a, LinkId b, LinkId marker);
    bool hasTag(LinkId a, LinkId b, LinkId marker) const noexcept;
    void attachItem(LinkId a, LinkId b, SpecItem item);
    std::vector<SpecItem> collectItems(LinkId a, LinkId b) const;

    LinksStore store_;
    std::array<LinkId, kAllOperators.size()> operatorPoints_{};
    std::array<LinkId, kAllPositionTypes.size()> positionPoints_{};
    std::array<LinkId, kAllElements.size()> elementPoints_{};
    LinkId allowedMarker_ = kNullLink;
    LinkId compatibleMarker_ = kNullLink;
    LinkId defaultMarker_ = kNullLink;
    std::vector<SpecItem> items_;
    std::unordered_map<LinkId, std::size_t> itemByPoint_;
};

}  // namespace bsc
