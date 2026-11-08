// Andrew Naplavkov

#ifndef BOAT_COMMANDS_HPP
#define BOAT_COMMANDS_HPP

#include <boat/db/command.hpp>
#if __has_include(<gdal.h>)
#include <boat/gdal/command.hpp>
#include <boat/gdal/dataset.hpp>
#endif
#include <boat/sql/commands.hpp>

namespace boat {

inline std::unique_ptr<db::command> make_command(std::string_view address)
{
    if (sql::supported_url(address))
        return sql::make_command(address);
#if __has_include(<gdal.h>)
    auto ret = std::make_unique<gdal::command>();
    ret->dataset = gdal::open(std::string{address}.data());
    return ret;
#else
    throw std::runtime_error("compiled without gdal");
#endif
}

}  // namespace boat

#endif  // BOAT_COMMANDS_HPP
