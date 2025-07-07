#!/bin/bash

set -e
set -x

# Validates and sets VERSION
. script/parse-validate-version.sh
PKG="$(jq -r .name package.json)"

DIST="${1:?Usage: $0 <$PKG-x.x.x.tgz>}"

if [ "$(basename $DIST)" != "$PKG-$VERSION.tgz" ]; then
	echo "Unexpected tarball '$DIST' given to $0"
	exit 1
fi

gh release upload "v$VERSION" "$DIST"
