#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: $0 EXECUTABLE VERSION OUTPUT" >&2
  exit 2
fi

executable=$(realpath "$1")
version=${2#v}
output=$(realpath --canonicalize-missing "$3")

test -x "$executable"
dpkg --validate-thing version "$version"
mkdir -p "$(dirname "$output")"

work=$(mktemp --directory)
trap 'rm -rf "$work"' EXIT

root="$work/ugis"
mkdir -p "$root/DEBIAN" "$root/usr/bin" "$work/debian"
install -m 0755 "$executable" "$root/usr/bin/ugis"
strip --strip-unneeded "$root/usr/bin/ugis"

cat > "$work/debian/control" <<'EOF'
Source: ugis
Section: science
Priority: optional
Maintainer: Andrew Naplavkov <storm-ptr@users.noreply.github.com>

Package: ugis
Architecture: any
Depends: ${shlibs:Depends}
Description: Universal GIS application
EOF

depends=$(
  cd "$work"
  dpkg-shlibdeps --package=ugis -O "$root/usr/bin/ugis" |
    sed -n 's/^shlibs:Depends=//p'
)

cat > "$root/DEBIAN/control" <<EOF
Package: ugis
Version: $version
Section: science
Priority: optional
Architecture: $(dpkg --print-architecture)
Maintainer: Andrew Naplavkov <storm-ptr@users.noreply.github.com>
Depends: $depends
Homepage: https://github.com/storm-ptr/boat
Description: Universal GIS application
 ugis is a desktop application for viewing and working with geospatial data.
EOF

dpkg-deb --build --root-owner-group "$root" "$output"
