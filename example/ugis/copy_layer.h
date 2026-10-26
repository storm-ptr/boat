// Andrew Naplavkov

#ifndef COPY_LAYER_H
#define COPY_LAYER_H

#include <stop_token>

struct leaf;

void copy_raster(  //
    leaf const& src,
    char const* dst,
    char const* drv,
    std::stop_token);

leaf copy_vector(  //
    leaf const& src,
    char const* dst,
    char const* drv,
    char const* name,
    std::stop_token);

#endif  // COPY_LAYER_H
