CXX ?= g++-15
CXXFLAGS += -std=gnu++2c -O2 -w -DBOAT_TEST_SQLITE_ONLY -I../include $(shell pkg-config --cflags $(PACKAGES))
LDLIBS += $(shell pkg-config --libs $(PACKAGES))

EXECUTABLE = run
PACKAGES = gdal libcurl libjpeg libpng libpq mysqlclient odbc spatialite sqlite3 tbb
SOURCES = main.cpp blob.cpp cache.cpp db.cpp geometry.cpp sql.cpp unicode.cpp uri.cpp
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all test test-autonomous test-sqlite reset

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

test: test-autonomous test-sqlite

test-autonomous: $(EXECUTABLE)
	./$(EXECUTABLE) --log_level=unit_scope --run_test=blob_*,cache,db,geometry_*,unicode,uri

test-sqlite: $(EXECUTABLE)
	./$(EXECUTABLE) --log_level=unit_scope --run_test=sql_*

reset:
	rm -f $(OBJECTS) $(EXECUTABLE) drop.*
