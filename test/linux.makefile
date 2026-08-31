CXX ?= g++-15
CPPFLAGS += -DBOAT_TEST_PASSWORD=$(TEST_PASSWORD) -DBOAT_TEST_POSTGRES_HOST=$(TEST_POSTGRES_HOST)
CXXFLAGS += -std=gnu++2c -O2 -w -I../include $(shell pkg-config --cflags $(PACKAGES))
LDLIBS += $(shell pkg-config --libs $(PACKAGES))

EXECUTABLE = run
PACKAGES = gdal libcurl libjpeg libpng libpq mysqlclient odbc spatialite sqlite3 tbb
SOURCES = main.cpp blob.cpp cache.cpp db.cpp geometry.cpp sql.cpp unicode.cpp uri.cpp
OBJECTS = $(SOURCES:.cpp=.o)
SQL_TESTS = sql_select,sql_vector,sql_datatypes
TEST_PASSWORD ?= Password12!
TEST_POSTGRES_HOST ?= localhost

.PHONY: all test test-autonomous test-sqlite test-postgres reset

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

test: test-autonomous test-sqlite

test-autonomous: $(EXECUTABLE)
	./$(EXECUTABLE) --log_level=unit_scope --run_test=blob_*,cache,db,geometry_*,unicode,uri

test-sqlite: $(EXECUTABLE)
	BOAT_TEST_DB=sqlite ./$(EXECUTABLE) --log_level=unit_scope --run_test=$(SQL_TESTS)

test-postgres: $(EXECUTABLE)
	BOAT_TEST_DB=postgres ./$(EXECUTABLE) --log_level=unit_scope --run_test=$(SQL_TESTS)

reset:
	rm -f $(OBJECTS) $(EXECUTABLE) drop.*
