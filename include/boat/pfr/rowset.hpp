// Andrew Naplavkov

#ifndef BOAT_PFR_ROWSET_HPP
#define BOAT_PFR_ROWSET_HPP

#include <boat/pfr/adapted/adapted.hpp>
#include <boost/pfr.hpp>

namespace boat::pfr {

struct rowset {
    std::vector<std::string> columns;
    std::vector<std::vector<variant>> rows;

    bool empty() const { return rows.empty(); }
    auto begin() const { return rows.begin(); }
    auto end() const { return rows.end(); }
    variant const& value() const { return rows.at(0).at(0); }
};

template <class T>
constexpr auto view = std::views::transform([](range_of<variant> auto&& r) {
    T ret;
    auto it = std::ranges::begin(r);
    if constexpr (requires { read(*it, ret); })
        read(*it++, ret);
    else
        boost::pfr::for_each_field(ret, [&](auto& v) { read(*it++, v); });
    check(it == std::ranges::end(r), "out of fields");
    return ret;
});

rowset to_rowset(std::ranges::input_range auto&& r)
{
    auto ret = rowset{
        boost::pfr::names_as_array<std::ranges::range_value_t<decltype(r)>>() |
        std::ranges::to<decltype(rowset::columns)>()};
    for (auto&& item : r) {
        auto& row = ret.rows.emplace_back();
        auto vis = [&](auto& v) { write(row.emplace_back(), v); };
        boost::pfr::for_each_field(item, vis);
    }
    return ret;
}

}  // namespace boat::pfr

#endif  // BOAT_PFR_ROWSET_HPP
