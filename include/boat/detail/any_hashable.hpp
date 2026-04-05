// Andrew Naplavkov

#ifndef BOAT_ANY_HASHABLE_HPP
#define BOAT_ANY_HASHABLE_HPP

#include <any>
#include <boost/functional/hash.hpp>

namespace boat {

class any_hashable {
    std::any any_;
    size_t (*hash_)(std::any const&);
    bool (*equal_)(std::any const&, std::any const&);

public:
    any_hashable() = delete;

    template <class T>
    any_hashable(T const& val)
        : any_{val}
        , hash_{[](std::any const& that) {
            if constexpr (requires { std::hash<T>{}; })
                return std::hash<T>{}(std::any_cast<T const&>(that));
            else
                return boost::hash_value(std::any_cast<T const&>(that));
        }}
        , equal_{[](std::any const& lhs, std::any const& rhs) {
            return std::any_cast<T const&>(lhs) == std::any_cast<T const&>(rhs);
        }}
    {
    }

    friend bool operator==(any_hashable const& lhs, any_hashable const& rhs)
    {
        return lhs.any_.type() == rhs.any_.type() &&
               lhs.equal_(lhs.any_, rhs.any_);
    }

    friend size_t hash_value(any_hashable const& that)
    {
        auto ret = size_t{};
        boost::hash_combine(ret, that.any_.type().hash_code());
        boost::hash_combine(ret, that.hash_(that.any_));
        return ret;
    }
};

}  // namespace boat

template <>
struct std::hash<boat::any_hashable> {
    static size_t operator()(boat::any_hashable const& that)
    {
        return hash_value(that);
    }
};

#endif  // BOAT_ANY_HASHABLE_HPP
