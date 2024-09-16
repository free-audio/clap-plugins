#! /bin/bash -xe

cpu="$(uname -m)"
case "$cpu" in
x86_64)
  cpu="x64";;
i686)
  cpu="x86";;
esac

if [[ $(uname) = Linux ]] ; then
  QT_FEATURES=",xcb,xcb-xlib,xkb,xkbcommon-x11,xlib,xrender,fontconfig,harfbuzz,vulkan"
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

vcpkg_options="--triplet ${triplet}-cp --host-triplet ${triplet}-cp --x-buildtrees-root=$buildtrees --allow-unsupported"
cmake_options="-DVCPKG_TARGET_TRIPLET=${triplet}-cp -DVCPKG_HOST_TRIPLET=${triplet}-cp"

if [[ ! -x vcpkg/vcpkg ]] ; then
  vcpkg/bootstrap-vcpkg.sh
else
  vcpkg/vcpkg --overlay-triplets=vcpkg-overlay/triplets upgrade $vcpkg_options --no-dry-run --debug
fi

vcpkg/vcpkg --overlay-triplets=vcpkg-overlay/triplets install $vcpkg_options --recurse --clean-after-build --debug \
  "qtbase[core,png,brotli,zstd,doubleconversion,gui,concurrent,appstore-compliant,network,freetype,testlib,freetype${QT_FEATURES}]" qtdeclarative

# save space
rm -rf $buildtrees

cmake --preset $cmake_preset $cmake_options -DCLAP_PLUGIN_GUI_MODEL=$gui_model
cmake --build --preset $cmake_preset --config Release --target clap-plugins
