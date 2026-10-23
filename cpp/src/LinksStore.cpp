#include "LinksStore.h"

#include <limits>
#include <stdexcept>

namespace bsc {

LinksStore::LinksStore() : links_(1), bySource_(1) {}

LinkId LinksStore::createPoint() {
    if (links_.size() >= std::numeric_limits<LinkId>::max()) {
        throw std::length_error("LinksStore: link index space exhausted");
    }
    const auto index = static_cast<LinkId>(links_.size());
    links_.push_back(Link{index, index, index});
    bySource_.emplace_back();
    bySource_[index].push_back(index);
    byPair_.emplace(key(index, index), index);
    return index;
}

LinkId LinksStore::getOrCreate(LinkId source, LinkId target) {
    if (!exists(source) || !exists(target)) {
        throw std::invalid_argument("LinksStore: source and target must be existing links");
    }
    if (const auto found = search(source, target)) {
        return *found;
    }
    if (links_.size() >= std::numeric_limits<LinkId>::max()) {
        throw std::length_error("LinksStore: link index space exhausted");
    }
    const auto index = static_cast<LinkId>(links_.size());
    links_.push_back(Link{index, source, target});
    bySource_.emplace_back();
    bySource_[source].push_back(index);
    byPair_.emplace(key(source, target), index);
    return index;
}

std::optional<LinkId> LinksStore::search(LinkId source, LinkId target) const noexcept {
    const auto it = byPair_.find(key(source, target));
    if (it == byPair_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<Link> LinksStore::get(LinkId index) const noexcept {
    if (!exists(index)) {
        return std::nullopt;
    }
    return links_[index];
}

bool LinksStore::exists(LinkId index) const noexcept {
    return index != kNullLink && index < links_.size();
}

bool LinksStore::isPoint(LinkId index) const noexcept {
    return exists(index) && links_[index].source == index && links_[index].target == index;
}

const std::vector<LinkId>& LinksStore::outgoing(LinkId source) const noexcept {
    static const std::vector<LinkId> empty;
    return exists(source) ? bySource_[source] : empty;
}

std::size_t LinksStore::size() const noexcept {
    return links_.size() - 1;
}

std::uint64_t LinksStore::key(LinkId source, LinkId target) noexcept {
    return (static_cast<std::uint64_t>(source) << 32U) | target;
}

}  // namespace bsc
