#include "LinkMatrix.h"

#include <stdexcept>
#include <utility>

namespace bsc {
namespace {

template <typename Enum>
constexpr std::size_t indexOf(Enum value) noexcept {
    return static_cast<std::size_t>(value);
}

}  // namespace

LinkMatrix::LinkMatrix() {
    for (const Operator op : kAllOperators) {
        operatorPoints_[indexOf(op)] = store_.createPoint();
    }
    for (const PositionType position : kAllPositionTypes) {
        positionPoints_[indexOf(position)] = store_.createPoint();
    }
    for (const Element element : kAllElements) {
        elementPoints_[indexOf(element)] = store_.createPoint();
    }
    allowedMarker_ = store_.createPoint();
    compatibleMarker_ = store_.createPoint();
    defaultMarker_ = store_.createPoint();
}

void LinkMatrix::allow(Operator op, Element element) {
    tag(pointOf(op), pointOf(element), allowedMarker_);
}

void LinkMatrix::makeCompatible(PositionType position, Element element) {
    tag(pointOf(position), pointOf(element), compatibleMarker_);
}

void LinkMatrix::setDefault(PositionType position, Element element) {
    const auto current = defaultFor(position, aspectOf(element));
    if (current && *current != element) {
        throw std::logic_error("LinkMatrix: default for this position and aspect is already set");
    }
    tag(pointOf(position), pointOf(element), defaultMarker_);
}

void LinkMatrix::addItem(Operator op, Element element, SpecItem item) {
    attachItem(pointOf(op), pointOf(element), std::move(item));
}

void LinkMatrix::addItem(Operator op, PositionType position, SpecItem item) {
    attachItem(pointOf(op), pointOf(position), std::move(item));
}

void LinkMatrix::addItem(PositionType position, Element element, SpecItem item) {
    attachItem(pointOf(position), pointOf(element), std::move(item));
}

bool LinkMatrix::isAllowed(Operator op, Element element) const noexcept {
    return hasTag(pointOf(op), pointOf(element), allowedMarker_);
}

bool LinkMatrix::isCompatible(PositionType position, Element element) const noexcept {
    return hasTag(pointOf(position), pointOf(element), compatibleMarker_);
}

std::optional<Element> LinkMatrix::defaultFor(PositionType position, Aspect aspect) const noexcept {
    for (const Element element : kAllElements) {
        if (aspectOf(element) == aspect && hasTag(pointOf(position), pointOf(element), defaultMarker_)) {
            return element;
        }
    }
    return std::nullopt;
}

std::vector<SpecItem> LinkMatrix::itemsFor(Operator op, Element element) const {
    return collectItems(pointOf(op), pointOf(element));
}

std::vector<SpecItem> LinkMatrix::itemsFor(Operator op, PositionType position) const {
    return collectItems(pointOf(op), pointOf(position));
}

std::vector<SpecItem> LinkMatrix::itemsFor(PositionType position, Element element) const {
    return collectItems(pointOf(position), pointOf(element));
}

LinkId LinkMatrix::pointOf(Operator op) const noexcept {
    return operatorPoints_[indexOf(op)];
}

LinkId LinkMatrix::pointOf(PositionType position) const noexcept {
    return positionPoints_[indexOf(position)];
}

LinkId LinkMatrix::pointOf(Element element) const noexcept {
    return elementPoints_[indexOf(element)];
}

const LinksStore& LinkMatrix::links() const noexcept {
    return store_;
}

std::size_t LinkMatrix::itemCount() const noexcept {
    return items_.size();
}

void LinkMatrix::tag(LinkId a, LinkId b, LinkId marker) {
    store_.getOrCreate(store_.getOrCreate(a, b), marker);
}

bool LinkMatrix::hasTag(LinkId a, LinkId b, LinkId marker) const noexcept {
    const auto pair = store_.search(a, b);
    return pair && store_.search(*pair, marker).has_value();
}

void LinkMatrix::attachItem(LinkId a, LinkId b, SpecItem item) {
    const LinkId pair = store_.getOrCreate(a, b);
    const LinkId itemPoint = store_.createPoint();
    items_.push_back(std::move(item));
    itemByPoint_.emplace(itemPoint, items_.size() - 1);
    store_.getOrCreate(pair, itemPoint);
}

std::vector<SpecItem> LinkMatrix::collectItems(LinkId a, LinkId b) const {
    std::vector<SpecItem> result;
    const auto pair = store_.search(a, b);
    if (!pair) {
        return result;
    }
    for (const LinkId linkId : store_.outgoing(*pair)) {
        const auto link = store_.get(linkId);
        if (!link) {
            continue;
        }
        const auto it = itemByPoint_.find(link->target);
        if (it != itemByPoint_.end()) {
            result.push_back(items_[it->second]);
        }
    }
    return result;
}

}  // namespace bsc
