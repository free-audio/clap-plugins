cmake_minimum_required(VERSION 3.17)
cmake_policy(SET CMP0091 NEW) # use CMAKE_MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0100 NEW) # handle .hh files

enable_testing()

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)

project(CLAP_PLUGINS C CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Gui Qml Quick REQUIRED)
add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x060000)

if(${CMAKE_CXX_COMPILER_ID} MATCHES "AppleClang|Clang|GNU")
  add_compile_options(-fvisibility=hidden -fvisibility-inlines-hidden
    -Werror=return-type -Werror=unused-result -Wno-unused-parameter
    -Wuninitialized -Wvla -Wmultichar -Werror=non-virtual-dtor -Woverloaded-virtual)
endif()

if(${CMAKE_CXX_COMPILER_ID} MATCHES "AppleClang|Clang")
  add_compile_options(-Wmost -Wsuper-class-method-mismatch)
endif()

add_subdirectory(clap EXCLUDE_FROM_ALL)
add_subdirectory(clap-helpers EXCLUDE_FROM_ALL)

include_directories(yas/include)

add_subdirectory(plugins)

###########
## CPACK ##
###########

set(CPACK_PACKAGE_NAME "clap-plugins")
set(CPACK_PACKAGE_VENDOR "CLAP")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CLAP Reference Plugins")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/free-audio/clap-plugins")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "clap-plugins")
set(CPACK_NSIS_MANIFEST_DPI_AWARE TRUE)
set(CPACK_NSIS_INSTALL_DIRECTORY "$COMMONPROGRAMDATA/CLAP")

include(CPack)
