#!/bin/bash

command -v realpath >/dev/null 2>&1 || realpath() {
    [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
}

command -v nproc >/dev/null 2>&1 || function nproc {
    command -v sysctl >/dev/null 2>&1 &&  \
        sysctl -n hw.logicalcpu ||
        echo "3"
}

SRCS=$(dirname $(realpath $0))/..

mkdir -p build/AsiCameraBoost/logs/archive

mv build/AsiCameraBoost/logs/*.txt build/AsiCameraBoost/logs/archive/

VERSIONS=( 1.16 1.17 )
for ASIVERSION in "${VERSIONS[@]}"
do
    BUILDDIR=./build/AsiCameraBoost/build-SDK-${ASIVERSION}
    LOGDIR=./build/AsiCameraBoost/logs/

    echo "Build boost library with SDK ${ASIVERSION}"
    mkdir -p ${BUILDDIR}
    pushd ${BUILDDIR}
    cmake -DASIVERSION=${ASIVERSION} -DCMAKE_BUILD_TYPE=Debug . $SRCS
    make -j$(($(nproc)+1))
    popd

    echo "Test SDK ${ASIVERSION} with boost library"
    ${BUILDDIR}/example/simply/AsiCameraSimply |& tee ${LOGDIR}/SDK_${ASIVERSION}_BOOST_$(date +%s).txt && echo "Success" || echo "Fail"

    echo "Test SDK ${ASIVERSION}"
    LD_PRELOAD=./AsiCamera/libasicamera2/${ASIVERSION}/lib/x64/libASICamera2.so \
    ${BUILDDIR}/example/simply/AsiCameraSimply |& tee ${LOGDIR}/SDK_${ASIVERSION}_______$(date +%s).txt && echo "Success" || echo "Fail"
done


LOGDIR_FULL=$(realpath ${LOGDIR})

echo
echo "Logs are in the ${LOGDIR_FULL} directory"
ls -l ${LOGDIR}
