// Andrew Naplavkov

#define BOOST_TEST_MODULE boat

#include <boat/blob.hpp>
#include <boat/catalogs.hpp>
#include <boat/db/io.hpp>
#include <boat/db/reflection.hpp>
#include <boat/detail/unicode.hpp>
#include <boat/detail/uri.hpp>
#include <boat/gdal/catalog.hpp>
#include <boat/gdal/command.hpp>
#include <boat/gdal/gil.hpp>
#include <boat/geometry/raster.hpp>
#include <boat/geometry/wkb.hpp>
#include <boat/gui/caches/lru.hpp>
#include <boat/gui/provider.hpp>
#include <boat/slippy.hpp>
#include <boat/sql/catalog.hpp>
#include <boat/sql/commands.hpp>
#include <boat/tile.hpp>
#include <boost/test/included/unit_test.hpp>
