CXXFLAGS=-std:c++latest -EHsc -bigobj -O2 -W3 -WX -D_UNICODE -DwxDEBUG_LEVEL=0 -MD $(CXXFLAGS)
EXECUTABLE=main.exe
INCLUDE=..\..\..\include\;$(WXWIN)\include;$(WXWIN)\include\msvc;$(INCLUDE)
LIB=$(WXWIN)\lib\vc_x64_lib;$(LIB)
LIBS=libcurl_imp.lib libmysql.lib libpq.lib odbc32.lib spatialite_i.lib sqlite3_i.lib
OBJECTS=$(SOURCES:.cpp=.obj)
SOURCES=*.cpp

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) /Fe:$(EXECUTABLE) $(OBJECTS) $(LIBS)

.cpp.obj:
	$(CXX) -c $(CXXFLAGS) $<

test: all
	$(EXECUTABLE) --log_level=unit_scope

rollback:
	del *.obj *.manifest $(EXECUTABLE) *.png
