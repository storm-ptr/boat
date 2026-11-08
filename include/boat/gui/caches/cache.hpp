// Andrew Naplavkov

#ifndef BOAT_GUI_CACHES_CACHE_HPP
#define BOAT_GUI_CACHES_CACHE_HPP

#include <atomic>
#include <boat/detail/any_hashable.hpp>

namespace boat::gui::caches {

/// Cumulative counts of get() calls and empty results.
struct cache_stats {
    size_t requests = 0;
    size_t misses = 0;
};

struct cache {
    virtual ~cache() = default;

    /// Returns an empty std::any on a miss.
    virtual std::any get(any_hashable const&) = 0;

    virtual void put(any_hashable, std::any) = 0;

    virtual cache_stats stats() const = 0;
};

namespace detail {

auto get_or_invoke(cache* ptr, any_hashable const& key, auto&& fn)
{
    if (auto any = ptr ? ptr->get(key) : std::any{}; any.has_value())
        return std::any_cast<decltype(fn())>(std::move(any));
    auto ret = fn();
    if (ptr)
        ptr->put(key, ret);
    return ret;
}

}  // namespace detail

/// Process-wide sequence.
inline size_t next_key()
{
    static constinit std::atomic_size_t seq_;
    return ++seq_;
}

}  // namespace boat::gui::caches

#endif  // BOAT_GUI_CACHES_CACHE_HPP
