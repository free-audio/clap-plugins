#! /bin/bash -e

cpu="$(uname -m)"
case "$cpu" in
x86_64)
  cpu="x64";;
i686)
  cpu="x86";;
esac

if [[ $(uname) = Linux ]] ; then
  QT_FEATURES=",xcb,xcb-xlib,xkb,xkbcommon-x11,xlib,xrender,fontconfig,harfbuzz"
  cmake_preset="ninja-vcpkg"
  gui_model=threaded
  triplet=$cpu-linux
  buildtrees=vcpkg/buildtrees
elif [[ $(uname) = Darwin ]] ; then
  QT_FEATURES=",harfbuzz"
  cmake_preset="ninja-vcpkg"
  gui_model=local
  triplet=$cpu-osx
  buildtrees=vcpkg/buildtrees
else
  QT_FEATURES=""
  cmake_preset="vs-vcpkg"
  gui_model=local
  triplet=$cpu-win
  buildtrees="C:\B"
fi

if [[ "$1" != "" ]]; then
  triplet="$1"
fi

vcpkg_options="--triplet ${triplet}-cp --host-triplet ${triplet}-cp --x-buildtrees-root=$buildtrees"
cmake_options="-DVCPKG_TARGET_TRIPLET=${triplet}-cp -DCMAKE_VCPKG_HOST_TRIPLET=${triplet}-cp"

if [[ ! -x vcpkg/vcpkg ]] ; then
  vcpkg/bootstrap-vcpkg.sh
else
  vcpkg/vcpkg --overlay-triplets=vcpkg-overlay/triplets $vcpkg_option upgrade --no-dry-run --debug
fi

vcpkg/vcpkg --overlay-triplets=vcpkg-overlay/triplets $vcpkg_options install --recurse --debug \
  "qtbase[core,png,doubleconversion,gui,concurrent,appstore-compliant,network,freetype,testlib,freetype${QT_FEATURES}]"

# save space
rm -rf $buildtrees

vcpkg/vcpkg --overlay-triplets=vcpkg-overlay/triplets $vcpkg_options install --recurse --debug \
  qtdeclarative

vcpkg/vcpkg --overlay-triplets=vcpkg-overlay/triplets $vcpkg_options upgrade --debug

# save space
rm -rf $buildtrees

cmake --preset $cmake_preset $cmake_options -DCLAP_PLUGIN_GUI_MODEL=$gui_model
cmake --build --preset $cmake_preset --config Release --target clap-plugins
