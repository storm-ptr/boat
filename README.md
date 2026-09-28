<img width="1214" height="789" alt="ugis2" src="https://github.com/user-attachments/assets/19899d42-95cb-49f2-8133-27a001a7eb9d" />

[![Ubuntu](https://github.com/storm-ptr/boat/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/storm-ptr/boat/actions/workflows/ubuntu.yml)
[![Windows](https://github.com/storm-ptr/boat/actions/workflows/windows.yml/badge.svg)](https://github.com/storm-ptr/boat/actions/workflows/windows.yml)
[![Latest GitHub Release](https://img.shields.io/github/release/storm-ptr/boat.svg)](https://github.com/storm-ptr/boat/releases/latest)

# boat

A cross-platform, header-only C++23 library for working with geospatial data.

## Modules

- **geometry** — Boost.Geometry wrapper and extensions: WKB conversion, raster algorithms
- **db** — type-safe database access (commands, rowsets, variants, reflection)
- **sql** — SQL dialect abstraction (MySQL, ODBC, PostgreSQL/PostGIS, SQLite/SpatiaLite)
- **gdal** — GDAL/OGR wrappers for raster and vector I/O
- **gui** — Qt and wxWidgets providers for map rendering and tile caching

## Documentation

[API reference](https://storm-ptr.github.io/boat/). Generate it locally with
`doxygen Doxyfile`.

## ugis

A micro GIS application built on top of the boat library —
browse, inspect and copy geospatial data from a variety of sources.

### Run

<details>
<summary>Ubuntu</summary>

```sh
sudo dpkg -i ugis.ubuntu-26.04.deb
sudo apt-get install -f
ugis
```

</details>

<details>
<summary>Windows</summary>

Extract the archive and run:

```sh
ugis.exe
```

</details>

## Build from source

### Prerequisites

<details>
<summary>Ubuntu</summary>

```sh
sudo apt-get install --yes --no-install-recommends \
  gcc-15 \
  g++-15 \
  git \
  make \
  pkg-config \
  qmake6 \
  qt6-base-dev \
  libboost-dev \
  libcurl4-openssl-dev \
  libgdal-dev \
  libjpeg-dev \
  default-libmysqlclient-dev \
  libpng-dev \
  libpq-dev \
  libspatialite-dev \
  libsqlite3-dev \
  libtbb-dev \
  odbc-postgresql \
  unixodbc-dev
```

</details>

<details>
<summary>Windows</summary>

Install the following:

- [Visual Studio 2026](https://visualstudio.microsoft.com/) (MSVC, NMake)
- [Git](https://git-scm.com/downloads/win)
- [Qt 6](https://www.qt.io/download) (MSVC 64-bit)
- [Boost](https://www.boost.org/users/download/) (headers only)
- [OSGeo4W](https://trac.osgeo.org/osgeo4w/) —
  GDAL, libcurl, libjpeg, libmysql, libpng, libpq, libspatialite, sqlite3, zlib
  (development packages)

Run the following commands from the **x64 Native Tools Command Prompt for
Visual Studio 2026**:

```cmd
set INCLUDE=C:\boost_1_91_0;C:\OSGeo4W\include;%INCLUDE%
set LIB=C:\OSGeo4W\lib;%LIB%
set PATH=C:\Qt\6.10.3\msvc2022_64\bin;C:\OSGeo4W\bin;%PATH%
set QT_PLUGIN_PATH=C:\Qt\6.10.3\msvc2022_64\plugins
```

</details>

### Build ugis

<details>
<summary>Ubuntu</summary>

```sh
git clone --depth=1 https://github.com/storm-ptr/boat.git
cd boat/example/ugis
qmake6 ugis.pro QMAKE_CC=gcc-15 QMAKE_CXX=g++-15
make --jobs="$(nproc)"
```

</details>

<details>
<summary>Windows</summary>

```cmd
git clone --depth=1 https://github.com/storm-ptr/boat.git
cd boat\example\ugis
qmake
nmake
```

The executable is placed in `release\ugis.exe`.

</details>
