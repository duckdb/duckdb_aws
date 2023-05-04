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

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${LOCAL_INSTALL_DIR})

########### ZLIB ###########
if(WIN32)
    set(ZLIB_LIBNAME zlib.lib)
else()
    set(ZLIB_LIBNAME libz.a)
endif()

ExternalProject_Add(ZLIB
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.2.11
        SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/zlib-src"
        BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/zlib-build"
        CMAKE_ARGS
        "-DBUILD_SHARED_LIBS:BOOL=OFF"
        "-DCMAKE_INSTALL_PREFIX:PATH=${LOCAL_INSTALL_DIR}"
        "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
        "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}"
        "-DCMAKE_INSTALL_LIBDIR:STRING=lib"
        "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON"
        BUILD_BYPRODUCTS  "${LOCAL_INSTALL_DIR}/lib/${ZLIB_LIBNAME}"
        )
add_library(ZLIB_STATIC STATIC IMPORTED)
set_property(TARGET ZLIB_STATIC PROPERTY IMPORTED_LOCATION "${LOCAL_INSTALL_DIR}/lib/${ZLIB_LIBNAME}")
add_dependencies(${EXTENSION_NAME} ZLIB_STATIC)

########### CURL ###########
if(WIN32)
    set(CURL_LIBNAME libcurl.lib)
else()
    set(CURL_LIBNAME libcurl.a)
endif()

ExternalProject_Add(CURL
        GIT_REPOSITORY "https://github.com/curl/curl"
        GIT_TAG "curl-7_88_1"
        GIT_CONFIG advice.detachedHead=false
        PREFIX "${CMAKE_CURRENT_BINARY_DIR}/curl"
        DEPENDS ZLIB
        CMAKE_ARGS
        "-DBUILD_SHARED_LIBS:BOOL=OFF"
        "-DBUILD_CURL_EXE:BOOL=OFF"
        "-DCURL_USE_OPENSSL:BOOL=ON"

        "-DCMAKE_INSTALL_PREFIX:PATH=${LOCAL_INSTALL_DIR}"
        "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"

        # This crap we don't need?
        "-DCURL_USE_LIBSSH2:BOOL=OFF"
        "-DCURL_DISABLE_LDAP:BOOL=ON"

        "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON"
        "-DCMAKE_INSTALL_LIBDIR:STRING=lib"
        "-DOPENSSL_USE_STATIC_LIBS:BOOL=ON"
        "-DOPENSSL_ROOT_DIR:STRING=${OPENSSL_ROOT_DIR}"
        "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}"
        BUILD_BYPRODUCTS "${LOCAL_INSTALL_DIR}/lib/${CURL_LIBNAME}"
        )
add_library(CURL_STATIC STATIC IMPORTED)
set_property(TARGET CURL_STATIC PROPERTY IMPORTED_LOCATION "${LOCAL_INSTALL_DIR}/lib/${CURL_LIBNAME}")
add_dependencies(${EXTENSION_NAME} CURL_STATIC)

########### AWS SDK ###########
if(MSVC)
    set(IGNORE_WARNING_OPTION /W0)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang$"
        OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(IGNORE_WARNING_OPTION -w)
endif()

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
        -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
        -DCMAKE_CXX_FLAGS=${IGNORE_WARNING_OPTION}

        # Note: there's options in the aws sdk build `BUILD_ZLIB` and `BUILD_CURL`, which would be nice here, but I
        #       couldn't get them to work for the life of me, so instead we use the external projects above instead
        BUILD_ALWAYS      TRUE
        TEST_COMMAND      ""
        BUILD_BYPRODUCTS
        "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-cpp-sdk-core${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-crt-cpp${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-s3${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-auth${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-event-stream${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-http${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-mqtt${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-sdkutils${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-io${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-checksums${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-compression${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-cal${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-common${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}s2n${CMAKE_STATIC_LIBRARY_SUFFIX}"
        )

# The reason for this madness is that i could not get cmake to link to static libs by name + ninja requires this
# because of this issue: https://stackoverflow.com/questions/54866067/cmake-and-ninja-missing-and-no-known-rule-to-make-it
add_library(awssdk_core STATIC IMPORTED)
add_dependencies(awssdk_core awssdk_project)
set_target_properties(awssdk_core PROPERTIES IMPORTED_LOCATION "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-cpp-sdk-core${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_crt STATIC IMPORTED)
add_dependencies(awssdk_crt awssdk_project)
set_target_properties(awssdk_crt PROPERTIES IMPORTED_LOCATION "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-crt-cpp${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_s3 STATIC IMPORTED)
add_dependencies(awssdk_c_s3 awssdk_project)
set_target_properties(awssdk_c_s3 PROPERTIES IMPORTED_LOCATION "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-s3${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_auth STATIC IMPORTED)
add_dependencies(awssdk_c_auth awssdk_project)
set_target_properties(awssdk_c_auth PROPERTIES IMPORTED_LOCATION "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-auth${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_eventstream STATIC IMPORTED)
add_dependencies(awssdk_c_eventstream awssdk_project)
set_target_properties(awssdk_c_eventstream PROPERTIES IMPORTED_LOCATION "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-event-stream${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_http STATIC IMPORTED)
add_dependencies(awssdk_c_http awssdk_project)
set_target_properties(awssdk_c_http PROPERTIES IMPORTED_LOCATION "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-http${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_mqtt STATIC IMPORTED)
add_dependencies(awssdk_c_mqtt awssdk_project)
set_target_properties(awssdk_c_mqtt PROPERTIES IMPORTED_LOCATION "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-mqtt${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_io STATIC IMPORTED)
add_dependencies(awssdk_c_io awssdk_project)
set_target_properties(awssdk_c_io PROPERTIES IMPORTED_LOCATION "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-io${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_sdkutils STATIC IMPORTED)
add_dependencies(awssdk_c_sdkutils awssdk_project)
set_target_properties(awssdk_c_sdkutils PROPERTIES IMPORTED_LOCATION "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-sdkutils${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_checksums STATIC IMPORTED)
add_dependencies(awssdk_checksums awssdk_project)
set_target_properties(awssdk_checksums PROPERTIES IMPORTED_LOCATION "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-checksums${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_compression STATIC IMPORTED)
add_dependencies(awssdk_c_compression awssdk_project)
set_target_properties(awssdk_c_compression PROPERTIES IMPORTED_LOCATION "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-compression${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_cal STATIC IMPORTED)
add_dependencies(awssdk_c_cal awssdk_project)
set_target_properties(awssdk_c_cal PROPERTIES IMPORTED_LOCATION "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-cal${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_c_common STATIC IMPORTED)
add_dependencies(awssdk_c_common awssdk_project)
set_target_properties(awssdk_c_common PROPERTIES IMPORTED_LOCATION "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aws-c-common${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(awssdk_libs2n STATIC IMPORTED)
add_dependencies(awssdk_libs2n awssdk_project)
set_target_properties(awssdk_libs2n PROPERTIES IMPORTED_LOCATION "${LOCAL_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}s2n${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_dependencies(${EXTENSION_NAME} AWSSDK)
######################

SET(OPENSSL_USE_STATIC_LIBS ON)
find_package(OpenSSL REQUIRED)

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

# AWS SDK Libs by name
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
        CURL_STATIC
        ZLIB_STATIC
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
