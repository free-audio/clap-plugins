#! /bin/bash -e

if [[ ! -x vcpkg/vcpkg ]] ; then
  vcpkg/bootstrap-vcpkg.sh
fi

if [[ $(uname) = Linux ]] ; then
  QT_FEATURES=",xcb,xcb-xlib,xkb,xkbcommon-x11,xlib,xrender,fontconfig,harfbuzz"
  cmake_preset="ninja-vcpkg"
  gui_model=threaded
  triplet=$(uname -m)-linux
  buildtrees=vcpkg/buildtrees
elif [[ $(uname) = Darwin ]] ; then
  QT_FEATURES=",harfbuzz"
  cmake_preset="ninja-vcpkg"
  gui_model=local
  triplet=$(uname -m)-osx
  buildtrees=vcpkg/buildtrees
else
  QT_FEATURES=""
  cmake_preset="vs-vcpkg"
  gui_model=local
  triplet=$(uname -m)-windows
  buildtrees="c:\bt"
fi

if [[ "$1" != "" ]]; then
  triplet="$1"
fi

vcpkg_options="--triplet ${triplet}-clap-plugins --host-triplet ${triplet}-clap-plugins --x-buildtrees-root=$buildtrees"
cmake_options="-DVCPKG_TARGET_TRIPLET=${triplet}-clap-plugins -DCMAKE_VCPKG_HOST_TRIPLET=${triplet}-clap-plugins"

vcpkg/vcpkg --overlay-triplets=vcpkg-overlay/triplets $vcpkg_options install --recurse \
  "qtbase[core,png,doubleconversion,gui,concurrent,appstore-compliant,network,freetype,testlib,freetype${QT_FEATURES}]"

# save space
rm -rf $buildtrees

vcpkg/vcpkg --overlay-triplets=vcpkg-overlay/triplets $vcpkg_options install --recurse \
  qtdeclarative

# save space
rm -rf $buildtrees

cmake --preset $cmake_preset $cmake_options -DCLAP_PLUGIN_GUI_MODEL=$gui_model
cmake --build --preset $cmake_preset --config Release --target clap-plugins
