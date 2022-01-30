#! /bin/bash

if [[ ! -x vcpkg/vcpkg ]] ; then
  vcpkg/bootstrap-vcpkg.sh
fi

if [[ $(uname) = Linux ]] ; then
  QT_FEATURES=",xcb,xcb-xlib,xkb,xkbcommon-x11,xlib,xrender,fontconfig,freetype,harfbuzz"
  cmake_preset="ninja-vcpkg"
  gui_model=threaded
  triplet=$(uname -m)-linux
elif [[ $(uname) = Darwin ]] ; then
  QT_FEATURES=""
  cmake_preset="ninja-vcpkg"
  gui_model=local
  triplet=$(uname -m)-osx
else
  QT_FEATURES=""
  cmake_preset="vs-vcpkg"
  gui_model=local
  triplet=$(uname -m)-windows
fi

if [[ "$1" != "" ]]; then
  triplet="$1"
fi

vcpkg_triplet="--triplet ${triplet}-clap-plugins --host-triplet ${triplet}-clap-plugins"
cmake_triplet="-DVCPKG_TARGET_TRIPLET=${triplet}-clap-plugins -DCMAKE_VCPKG_HOST_TRIPLET=${triplet}-clap-plugins"

vcpkg/vcpkg --overlay-triplets=vcpkg-overlay/triplets $vcpkg_triplet install --recurse \
  "qtbase[core,png,doubleconversion,gui,concurrent,appstore-compliant,network,freetype,testlib${QT_FEATURES}]"

# save space
rm -rf vcpkg/buildtrees

vcpkg/vcpkg --overlay-triplets=vcpkg-overlay/triplets $vcpkg_triplet install --recurse \
  qtdeclarative

# save space
rm -rf vcpkg/buildtrees

cmake --preset $cmake_preset $cmake_triplet -DCLAP_PLUGIN_GUI_MODEL=$gui_model
cmake --build --preset $cmake_preset --config Release --target clap-plugins
