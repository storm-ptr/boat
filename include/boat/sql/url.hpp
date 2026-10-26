// Andrew Naplavkov

#ifndef BOAT_SQL_URL_HPP
#define BOAT_SQL_URL_HPP

#include <boat/detail/string.hpp>

namespace boat::sql {

/// Recognizes SQL URL schemes even without the corresponding client library.
inline bool supported_url(std::string_view url)
{
    return any({"mysql://", "odbc://", "postgres://", "sqlite://"},
               prefix(url));
}

}  // namespace boat::sql

#endif  // BOAT_SQL_URL_HPP
