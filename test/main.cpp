// Andrew Naplavkov

#define BOOST_TEST_MODULE boat

#include <boat/blob.hpp>
#include <boat/db/commands.hpp>
#include <boat/detail/unicode.hpp>
#include <boat/detail/uri.hpp>
#include <boat/geometry/raster.hpp>
#include <boat/geometry/slippy.hpp>
#include <boat/geometry/wkb.hpp>
#include <boat/gui/caches/lru.hpp>
#include <boat/gui/datasets/datasets.hpp>
#include <boat/pfr/io.hpp>
#include <boat/sql/api.hpp>
#include <boat/sql/io.hpp>
#include <boat/sql/reflection.hpp>
#include <boost/test/included/unit_test.hpp>
