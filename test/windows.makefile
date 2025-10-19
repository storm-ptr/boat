CXXFLAGS=-std:c++latest -EHsc -bigobj -O2 -W3 -WX $(CXXFLAGS)
EXECUTABLE=run_me.exe
INCLUDE=..\include\;$(INCLUDE)
LIBS=libmysql.lib libpq.lib odbc32.lib spatialite_i.lib sqlite3_i.lib
OBJECTS=$(SOURCES:.cpp=.obj)
SOURCES=*.cpp

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) /Fe:$(EXECUTABLE) $(OBJECTS) $(LIBS)

.cpp.obj:
	$(CXX) -c $(CXXFLAGS) $<

test: all
	$(EXECUTABLE) --log_level=unit_scope

reset:
	del *.obj $(EXECUTABLE)
