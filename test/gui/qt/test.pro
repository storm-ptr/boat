QT = core gui
CONFIG += c++latest console warn_off
SOURCES = *.cpp

windows:{
QMAKE_CXXFLAGS += -EHsc -bigobj
INCLUDEPATH += ../../../include
LIBS += \
  -L$$(LIB)\
  -llibcurl_imp\
  -llibmysql\
  -llibpq\
  -lodbc32\
  -lspatialite_i\
  -lsqlite3_i
TARGET = run_me.exe
rollback.commands = del /q *.png & rmdir /s /q debug release
rollback.depends = distclean
test.commands = release\run_me.exe --log_level=unit_scope
test.depends = release
QMAKE_EXTRA_TARGETS += rollback test
}
