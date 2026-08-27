CXXFLAGS=-std:c++latest -EHsc -bigobj -O2 -W3 -WX -Wv:18 -D_UNICODE -DwxDEBUG_LEVEL=0 -DWXUSINGDLL -MD $(CXXFLAGS)
EXECUTABLE=run.exe
INCLUDE=..\..\..\include\;$(WXWIN)\include;$(WXWIN)\lib\vc14x_x64_dll\mswu;$(INCLUDE)
LIB=$(WXWIN)\lib\vc14x_x64_dll;$(LIB)
LIBS=wxmsw33u_core.lib wxbase33u.lib gdal_i.lib jpeg.lib libcurl_imp.lib libmysql.lib libpng16.lib libpq.lib odbc32.lib spatialite_i.lib sqlite3_i.lib
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
	del *.obj *.manifest $(EXECUTABLE) drop.*
