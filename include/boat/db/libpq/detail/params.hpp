// Andrew Naplavkov

#ifndef BOAT_DB_LIBPQ_PARAMS_HPP
#define BOAT_DB_LIBPQ_PARAMS_HPP

#include <boat/db/libpq/detail/utility.hpp>
#include <boat/pfr/variant.hpp>

namespace boat::db::libpq::params {

struct param {
    virtual ~param() = default;
    virtual Oid type() = 0;
    virtual char const* value() = 0;
    virtual int length() = 0;
    virtual int format() = 0;
};

template <arithmetic T, Oid oid>
    requires(!mixed(std::endian::native))
class scalar : public param {
    T val_;

public:
    explicit scalar(T val) : val_(val)
    {
        if constexpr (std::endian::native != std::endian::big)
            val_ = byteswap(val_);
    }

    Oid type() override { return oid; }
    char const* value() override { return as_chars(&val_); }
    int length() override { return sizeof val_; }
    int format() override { return binary_fmt; }
};

template <class T, Oid oid, int fmt>
class array : public param {
    T view_;

public:
    explicit array(T view) : view_(view) {}
    Oid type() override { return oid; }
    char const* value() override { return as_chars(view_.data()); }
    int length() override { return int(view_.size()); }
    int format() override { return fmt; }
};

inline std::unique_ptr<param> make(pfr::variant const& var)
{
    using integer = scalar<int64_t, int8_oid>;
    using real = scalar<double, float8_oid>;
    using text = array<std::string_view, text_oid, text_fmt>;
    using binary = array<blob_view, bytea_oid, binary_fmt>;
    auto ret = std::unique_ptr<param>{};
    auto vis = overloaded{
        [](pfr::null) { throw std::runtime_error{"null param"}; },
        [&](int64_t v) { ret = std::make_unique<integer>(v); },
        [&](double v) { ret = std::make_unique<real>(v); },
        [&](std::string_view v) { ret = std::make_unique<text>(v); },
        [&](blob_view v) { ret = std::make_unique<binary>(v); },
    };
    std::visit(vis, var);
    return ret;
}

}  // namespace boat::db::libpq::params

#endif  // BOAT_DB_LIBPQ_PARAMS_HPP
