// Andrew Naplavkov

#define BOOST_TEST_MODULE boat

#include <boat/blob.hpp>
#include <boat/db/io.hpp>
#include <boat/db/reflection.hpp>
#include <boat/detail/unicode.hpp>
#include <boat/detail/uri.hpp>
#include <boat/gdal/command.hpp>
#include <boat/gdal/gil.hpp>
#include <boat/gdal/raster.hpp>
#include <boat/gdal/vector.hpp>
#include <boat/geometry/raster.hpp>
#include <boat/geometry/slippy.hpp>
#include <boat/geometry/tile.hpp>
#include <boat/geometry/wkb.hpp>
#include <boat/gui/caches/lru.hpp>
#include <boat/gui/datasets/datasets.hpp>
#include <boat/sql/agent.hpp>
#include <boat/sql/commands.hpp>
#include <boost/test/included/unit_test.hpp>
