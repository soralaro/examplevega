project(hisi_test)
cmake_minimum_required(VERSION 3.10)
SET( CMAKE_SYSTEM_NAME         Linux )
SET( CMAKE_BUILD_TYPE          "Release" )

set( CMAKE_C_COMPILER          aarch64-himix100-linux-gcc )
set( CMAKE_CXX_COMPILER        aarch64-himix100-linux-g++ )

SET( CMAKE_SYSTEM_PROCESSOR    "aarch64" )

SET( ARM_C_CXX_FLAGS           "-mcpu=cortex-a73.cortex-a53" )
set( ARM_C_CXX_FLAGS           "${ARM_C_CXX_FLAGS} -DHI3559A")
SET( ARM_C_CXX_FLAGS           "${ARM_C_CXX_FLAGS} -fno-aggressive-loop-optimizations" )

set( ARM_C_CXX_FLAGS           "${ARM_C_CXX_FLAGS} -DHI3559av100 -DHI_XXXX -DISP_V2")
set( ARM_C_CXX_FLAGS           "${ARM_C_CXX_FLAGS} -DHI_ACODEC_TYPE_INNER -DHI_ACODEC_TYPE_HDMI")
set( ARM_C_CXX_FLAGS           "${ARM_C_CXX_FLAGS} -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -DUSER_BIT_64 -DKERNEL_BIT_64 -DENABLE_JPEGEDCF")

SET( CMAKE_CXX_FLAGS           "${ARM_C_CXX_FLAGS} "   CACHE STRING "c++ flags" )
SET( CMAKE_C_FLAGS             "${ARM_C_CXX_FLAGS} "   CACHE STRING "c flags" )

SET( BUILD_PLATFORM            "HI3559" )
set(CMAKE_CXX_FLAGS_DEBUG   "$ENV{CXXFLAGS} -O0 -g -ggdb -Wl,--build-id")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -DNDEBUG")

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Werror -fno-strict-aliasing -Wno-deprecated-declarations -Wno-unused-local-typedefs")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fstack-protector") # -fstack-protector-all
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fno-aggressive-loop-optimizations")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fmessage-length=0 -MMD -MP -MF")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp")

include_directories(../include)
link_directories(../lib)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


FILE(GLOB_RECURSE TEST_SRC_FILES  *.c *.cc *.cpp)

set(LNKLIBS
        "-Wl,-Bdynamic"
        -lpthread
        -lssl
        -lcrypto
        -lcurl
        -lc_sec
        -lslog
        -lprotobuf
        -ldrvdevdrv
        -ldrvhdc_host
        -lgflags
        -lglog
        -lmatrix
        -lmemory
        -lmmpa
        -lprofilerclient
        -ljsoncpp
        -lz
        -lopencv_core
        -lopencv_imgproc
        -lopencv_imgcodecs
        -lexpreval
#        -lexif
        -ljpeg
        -lpudge
        -lVegaHiai
        )

foreach(test_src ${TEST_SRC_FILES})
    GET_FILENAME_COMPONENT(ONE_TEST_TARGET ${test_src} NAME_WE)
    ADD_EXECUTABLE(${ONE_TEST_TARGET} ${test_src})
    TARGET_LINK_LIBRARIES(${ONE_TEST_TARGET} ${LNKLIBS})
    install(TARGETS ${ONE_TEST_TARGET} RUNTIME DESTINATION bin/)
endforeach(test_src)