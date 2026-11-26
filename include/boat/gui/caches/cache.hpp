// Andrew Naplavkov

#ifndef BOAT_GUI_CACHES_CACHE_HPP
#define BOAT_GUI_CACHES_CACHE_HPP

#include <atomic>
#include <boat/detail/any_hashable.hpp>
#include <functional>
#include <stop_token>

namespace boat::gui::caches {

/// Counts get() calls and misses.
struct cache_stats {
    size_t requests = 0;
    size_t misses = 0;
};

struct cache {
    virtual ~cache() = default;

    /// Returns empty on a miss.
    virtual std::any get(any_hashable const&) = 0;

    virtual void put(any_hashable, std::any) = 0;

    virtual cache_stats stats() const = 0;
};

template <class F, class... Args>
auto get_or_invoke(
    cache* ptr,
    any_hashable const& key,
    std::stop_token tok,
    F&& f,
    Args&&... args)
{
    if (auto any = ptr ? ptr->get(key) : std::any{}; any.has_value())
        return std::any_cast<std::decay_t<std::invoke_result_t<F, Args...>>>(
            std::move(any));
    auto ret = std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    if (ptr && !tok.stop_requested())
        ptr->put(key, ret);
    return ret;
}

/// Process-wide sequence.
inline size_t next_key()
{
    static constinit std::atomic_size_t seq_;
    return ++seq_;
}

}  // namespace boat::gui::caches

#endif  // BOAT_GUI_CACHES_CACHE_HPP
