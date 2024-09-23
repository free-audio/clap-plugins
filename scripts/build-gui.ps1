vcpkg/vcpkg --overlay-triplets=vcpkg-overlay/triplets install --triplet x64-win-cp --host-triplet x64-win-cp '--x-buildtrees-root=C:\B' --allow-unsupported --recurse --clean-after-build 'qtbase[core,png,brotli,zstd,doubleconversion,gui,concurrent,appstore-compliant,network,freetype,testlib,freetype]' qtdeclarative
cmake --preset $cmake_preset $cmake_options -DCLAP_PLUGIN_GUI_MODEL=$gui_model
cmake --build --preset $cmake_preset --config Release --target clap-plugins
