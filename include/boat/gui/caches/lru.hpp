// Andrew Naplavkov

#ifndef BOAT_GUI_CACHES_LRU_HPP
#define BOAT_GUI_CACHES_LRU_HPP

#include <boat/detail/linked_hash_map.hpp>
#include <boat/gui/caches/cache.hpp>
#include <mutex>

namespace boat::gui::caches {

/// Thread-safe; capacity is the entry limit.
class lru : public cache {
    std::mutex guard_;
    size_t capacity_;
    linked_hash_map<any_hashable, std::any> data_;

public:
    explicit lru(size_t capacity) : capacity_{capacity} {}

    std::any get(any_hashable const& key) override
    {
        auto lock = std::lock_guard{guard_};
        if (auto it = data_.find(key); it != data_.end()) {
            data_.transfer(it, data_.end());
            return it->second;
        }
        return {};
    }

    void put(any_hashable key, std::any val) override
    {
        auto lock = std::lock_guard{guard_};
        auto it = data_.insert(data_.end(), {std::move(key), {}}).first;
        it->second = std::move(val);
        data_.transfer(it, data_.end());
        if (data_.size() > capacity_)
            data_.erase(data_.begin());
    }
};

}  // namespace boat::gui::caches

#endif  // BOAT_GUI_CACHES_LRU_HPP
