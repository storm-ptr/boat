QT = core gui
CONFIG += c++latest console warn_off
CONFIG -= app_bundle
DEFINES += QT_NO_KEYWORDS
SOURCES = *.cpp
INCLUDEPATH += ../../../include
TARGET = run

windows:{
QMAKE_CXXFLAGS += -EHsc -bigobj
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
reset.commands = del /q drop.* & rmdir /s /q debug release
reset.depends = distclean
test.commands = $$system_path(release/$$TARGET) --log_level=unit_scope
test.depends = release
QMAKE_EXTRA_TARGETS += reset test
}

unix:!macx {
CONFIG += link_pkgconfig
PKGCONFIG += \
  gdal \
  libcurl \
  libjpeg \
  libpng \
  libpq \
  mysqlclient \
  odbc \
  spatialite \
  sqlite3 \
  tbb
}
