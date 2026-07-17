QT = concurrent core gui widgets
CONFIG += c++latest warn_off
SOURCES = *.cpp
HEADERS = *.h

windows:{
QMAKE_CXXFLAGS += -EHsc -bigobj -MP
INCLUDEPATH += ../../include
LIBS += \
  -L$$(LIB)\
  -lgdal_i\
  -ljpeg\
  -llibcurl_imp\
  -llibmysql\
  -llibpng16\
  -llibpq\
  -lodbc32\
  -lspatialite_i\
  -lsqlite3_i
reset.commands = rmdir /s /q debug release
reset.depends = distclean
QMAKE_EXTRA_TARGETS += reset
}
