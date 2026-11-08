// Andrew Naplavkov

#ifndef BOAT_GUI_CACHES_LRU_HPP
#define BOAT_GUI_CACHES_LRU_HPP

#include <boat/gui/caches/cache.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <mutex>
#include <utility>

namespace boat::gui::caches {

/// Thread-safe; capacity is the entry limit.
class lru : public cache {
    using entry = std::pair<any_hashable, std::any>;
    mutable std::mutex guard_;
    cache_stats stats_;
    size_t capacity_;
    boost::multi_index_container<
        entry,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
            boost::multi_index::hashed_unique<
                boost::multi_index::
                    member<entry, any_hashable, &entry::first>>>>
        data_;

public:
    explicit lru(size_t capacity) : capacity_{capacity} {}

    std::any get(any_hashable const& key) override
    {
        auto lock = std::lock_guard{guard_};
        ++stats_.requests;
        auto& index = data_.get<1>();
        if (auto it = index.find(key); it != index.end()) {
            data_.relocate(data_.end(), data_.project<0>(it));
            stats_.misses += !it->second.has_value();
            return it->second;
        }
        ++stats_.misses;
        return {};
    }

    void put(any_hashable key, std::any val) override
    {
        auto lock = std::lock_guard{guard_};
        auto it = data_.push_back({std::move(key), {}}).first;
        data_.replace(it, {it->first, std::move(val)});
        data_.relocate(data_.end(), it);
        if (data_.size() > capacity_)
            data_.pop_front();
    }

    cache_stats stats() const override
    {
        auto lock = std::lock_guard{guard_};
        return stats_;
    }
};

}  // namespace boat::gui::caches

#endif  // BOAT_GUI_CACHES_LRU_HPP
