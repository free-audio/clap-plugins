#! /bin/bash

if [[ ! -x vcpkg/vcpkg ]] ; then
  vcpkg/bootstrap-vcpkg.sh
fi

if [[ "$1" != "" ]]; then
  vcpkg_triplet="--triplet $1"
  cmake_triplet="-DVCPKG_TARGET_TRIPLET=$1"
fi

if [[ $(uname) = Linux ]] ; then
  QT_FEATURES=",xcb,xcb-xlib,xkb,xkbcommon-x11,xlib,xrender,fontconfig,freetype,harfbuzz"
  cmake_preset="ninja-vcpkg"
elif [[ $(uname) = Darwin ]] ; then
  QT_FEATURES=""
  cmake_preset="ninja-vcpkg"
else
  QT_FEATURES=""
  cmake_preset="vs-vcpkg"
fi

vcpkg/vcpkg --overlay-triplets=vcpkg-overlay/triplets $vcpkg_triplet install --recurse \
  "qtbase[core,png,doubleconversion,gui,concurrent,appstore-compliant,network,freetype,testlib${QT_FEATURES}]"

# save space
rm -rf vcpkg/buildtrees

vcpkg/vcpkg --overlay-triplets=vcpkg-overlay/triplets $vcpkg_triplet install --recurse \
  qtdeclarative

# save space
rm -rf vcpkg/buildtrees

cmake --preset $cmake_preset $cmake_triplet
cmake --build --preset $cmake_preset --config Release --target clap-plugins
