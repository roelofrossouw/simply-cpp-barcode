cmake_minimum_required(VERSION 3.22)
set(SC_MODULE pdf417)
project(simply-cpp-${SC_MODULE} HOMEPAGE_URL "https://github.com/roelofrossouw/simply-cpp-pdf417" LANGUAGES CXX)
include(cmake/sc_bootstrap.cmake)

option(SC_PDF417_BUILD_TESTS "Build simply-cpp-pdf417 tests" ${PROJECT_IS_TOP_LEVEL})

get_sc_version()
message(STATUS "SimplyCPP ${SC_MODULE} Version: ${SC_VERSION}")

# sc-core's rect and size (and sc::params for the example); zxing-cpp (Apache-2.0) decodes, OpenCV
# enlarges and thresholds; both from the system and kept out of the header.
find_or_install_package(sc-core simply-cpp-dev simply-cpp 2.0.0)
find_or_install_package(ZXing libzxing-dev zxing-cpp)
find_or_install_package(OpenCV libopencv-dev opencv COMPONENTS core imgproc imgcodecs) # imgcodecs: the example only

set(SOURCE_OBJECTS)
set(SOURCE_LIBRARIES)
set(SOURCE_LINK_LIBRARIES)

add_sc_object(sc_pdf417
        LINK_LIBRARIES sc::sc-core ZXing::ZXing ${OpenCV_LIBS}
        PUBLIC_LINK_LIBRARIES sc::sc-core ZXing::ZXing ${OpenCV_LIBS})
target_include_directories(sc-obj-sc_pdf417 PRIVATE ${OpenCV_INCLUDE_DIRS})

add_sc_libraries(DESCRIPTION "simply-cpp PDF417: barcodes found in a picture and decoded, poor scans too (zxing-cpp, OpenCV)")
target_compile_features(sc-pdf417 PUBLIC cxx_std_20)
target_compile_features(sc-pdf417-shared PUBLIC cxx_std_20)

install_sc_module(PATH_VARS CMAKE_INSTALL_INCLUDEDIR)
install(FILES cmake/SimplyCppFunctions.cmake cmake/sc_bootstrap.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sc-pdf417
        COMPONENT development)

if (SC_PDF417_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests sc-pdf417-test)
endif ()

add_sc_example(sc-pdf417-demo LINK_LIBRARIES sc::sc-core sc::sc-pdf417-shared ${OpenCV_LIBS})

# OpenCV's runtime package is named per release (406t64 on noble, 410 on resolute): the dev
# packages bring the right ones.
package_sc_module(DEV_DEPENDS "simply-cpp-dev (>= 2.0.0~)" libzxing-dev libopencv-dev)
