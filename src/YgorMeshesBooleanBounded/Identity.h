#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <memory>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {
template<class Tag>
class strong_id {
  public:
    explicit constexpr strong_id(std::uint64_t ordinal) noexcept : ordinal_(ordinal) {}
    constexpr std::uint64_t ordinal() const noexcept { return ordinal_; }
    friend constexpr bool operator==(strong_id a, strong_id b) noexcept { return a.ordinal_ == b.ordinal_; }
    friend constexpr bool operator!=(strong_id a, strong_id b) noexcept { return !(a == b); }
    friend constexpr bool operator<(strong_id a, strong_id b) noexcept { return a.ordinal_ < b.ordinal_; }
  private:
    std::uint64_t ordinal_;
};
template<class Kind> struct source_position { std::uint64_t ordinal; };
template<class Tag> struct task_local_id { std::uint64_t ordinal; };

struct context_owner_token {
    std::shared_ptr<const std::uint8_t> anchor;
    static context_owner_token create() { return {std::make_shared<const std::uint8_t>(0)}; }
    bool same_owner(const context_owner_token &other) const noexcept { return anchor == other.anchor; }
};

template<class Tag, class Key>
std::optional<std::vector<std::pair<Key, strong_id<Tag>>>> publish_canonical_ids(std::vector<Key> keys) {
    std::sort(keys.begin(), keys.end());
    if (std::adjacent_find(keys.begin(), keys.end()) != keys.end()) return std::nullopt;
    std::vector<std::pair<Key, strong_id<Tag>>> result;
    result.reserve(keys.size());
    for (std::uint64_t i = 0; i < keys.size(); ++i) result.emplace_back(std::move(keys[i]), strong_id<Tag>(i));
    return result;
}
}
