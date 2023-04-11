cmake_minimum_required(VERSION 2.8.12)

# Set extension name here
set(TARGET_NAME aws)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
project(${TARGET_NAME})
include_directories(src/include)

set(EXTENSION_SOURCES src/aws_extension.cpp)
add_library(${EXTENSION_NAME} STATIC ${EXTENSION_SOURCES})

set(PARAMETERS "-warnings")
build_loadable_extension(${TARGET_NAME} ${PARAMETERS} ${EXTENSION_SOURCES})

include(ExternalProject)

########### CURL ###########
if(WIN32)
    set(CURL_LIBNAME libcurl.lib)
else()
    set(CURL_LIBNAME libcurl.a)
endif()

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

        "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON"
        "-DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/curl_install"
        "-DCMAKE_INSTALL_LIBDIR:STRING=lib"
        "-DOPENSSL_USE_STATIC_LIBS:BOOL=ON"
        "-DOPENSSL_ROOT_DIR:STRING=/opt/homebrew/Cellar/openssl@1.1"
        "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}"
        BUILD_BYPRODUCTS  "${CMAKE_CURRENT_BINARY_DIR}/curl_install/lib/${CURL_LIBNAME}"
        )
add_library(curl_static STATIC IMPORTED)
set_property(TARGET curl_static PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/curl_install/lib/${CURL_LIBNAME}")

########### ZLIB ###########
if(WIN32)
    set(ZLIB_LIBNAME zlib.lib)
else()
    set(ZLIB_LIBNAME libz.a)
endif()

ExternalProject_Add(zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.2.11
        SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/zlib-src"
        BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/zlib-build"
        CMAKE_CACHE_ARGS
        "-DBUILD_SHARED_LIBS:BOOL=OFF"
        "-DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/zlib_install"
        "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}"
        "-DCMAKE_INSTALL_LIBDIR:STRING=lib"
        "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON"
        BUILD_BYPRODUCTS  "${CMAKE_CURRENT_BINARY_DIR}/zlib_install/lib/${ZLIB_LIBNAME}"
        )
add_library(zlib_static STATIC IMPORTED)
set_property(TARGET zlib_static PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/zlib_install/lib/${ZLIB_LIBNAME}")

########### AWS SDK ###########
if (OSX_BUILD_UNIVERSAL)
    set(CMAKE_OSX_ARCHITECTURES_PIPESEPARATOR "x86_64|arm64")
endif()
ExternalProject_Add(awssdk
        GIT_REPOSITORY    https://github.com/aws/aws-sdk-cpp.git
        GIT_TAG           1.11.35
        LIST_SEPARATOR    "|"
        CMAKE_ARGS       -DBUILD_SHARED_LIBS=OFF
        #            -DBUILD_ONLY=core;s3;s3-crt;transfer
        -DBUILD_ONLY=core
        -DENABLE_TESTING=OFF
        -DCMAKE_INSTALL_LIBDIR=lib
        -DSIMPLE_INSTALL=ON
        -DCMAKE_INSTALL_PREFIX=install
        -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES_PIPESEPARATOR}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON

            # Note: there's options in the aws sdk build `BUILD_ZLIB` and `BUILD_CURL`, which would be nice here, but I
            #       couldn't get them to work for the life of me, so instead we use the external projects above instead
        BUILD_ALWAYS      TRUE
        TEST_COMMAND      ""
        BUILD_BYPRODUCTS
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-cpp-sdk-core${CMAKE_STATIC_LIBRARY_SUFFIX}"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-crt-cpp${CMAKE_STATIC_LIBRARY_SUFFIX}"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-s3${CMAKE_STATIC_LIBRARY_SUFFIX}"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-auth${CMAKE_STATIC_LIBRARY_SUFFIX}"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-event-stream${CMAKE_STATIC_LIBRARY_SUFFIX}"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-http${CMAKE_STATIC_LIBRARY_SUFFIX}"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-mqtt${CMAKE_STATIC_LIBRARY_SUFFIX}"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-sdkutils${CMAKE_STATIC_LIBRARY_SUFFIX}"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-io${CMAKE_STATIC_LIBRARY_SUFFIX}"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-checksums${CMAKE_STATIC_LIBRARY_SUFFIX}"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-compression${CMAKE_STATIC_LIBRARY_SUFFIX}"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-cal${CMAKE_STATIC_LIBRARY_SUFFIX}"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-common${CMAKE_STATIC_LIBRARY_SUFFIX}"
            "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}s2n${CMAKE_STATIC_LIBRARY_SUFFIX}"
        )

add_library(awssdk_core STATIC IMPORTED)
add_dependencies(awssdk_core awssdk_project)
set_target_properties(awssdk_core PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-cpp-sdk-core${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_crt STATIC IMPORTED)
add_dependencies(awssdk_crt awssdk_project)
set_target_properties(awssdk_crt PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-crt-cpp${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_s3 STATIC IMPORTED)
add_dependencies(awssdk_c_s3 awssdk_project)
set_target_properties(awssdk_c_s3 PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-s3${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_auth STATIC IMPORTED)
add_dependencies(awssdk_c_auth awssdk_project)
set_target_properties(awssdk_c_auth PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-auth${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_eventstream STATIC IMPORTED)
add_dependencies(awssdk_c_eventstream awssdk_project)
set_target_properties(awssdk_c_eventstream PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-event-stream${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_http STATIC IMPORTED)
add_dependencies(awssdk_c_http awssdk_project)
set_target_properties(awssdk_c_http PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-http${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_mqtt STATIC IMPORTED)
add_dependencies(awssdk_c_mqtt awssdk_project)
set_target_properties(awssdk_c_mqtt PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-mqtt${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_io STATIC IMPORTED)
add_dependencies(awssdk_c_io awssdk_project)
set_target_properties(awssdk_c_io PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-io${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_sdkutils STATIC IMPORTED)
add_dependencies(awssdk_c_sdkutils awssdk_project)
set_target_properties(awssdk_c_sdkutils PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-sdkutils${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_checksums STATIC IMPORTED)
add_dependencies(awssdk_checksums awssdk_project)
set_target_properties(awssdk_checksums PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-checksums${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_compression STATIC IMPORTED)
add_dependencies(awssdk_c_compression awssdk_project)
set_target_properties(awssdk_c_compression PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-compression${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_cal STATIC IMPORTED)
add_dependencies(awssdk_c_cal awssdk_project)
set_target_properties(awssdk_c_cal PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-cal${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_common STATIC IMPORTED)
add_dependencies(awssdk_c_common awssdk_project)
set_target_properties(awssdk_c_common PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-common${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_libs2n STATIC IMPORTED)
add_dependencies(awssdk_libs2n awssdk_project)
set_target_properties(awssdk_libs2n PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}s2n${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_dependencies(${EXTENSION_NAME} awssdk)
######################

SET(OPENSSL_USE_STATIC_LIBS ON)
find_package(OpenSSL REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/awssdk-prefix/src/awssdk-build/install/include)
message(${CMAKE_CURRENT_BINARY_DIR})

if(APPLE)
    # somehow aws-sdk doesn't build libs2n on apple/clang
    # seems not needed on mac with security framework,
    # otherwise can build from https://github.com/aws/s2n-tls
    set(AWS_FRAMEWORK "-framework corefoundation -framework SystemConfiguration -framework security")
    set(AWS_S2N_LIBRARY "")
else()
    set(AWS_S2N_LIBRARY awssdk_libs2n)
endif()

if(WIN32)
    target_link_libraries(${EXTENSION_NAME}
            wininet.lib
            winhttp.lib
            bcrypt.lib
            ncrypt.lib
            userenv.lib
            rpcrt4.lib
            secur32.lib
            advapi32.lib
            shLwApi.lib
            Version.lib
            )
endif()

target_link_libraries(${EXTENSION_NAME}
        awssdk_core
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
        ${AWS_S2N_LIBRARY}
        curl_static
        zlib_static
        OpenSSL::SSL
        OpenSSL::Crypto
        ${AWS_FRAMEWORK}
        ${AWS_WIN_LIBS}
        )

install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
