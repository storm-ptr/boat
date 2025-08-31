// Andrew Naplavkov

#ifndef BOAT_TEST_UTILITY_HPP
#define BOAT_TEST_UTILITY_HPP

#include <boat/geometry/detail/utility.hpp>
#include <functional>

namespace boost::geometry {

template <boat::geometry::ogc99 T>
bool operator==(T const& lhs, T const& rhs)
{
    return equals(lhs, rhs);
}

}  // namespace boost::geometry

inline auto const utf8_locale = std::locale{"en_US.utf8"};

template <class F, class Arg>
struct revoke {
    F f_;
    Arg arg_;
    revoke(F f, Arg const& arg) : f_{f}, arg_{std::invoke(f_, arg)} {}
    ~revoke() { std::invoke(f_, arg_); }
};

inline auto make_locale_scope(std::locale const& loc)
{
    return revoke{&std::locale::global, loc};
}

template <boat::ostream T>
auto make_locale_scope(std::locale const& loc, T& os)
{
    return revoke{std::bind_front(&T::imbue, &os), loc};
}

#define BOAT_LIFT(f) \
    []<class... Args>(Args&&... args) { return f(std::forward<Args>(args)...); }

#endif  // BOAT_TEST_UTILITY_HPP
