#! /bin/bash -e

if [[ $(uname) = Linux ]] ; then
  cmake_preset="ninja-headless"
  gui_model=threaded
elif [[ $(uname) = Darwin ]] ; then
  cmake_preset="ninja-headless"
  gui_model=local
else
  cmake_preset="vs-headless"
  gui_model=local
fi

cmake --preset $cmake_preset $cmake_options
cmake --build --preset $cmake_preset --config Release --target clap-plugins
