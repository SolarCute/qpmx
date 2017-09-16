#!/bin/bash
set -e

currDir=$(dirname $0)

if [[ "$PLATFORM" == "gcc_64" ]]; then
	mv ./qtmodules-travis/ci/linux/build-all.sh ./qtmodules-travis/ci/linux/build-all-orig.sh
	mv $currDir/prepare.sh ./qtmodules-travis/ci/linux/build-all.sh
fi

if [[ "$PLATFORM" == "clang_64" ]]; then
	echo "CONFIG += app_bundle" >> .qmake.conf
fi
