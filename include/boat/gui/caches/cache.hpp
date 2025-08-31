// Andrew Naplavkov

#ifndef BOAT_GUI_CACHES_CACHE_HPP
#define BOAT_GUI_CACHES_CACHE_HPP

#include <atomic>
#include <boat/detail/any_hashable.hpp>
#include <functional>
#include <type_traits>

namespace boat::gui::caches {

struct cache {
    virtual ~cache() = default;
    virtual std::any get(any_hashable const&) = 0;
    virtual void put(any_hashable, std::any) = 0;
};

template <class F, class... Args>
auto get_or_invoke(cache* ptr, any_hashable const& key, F&& f, Args&&... args)
{
    if (auto any = ptr ? ptr->get(key) : std::any{}; any.has_value())
        return std::any_cast<std::decay_t<std::invoke_result_t<F, Args...>>>(
            std::move(any));
    auto ret = std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    if (ptr)
        ptr->put(key, ret);
    return ret;
}

inline size_t next_key()
{
    static constinit std::atomic_size_t seq_;
    return ++seq_;
}

}  // namespace boat::gui::caches

#endif  // BOAT_GUI_CACHES_CACHE_HPP
