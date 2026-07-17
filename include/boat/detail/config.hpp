// Andrew Naplavkov

#ifndef BOAT_CONFIG_HPP
#define BOAT_CONFIG_HPP

#include <chrono>

namespace boat {

/// common timeout for network and database operations
constexpr auto timeout = std::chrono::seconds{15};

}  // namespace boat

#endif  // BOAT_CONFIG_HPP
