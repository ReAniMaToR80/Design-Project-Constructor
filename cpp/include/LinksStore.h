// LinksStore.h — minimal associative store built on the L -> L^2 principle.
//
// Every link is a doublet: it maps its own index L to an ordered pair
// (source, target) of other links, i.e. L -> L x L. A "point" is a link that
// refers to itself (source == target == index) and represents an atomic
// entity (an operator, an element, a specification item, a relation marker).
// Relations are expressed only as links between links, which keeps the
// domain dimensions orthogonal.
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace bsc {

using LinkId = std::uint32_t;

inline constexpr LinkId kNullLink = 0;

struct Link {
    LinkId index = kNullLink;
    LinkId source = kNullLink;
    LinkId target = kNullLink;
};

class LinksStore {
public:
    LinksStore();

    // Creates a new self-referencing point link.
    LinkId createPoint();

    // Returns the existing doublet (source, target) or creates it.
    // Throws std::invalid_argument if source or target does not exist.
    LinkId getOrCreate(LinkId source, LinkId target);

    std::optional<LinkId> search(LinkId source, LinkId target) const noexcept;
    std::optional<Link> get(LinkId index) const noexcept;
    bool exists(LinkId index) const noexcept;
    bool isPoint(LinkId index) const noexcept;

    // All links whose source is the given link (in creation order).
    const std::vector<LinkId>& outgoing(LinkId source) const noexcept;

    // Number of stored links (the null link is not counted).
    std::size_t size() const noexcept;

private:
    static std::uint64_t key(LinkId source, LinkId target) noexcept;

    std::vector<Link> links_;                   // index 0 is the null link
    std::vector<std::vector<LinkId>> bySource_; // outgoing index
    std::unordered_map<std::uint64_t, LinkId> byPair_;
};

}  // namespace bsc
