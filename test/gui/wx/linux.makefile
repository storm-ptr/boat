CXX ?= g++-15
CPPFLAGS += -DBOAT_TEST_PASSWORD=$(TEST_PASSWORD) -DBOAT_TEST_MYSQL_HOST=$(TEST_MYSQL_HOST) -DBOAT_TEST_POSTGRES_HOST=$(TEST_POSTGRES_HOST)
CXXFLAGS += -std=gnu++2c -O2 -w -DwxDEBUG_LEVEL=0 -I../../../include $(shell wx-config --cxxflags) $(shell pkg-config --cflags $(PACKAGES))
LDLIBS += $(shell wx-config --libs core,base) $(shell pkg-config --libs $(PACKAGES))

EXECUTABLE = run
PACKAGES = gdal libcurl libjpeg libpng libpq mysqlclient odbc spatialite sqlite3 tbb
SOURCES = main.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TEST_PASSWORD ?= Password12!
TEST_MYSQL_HOST ?= 127.0.0.1
TEST_POSTGRES_HOST ?= localhost

.PHONY: all test reset

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

test: $(EXECUTABLE)
	xvfb-run --auto-servernum ./$(EXECUTABLE) --log_level=unit_scope

reset:
	rm -f $(OBJECTS) $(EXECUTABLE) drop.*
