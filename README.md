[![Build status](https://ci.appveyor.com/api/projects/status/github/storm-ptr/boat?svg=true&branch=main)](https://ci.appveyor.com/project/storm-ptr/boat/branch/main)
[![Linux](https://github.com/storm-ptr/boat/actions/workflows/linux.yml/badge.svg)](https://github.com/storm-ptr/boat/actions/workflows/linux.yml)

## Build ugis on Ubuntu 26.04

Install the compiler and development packages:

```sh
sudo apt-get update
sudo apt-get install --yes --no-install-recommends \
  gcc-15 \
  g++-15 \
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
  unixodbc-dev \
  xauth \
  xvfb
```

Configure and build:

```sh
cd example/ugis
qmake6 ugis.pro QMAKE_CC=gcc-15 QMAKE_CXX=g++-15
make --jobs=2
```

Run a ten-second smoke test on a virtual display:

```sh
status=0
timeout --kill-after=5s 10s xvfb-run --auto-servernum ./ugis || status=$?
test "$status" -eq 124
```

Build and run the autonomous tests (no database servers or network access are
required):

```sh
cd test
make --file=linux.makefile --jobs=2 CXX=g++-15
make --file=linux.makefile test CXX=g++-15
```

Run the PostgreSQL tests against a local PostGIS-enabled server:

```sh
make --file=linux.makefile test-postgres CXX=g++-15 \
  TEST_PASSWORD=Password12! TEST_POSTGRES_HOST=localhost
```

Run the MySQL tests against a local server:

```sh
make --file=linux.makefile test-mysql CXX=g++-15 \
  TEST_PASSWORD=Password12! TEST_MYSQL_HOST=127.0.0.1
```

Run the local and PostgreSQL GDAL vector tests:

```sh
make --file=linux.makefile test-gdal-local CXX=g++-15
make --file=linux.makefile test-gdal-postgres CXX=g++-15 \
  TEST_PASSWORD=Password12! TEST_POSTGRES_HOST=localhost
```
