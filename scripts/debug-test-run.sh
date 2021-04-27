#!/bin/bash

#rm -rf /tmp/AsiCameraBoost
mkdir -p /tmp/AsiCameraBoost/AsiCamera
pushd /tmp/AsiCameraBoost

git clone --depth=1 https://github.com/pawel-soja/AsiCamera.git
./AsiCamera/scripts/debug-test.sh
popd
