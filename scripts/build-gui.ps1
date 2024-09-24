vcpkg/bootstrap-vcpkg.bat

# '--x-buildtrees-root=C:\B'
vcpkg/vcpkg.exe --overlay-triplets=vcpkg-overlay/triplets install --triplet x64-win-cp --host-triplet x64-win-cp --allow-unsupported --recurse --clean-after-build 'qtbase[core,png,brotli,zstd,doubleconversion,gui,concurrent,appstore-compliant,network,freetype,testlib,freetype]' qtdeclarative
cmake --preset vs-vcpkg -DVCPKG_TARGET_TRIPLET=x64-win-cp -DVCPKG_HOST_TRIPLET=x64-win-cp -DCLAP_PLUGIN_GUI_MODEL=local
cmake --build --preset vs-vcpkg --config Release --target clap-plugins
