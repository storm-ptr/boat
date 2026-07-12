QT = core gui
CONFIG += c++latest console warn_off
SOURCES = *.cpp

windows:{
QMAKE_CXXFLAGS += -EHsc -bigobj
INCLUDEPATH += ../../../include
LIBS += \
  -L$$(LIB)\
  -lgdal_i\
  -llibcurl_imp\
  -llibmysql\
  -llibpng16\
  -llibpq\
  -lodbc32\
  -ljpeg\
  -lspatialite_i\
  -lsqlite3_i
TARGET = run
reset.commands = del /q drop.* & rmdir /s /q debug release
reset.depends = distclean
test.commands = $$system_path(release/$$TARGET) --log_level=unit_scope
test.depends = release
QMAKE_EXTRA_TARGETS += reset test
}
