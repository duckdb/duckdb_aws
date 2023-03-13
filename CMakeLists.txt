cmake_minimum_required(VERSION 2.8.12)

# Set extension name here
set(TARGET_NAME quack)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
project(${TARGET_NAME})
include_directories(src/include)

set(EXTENSION_SOURCES src/quack_extension.cpp)
add_library(${EXTENSION_NAME} STATIC ${EXTENSION_SOURCES})

set(PARAMETERS "-warnings")
build_loadable_extension(${TARGET_NAME} ${PARAMETERS} ${EXTENSION_SOURCES})

include(ExternalProject)

########### CURL ###########
ExternalProject_Add(curl
        GIT_REPOSITORY "https://github.com/curl/curl"
        GIT_TAG "curl-7_88_1"
        GIT_CONFIG advice.detachedHead=false
        PREFIX "${CMAKE_CURRENT_BINARY_DIR}/curl"
        CMAKE_CACHE_ARGS
        "-DBUILD_SHARED_LIBS:BOOL=OFF"
        "-DBUILD_CURL_EXE:BOOL=OFF"
        "-DCURL_USE_OPENSSL:BOOL=ON"

        # This crap we don't need?
        "-DCURL_USE_LIBSSH2:BOOL=OFF"
        "-DCURL_DISABLE_LDAP:BOOL=ON"

        "-DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/curl_install"
        "-DOPENSSL_USE_STATIC_LIBS:BOOL=ON"
        "-DOPENSSL_ROOT_DIR:STRING=/opt/homebrew/Cellar/openssl@1.1"
        BUILD_BYPRODUCTS  "${CMAKE_CURRENT_BINARY_DIR}/curl_install/lib/libcurl.a"
        )
add_library(curl_static STATIC IMPORTED)
set_property(TARGET curl_static PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/curl_install/lib/libcurl.a")

########### ZLIB ###########
ExternalProject_Add(zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.2.11
        SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/zlib-src"
        BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/zlib-build"
        CMAKE_CACHE_ARGS
        "-DBUILD_SHARED_LIBS:BOOL=OFF"
        "-DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/zlib_install"
        BUILD_BYPRODUCTS  "${CMAKE_CURRENT_BINARY_DIR}/zlib_install/lib/libz.a"
        )
add_library(zlib_static STATIC IMPORTED)
set_property(TARGET zlib_static PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/zlib_install/lib/libz.a")

########### AWS SDK ###########
ExternalProject_Add(awssdk
        GIT_REPOSITORY    https://github.com/aws/aws-sdk-cpp.git
        GIT_TAG           1.11.35
        LIST_SEPARATOR    "|"
        CMAKE_ARGS       -DBUILD_SHARED_LIBS=OFF
#            -DBUILD_ONLY=core;s3;s3-crt;transfer
            -DBUILD_ONLY=core
            -DENABLE_TESTING=OFF
            -DSIMPLE_INSTALL=ON
            -DCMAKE_INSTALL_PREFIX=install
            # Note: there's options in the aws sdk build `BUILD_ZLIB` and `BUILD_CURL`, which would be nice here, but I
            #       couldn't get them to work for the life of me, so instead we use the external projects above instead
        BUILD_ALWAYS      TRUE
        TEST_COMMAND      ""
        BUILD_BYPRODUCTS  "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-cpp-sdk-core.a"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-crt-cpp.a"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-s3.a"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-auth.a"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-event-stream.a"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-http.a"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-mqtt.a"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-sdkutils.a"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-io.a"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-checksums.a"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-compression.a"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-cal.a"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-common.a"
        )

add_library(awssdk_core_mine STATIC IMPORTED)
add_dependencies(awssdk_core_mine awssdk_project)
set_target_properties(awssdk_core_mine PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-cpp-sdk-core.a")

add_library(awssdk_crt STATIC IMPORTED)
add_dependencies(awssdk_crt awssdk_project)
set_target_properties(awssdk_crt PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-crt-cpp.a")

add_library(awssdk_c_s3 STATIC IMPORTED)
add_dependencies(awssdk_c_s3 awssdk_project)
set_target_properties(awssdk_c_s3 PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-s3.a")

add_library(awssdk_c_auth STATIC IMPORTED)
add_dependencies(awssdk_c_auth awssdk_project)
set_target_properties(awssdk_c_auth PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-auth.a")

add_library(awssdk_c_eventstream STATIC IMPORTED)
add_dependencies(awssdk_c_eventstream awssdk_project)
set_target_properties(awssdk_c_eventstream PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-event-stream.a")

add_library(awssdk_c_http STATIC IMPORTED)
add_dependencies(awssdk_c_http awssdk_project)
set_target_properties(awssdk_c_http PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-http.a")

add_library(awssdk_c_mqtt STATIC IMPORTED)
add_dependencies(awssdk_c_mqtt awssdk_project)
set_target_properties(awssdk_c_mqtt PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-mqtt.a")

add_library(awssdk_c_io STATIC IMPORTED)
add_dependencies(awssdk_c_io awssdk_project)
set_target_properties(awssdk_c_io PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-io.a")

add_library(awssdk_c_sdkutils STATIC IMPORTED)
add_dependencies(awssdk_c_sdkutils awssdk_project)
set_target_properties(awssdk_c_sdkutils PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-sdkutils.a")

add_library(awssdk_checksums STATIC IMPORTED)
add_dependencies(awssdk_checksums awssdk_project)
set_target_properties(awssdk_checksums PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-checksums.a")

add_library(awssdk_c_compression STATIC IMPORTED)
add_dependencies(awssdk_c_compression awssdk_project)
set_target_properties(awssdk_c_compression PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-compression.a")

add_library(awssdk_c_cal STATIC IMPORTED)
add_dependencies(awssdk_c_cal awssdk_project)
set_target_properties(awssdk_c_cal PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-cal.a")

add_library(awssdk_c_common STATIC IMPORTED)
add_dependencies(awssdk_c_common awssdk_project)
set_target_properties(awssdk_c_common PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/libaws-c-common.a")

add_dependencies(${EXTENSION_NAME} awssdk)
######################

SET(OPENSSL_USE_STATIC_LIBS ON)
find_package(OpenSSL REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/include)
message(${CMAKE_CURRENT_BINARY_DIR})

if(APPLE)
    # apple why u so weird
    # somehow aws-sdk doesn't build libs2n on apple/clang
    # seems not needed on mac with security framework,
    # otherwise can build from https://github.com/aws/s2n-tls
    set(AWS_FRAMEWORK "-framework corefoundation -framework SystemConfiguration -framework security")
    set(AWS_S2N_LIBRARY "")
endif()

message(--------------------------------------- QUACK AWS --------------------------------------- )
message(${OPENSSL_LIBRARIES})

target_link_libraries(${EXTENSION_NAME}
        awssdk_core_mine
        awssdk_crt
        awssdk_c_s3
        awssdk_c_auth
        awssdk_c_eventstream
        awssdk_c_http
        awssdk_c_mqtt
        awssdk_c_sdkutils
        awssdk_c_io
        awssdk_checksums
        awssdk_c_compression
        awssdk_c_cal
        awssdk_c_common
        OpenSSL::SSL
        OpenSSL::Crypto
        curl_static
        zlib_static
        ${AWS_FRAMEWORK}
        )

install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
