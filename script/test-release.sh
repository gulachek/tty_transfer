#!/bin/bash

set -e
set -x

if [ ! -f package.json ]; then
	echo "Please run from project root!"
	exit 1
fi

. script/util.sh

PKG="$(jq -r .name package.json)"
VERSION="$(jq -r .version package.json)"

DIST="$(realpath ${1:?Usage: $0 <$PKG-x.x.x.tgz>})"
if [ "$(basename $DIST)" != "$PKG-$VERSION.tgz" ]; then
	echo "Unexpected tarball '$DIST' given to $0"
	exit 1
fi

if [ ! -f "$DIST" ]; then
	echo "$DIST doesn't exist"
	exit 1
fi

PKGROOT="$VENDORSRC/$PKG"
md "$PKGROOT"

untar -d "$PKGROOT" -f "$DIST"

cmake -S "$PKGROOT" -B "$PKGROOT/build"
cmake --build "$PKGROOT/build"
cmake --install "$PKGROOT/build" --prefix "$VENDOR"

TEST="$PWD/test/release"

echo "Testing pkgconfig"
rm -rf "$TEST/build"
node "$TEST/make.mjs" --srcdir "$TEST" --outdir "$TEST/build" test

echo "Testing CMake"
rm -rf "$TEST/build"
cmake -DCMAKE_PREFIX_PATH="$VENDOR" -S "$TEST" -B "$TEST/build"
cmake --build "$TEST/build"
"$TEST/build/test"
