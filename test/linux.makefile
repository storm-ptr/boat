CXX ?= g++-15
CXXFLAGS += -std=gnu++2c -O2 -w -I../include $(shell pkg-config --cflags $(PACKAGES))
LDLIBS += $(shell pkg-config --libs $(PACKAGES))

EXECUTABLE = run
PACKAGES = gdal libcurl libjpeg libpng libpq mysqlclient odbc spatialite sqlite3 tbb
SOURCES = main.cpp blob.cpp cache.cpp db.cpp geometry.cpp unicode.cpp uri.cpp
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all test reset

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

test: $(EXECUTABLE)
	./$(EXECUTABLE) --log_level=unit_scope

reset:
	rm -f $(OBJECTS) $(EXECUTABLE) drop.*
