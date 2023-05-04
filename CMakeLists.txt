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

set(LOCAL_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/local)

include(ExternalProject)

########### ZLIB ###########
ExternalProject_Add(ZLIB
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.2.11
        SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/zlib-src"
        BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/zlib-build"
        CMAKE_CACHE_ARGS
        "-DBUILD_SHARED_LIBS:BOOL=OFF"
        "-DCMAKE_INSTALL_PREFIX:PATH=${LOCAL_INSTALL_DIR}"
        "-DCMAKE_PREFIX_PATH=${LOCAL_INSTALL_DIR}"
        "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}"
        "-DCMAKE_INSTALL_LIBDIR:STRING=lib"
        "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON"
        BUILD_BYPRODUCTS  "${LOCAL_INSTALL_DIR}/lib/${ZLIB_LIBNAME}"
        )

########### CURL ###########
ExternalProject_Add(CURL
        GIT_REPOSITORY "https://github.com/curl/curl"
        GIT_TAG "curl-7_88_1"
        GIT_CONFIG advice.detachedHead=false
        PREFIX "${CMAKE_CURRENT_BINARY_DIR}/curl"
        DEPENDS ZLIB
        CMAKE_CACHE_ARGS
        "-DBUILD_SHARED_LIBS:BOOL=OFF"
        "-DBUILD_CURL_EXE:BOOL=OFF"
        "-DCURL_USE_OPENSSL:BOOL=ON"

        "-DCMAKE_INSTALL_PREFIX:PATH=${LOCAL_INSTALL_DIR}"
        "-DCMAKE_PREFIX_PATH=${LOCAL_INSTALL_DIR}"

        # This crap we don't need?
        "-DCURL_USE_LIBSSH2:BOOL=OFF"g
        "-DCURL_DISABLE_LDAP:BOOL=ON"

        "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON"
        "-DCMAKE_INSTALL_LIBDIR:STRING=lib"
        "-DOPENSSL_USE_STATIC_LIBS:BOOL=ON"
        "-DOPENSSL_ROOT_DIR:STRING=${OPENSSL_ROOT_DIR}"
        "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}"
        #        BUILD_BYPRODUCTS  "${LOCAL_INSTALL_DIR}/lib/${CURL_LIBNAME}"
        )

########### AWS SDK ###########
if (OSX_BUILD_UNIVERSAL)
    set(CMAKE_OSX_ARCHITECTURES_PIPESEPARATOR "x86_64|arm64")
endif()
ExternalProject_Add(AWSSDK
        GIT_REPOSITORY    https://github.com/aws/aws-sdk-cpp.git
        GIT_TAG           1.11.35
        DEPENDS           ZLIB CURL
        LIST_SEPARATOR    "|"
        CMAKE_ARGS       -DBUILD_SHARED_LIBS=OFF
        -DBUILD_ONLY=core
        -DENABLE_TESTING=OFF
        -DCMAKE_INSTALL_LIBDIR=lib
        -DSIMPLE_INSTALL=ON
        -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES_PIPESEPARATOR}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_INSTALL_PREFIX=${LOCAL_INSTALL_DIR}
        -DCMAKE_PREFIX_PATH=${LOCAL_INSTALL_DIR}

        # Note: there's options in the aws sdk build `BUILD_ZLIB` and `BUILD_CURL`, which would be nice here, but I
        #       couldn't get them to work for the life of me, so instead we use the external projects above instead
        BUILD_ALWAYS      TRUE
        TEST_COMMAND      ""
        )

add_dependencies(${EXTENSION_NAME} AWSSDK)
######################

SET(OPENSSL_USE_STATIC_LIBS ON)
find_package(OpenSSL REQUIRED)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${LOCAL_INSTALL_DIR})
include_directories(${LOCAL_INSTALL_DIR}/include)

if(APPLE)
    # somehow aws-sdk doesn't build libs2n on apple/clang
    # seems not needed on mac with security framework,
    # otherwise can build from https://github.com/aws/s2n-tls
    set(AWS_FRAMEWORK "-framework corefoundation -framework SystemConfiguration -framework security")
    set(AWS_S2N_LIBRARY "")
elseif(WIN32)
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

find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)

# AWS SDK Libs by name
target_link_libraries(${EXTENSION_NAME}
        ${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-cpp-sdk-core${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-crt-cpp${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-s3${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-auth${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-event-stream${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-http${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-mqtt${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-sdkutils${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-io${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-checksums${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-compression${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-cal${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-common${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${AWS_S2N_LIBRARY}
        CURL::libcurl
        ZLIB::ZLIB
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
