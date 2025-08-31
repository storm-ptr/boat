// Andrew Naplavkov

#ifndef BOAT_LINKED_HASH_MAP_HPP
#define BOAT_LINKED_HASH_MAP_HPP

#include <list>
#include <unordered_map>

namespace boat {

template <class Key,
          class T,
          class Hash = std::hash<Key>,
          class Equal = std::equal_to<Key>>
class linked_hash_map {
public:
    using value_type = std::pair<Key const, T>;
    using container_type = std::list<value_type>;
    using iterator = container_type::iterator;

    linked_hash_map() = default;
    linked_hash_map(linked_hash_map const&) = delete;
    linked_hash_map& operator=(linked_hash_map const&) = delete;
    size_t size() const { return data_.size(); }
    iterator begin() { return data_.begin(); }
    iterator end() { return data_.end(); }
    void transfer(iterator from, iterator to) { data_.splice(to, data_, from); }

    void clear()
    {
        data_.clear();
        index_.clear();
    }

    iterator find(Key const& key)
    {
        auto it = index_.find(key);
        return it == index_.end() ? data_.end() : it->second;
    }

    iterator erase(iterator it)
    {
        index_.erase(it->first);
        return data_.erase(it);
    }

    std::pair<iterator, bool> insert(iterator it, value_type val)
    {
        auto ret = find(val.first);
        if (ret != end())
            return {ret, false};
        ret = data_.insert(it, std::move(val));
        try {
            index_[ret->first] = ret;
            return {ret, true};
        }
        catch (...) {
            data_.erase(ret);
            throw;
        }
    }

private:
    container_type data_;
    std::unordered_map<Key, iterator, Hash, Equal> index_;
};

}  // namespace boat

#endif  // BOAT_LINKED_HASH_MAP_HPP
