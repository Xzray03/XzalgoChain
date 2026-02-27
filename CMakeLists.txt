# XzalgoChain - 320-bit Cryptographic Hash Function
# CMakeLists.txt - CMake build configuration for header-only library
# Copyright 2026 Xzrayツ

cmake_minimum_required(VERSION 3.10)
project(XzalgoChain VERSION 0.0.1.1 LANGUAGES C)

include(CheckCCompilerFlag)

# ==================== PROJECT CONFIGURATION ====================

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ==================== OPTIONS ====================

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_STATIC_LIBS "Build static libraries" ON)
option(XZALGOCHAIN_USE_OPENMP "Use OpenMP for parallel processing" ON)
option(XZALGOCHAIN_ENABLE_SIMD "Enable SIMD optimizations" ON)
option(XZALGOCHAIN_FORCE_SCALAR "Force scalar mode (disable SIMD)" OFF)
option(XZALGOCHAIN_INSTALL_HEADERS "Install header files" ON)

# ==================== COMPILER AND PLATFORM DETECTION ====================

# Detect operating system
if(WIN32)
    set(XZALGOCHAIN_PLATFORM "Windows")
    add_definitions(-DWIN32 -D_WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_definitions(-DWIN64)
    endif()
elseif(APPLE)
    set(XZALGOCHAIN_PLATFORM "macOS")
    add_definitions(-DAPPLE -D_DARWIN_C_SOURCE)

    # Check if iOS
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "tvOS" OR CMAKE_SYSTEM_NAME STREQUAL "watchOS")
        set(XZALGOCHAIN_PLATFORM "iOS")
        add_definitions(-DIOS)
    endif()
elseif(UNIX)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        if(ANDROID)
            set(XZALGOCHAIN_PLATFORM "Android")
            add_definitions(-DANDROID)
            if(DEFINED ANDROID_NATIVE_API_LEVEL)
                add_definitions(-D__ANDROID_API__=${ANDROID_NATIVE_API_LEVEL})
            endif()
        else()
            set(XZALGOCHAIN_PLATFORM "Linux")
            add_definitions(-DLINUX -D_XOPEN_SOURCE=700)
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        set(XZALGOCHAIN_PLATFORM "FreeBSD")
        add_definitions(-DFREEBSD -D__BSD_VISIBLE)
    else()
        set(XZALGOCHAIN_PLATFORM "Unix-like")
    endif()
else()
    set(XZALGOCHAIN_PLATFORM "Unknown")
endif()

# Detect architecture
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
    set(XZALGOCHAIN_ARCH "x86_64")
    set(XZALGOCHAIN_IS_X86 TRUE)
    set(XZALGOCHAIN_IS_64BIT TRUE)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i.86|x86|X86)$")
    set(XZALGOCHAIN_ARCH "x86")
    set(XZALGOCHAIN_IS_X86 TRUE)
    set(XZALGOCHAIN_IS_64BIT FALSE)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64|AARCH64)$")
    set(XZALGOCHAIN_ARCH "ARM64")
    set(XZALGOCHAIN_IS_ARM TRUE)
    set(XZALGOCHAIN_IS_64BIT TRUE)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|ARM|armv7|armv7l|armv7a)$")
    set(XZALGOCHAIN_ARCH "ARM")
    set(XZALGOCHAIN_IS_ARM TRUE)
    set(XZALGOCHAIN_IS_64BIT FALSE)
else()
    set(XZALGOCHAIN_ARCH ${CMAKE_SYSTEM_PROCESSOR})
endif()

# ==================== COMPILER FLAGS ====================

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ==================== OPENMP DETECTION ====================

if(XZALGOCHAIN_USE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_C_FOUND)
        set(HAVE_OPENMP TRUE)
        message(STATUS "OpenMP found: ${OpenMP_C_FLAGS}")
    else()
        set(HAVE_OPENMP FALSE)
        message(STATUS "OpenMP not found - parallel processing disabled")
    endif()
else()
    set(HAVE_OPENMP FALSE)
    message(STATUS "OpenMP disabled by user option")
endif()

# Warning flags
if(MSVC)
    add_compile_options(/W3)
else()
    add_compile_options(-Wall -Wextra)
endif()

# Release/Optimization flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3)
    if(NOT MSVC)
        add_compile_options(-flto)
        add_link_options(-flto)
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
    add_definitions(-DDEBUG)
else()
    add_definitions(-DNDEBUG)
endif()

# Position Independent Code for shared library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ==================== SIMD DETECTION AND FLAGS ====================

if(XZALGOCHAIN_ENABLE_SIMD AND NOT XZALGOCHAIN_FORCE_SCALAR)
    # x86/x86_64 SIMD flags
    if(XZALGOCHAIN_IS_X86)
        include(CheckCCompilerFlag)

        # Check for AVX2 support
        check_c_compiler_flag("-mavx2" HAS_AVX2)
        if(HAS_AVX2)
            add_compile_options(-mavx2)
            add_definitions(-DXZALGOCHAIN_HAVE_AVX2)
        endif()

        check_c_compiler_flag("-mfma" HAS_FMA)
        if(HAS_FMA)
            add_compile_options(-mfma)
        endif()

        check_c_compiler_flag("-msse4.2" HAS_SSE42)
        if(HAS_SSE42)
            add_compile_options(-msse4.2)
        endif()

        check_c_compiler_flag("-mpclmul" HAS_PCLMUL)
        if(HAS_PCLMUL)
            add_compile_options(-mpclmul)
        endif()

        check_c_compiler_flag("-maes" HAS_AES)
        if(HAS_AES)
            add_compile_options(-maes)
        endif()

        # Architecture optimizations
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            check_c_compiler_flag("-march=native" HAS_NATIVE)
            if(HAS_NATIVE)
                add_compile_options(-march=native -mtune=native)
            endif()
        endif()

    # ARM SIMD flags
    elseif(XZALGOCHAIN_IS_ARM)
        add_definitions(-D__ARM_NEON -D__ARM_NEON__)
        add_definitions(-DXZALGOCHAIN_HAVE_NEON)

        if(NOT XZALGOCHAIN_IS_64BIT)
            # ARM32 specific flags
            if(ANDROID)
                add_compile_options(-march=armv7-a -mfpu=neon -mfloat-abi=softfp)
            else()
                check_c_compiler_flag("-mfpu=neon" HAS_NEON_FPU)
                if(HAS_NEON_FPU)
                    add_compile_options(-march=armv7-a -mfpu=neon -mfloat-abi=hard)
                endif()
            endif()
        else()
            # ARM64 - NEON is always available
            check_c_compiler_flag("-march=armv8-a+fp+simd" HAS_ARMV8_SIMD)
            if(HAS_ARMV8_SIMD)
                add_compile_options(-march=armv8-a+fp+simd)
            endif()
        endif()
    endif()
endif()

# Force scalar mode if requested
if(XZALGOCHAIN_FORCE_SCALAR)
    add_definitions(-DXZALGOCHAIN_FORCE_SCALAR=1)
endif()

# ==================== HEADER FILES ====================

set(XZALGOCHAIN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/XzalgoChain)

set(XZALGOCHAIN_HEADERS
    ${XZALGOCHAIN_INCLUDE_DIR}/XzalgoChain.h
    ${XZALGOCHAIN_INCLUDE_DIR}/config.h
    ${XZALGOCHAIN_INCLUDE_DIR}/utils.h
    ${XZALGOCHAIN_INCLUDE_DIR}/platform_detect.h
    ${XZALGOCHAIN_INCLUDE_DIR}/simd_detect.h
    ${XZALGOCHAIN_INCLUDE_DIR}/algorithm.h
    ${XZALGOCHAIN_INCLUDE_DIR}/algorithm_scalar.h
    ${XZALGOCHAIN_INCLUDE_DIR}/algorithm_simd.h
)

# ==================== INTERFACE LIBRARY ====================

# Create an INTERFACE library for header-only usage
add_library(XzalgoChain_interface INTERFACE)
target_include_directories(XzalgoChain_interface INTERFACE
    $<BUILD_INTERFACE:${XZALGOCHAIN_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include/XzalgoChain>
)

if(OpenMP_C_FOUND)
    target_link_libraries(XzalgoChain_interface INTERFACE OpenMP::OpenMP_C)
endif()

# ==================== STATIC LIBRARY ====================

if(BUILD_STATIC_LIBS)
    set(XZALGOCHAIN_SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/xzalgochain.c)

    if(NOT EXISTS ${XZALGOCHAIN_SOURCE_FILE})
        message(FATAL_ERROR "xzalgochain.c not found at ${XZALGOCHAIN_SOURCE_FILE}")
    endif()

    add_library(XzalgoChain_static STATIC ${XZALGOCHAIN_SOURCE_FILE})
    target_include_directories(XzalgoChain_static PUBLIC
        $<BUILD_INTERFACE:${XZALGOCHAIN_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include/XzalgoChain>
    )

    target_compile_definitions(XzalgoChain_static PRIVATE
        XZALGOCHAIN_STATIC=1
    )

    # Link with OpenMP if available
    if(OpenMP_C_FOUND)
        target_link_libraries(XzalgoChain_static PUBLIC OpenMP::OpenMP_C)
    endif()

    # Set properties
    set_target_properties(XzalgoChain_static PROPERTIES
        OUTPUT_NAME XzalgoChain
        PUBLIC_HEADER "${XZALGOCHAIN_HEADERS}"
        VERSION ${PROJECT_VERSION}
    )
endif()

# ==================== SHARED LIBRARY ====================

if(BUILD_SHARED_LIBS)
    set(XZALGOCHAIN_SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/xzalgochain.c)

    if(NOT EXISTS ${XZALGOCHAIN_SOURCE_FILE})
        message(FATAL_ERROR "xzalgochain.c not found at ${XZALGOCHAIN_SOURCE_FILE}")
    endif()

    add_library(XzalgoChain_shared SHARED ${XZALGOCHAIN_SOURCE_FILE})
    target_include_directories(XzalgoChain_shared PUBLIC
        $<BUILD_INTERFACE:${XZALGOCHAIN_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include/XzalgoChain>
    )

    target_compile_definitions(XzalgoChain_shared PRIVATE
        XZALGOCHAIN_SHARED=1
        XZALGOCHAIN_EXPORTS=1
    )

    # Link with OpenMP if available
    if(OpenMP_C_FOUND)
        target_link_libraries(XzalgoChain_shared PUBLIC OpenMP::OpenMP_C)
    endif()

    # Set properties
    set_target_properties(XzalgoChain_shared PROPERTIES
        OUTPUT_NAME XzalgoChain
        PUBLIC_HEADER "${XZALGOCHAIN_HEADERS}"
        VERSION ${PROJECT_VERSION}
        SOVERSION 0
    )

    # Set appropriate suffix for different platforms
    if(WIN32)
        set_target_properties(XzalgoChain_shared PROPERTIES
            SUFFIX ".dll"
            PREFIX ""
            IMPORT_PREFIX ""
            IMPORT_SUFFIX ".lib"
        )

        target_compile_options(XzalgoChain_shared PRIVATE /D_XZALGOCHAIN_BUILD_DLL_)
    elseif(APPLE)
        set_target_properties(XzalgoChain_shared PROPERTIES
            SUFFIX ".dylib"
        )
    endif()
endif()

# ==================== ALIAS TARGET ====================

# Create an alias target for easy linking
if(BUILD_SHARED_LIBS)
    add_library(XzalgoChain ALIAS XzalgoChain_shared)
elseif(BUILD_STATIC_LIBS)
    add_library(XzalgoChain ALIAS XzalgoChain_static)
else()
    add_library(XzalgoChain ALIAS XzalgoChain_interface)
endif()

# ==================== UNINSTALL SCRIPT ====================

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake.in
"if(NOT EXISTS \"@CMAKE_CURRENT_BINARY_DIR@/install_manifest.txt\")
    message(FATAL_ERROR \"Cannot find install manifest: @CMAKE_CURRENT_BINARY_DIR@/install_manifest.txt\")
endif()

file(READ \"@CMAKE_CURRENT_BINARY_DIR@/install_manifest.txt\" files)
string(REGEX REPLACE \"\\n\" \";\" files \"\${files}\")

foreach(file \${files})
    message(STATUS \"Uninstalling: \$ENV{DESTDIR}\${file}\")
    if(EXISTS \"\$ENV{DESTDIR}\${file}\")
        execute_process(
            COMMAND \${CMAKE_COMMAND} -E remove \"\$ENV{DESTDIR}\${file}\"
            OUTPUT_VARIABLE rm_out
            RESULT_VARIABLE rm_ret
        )
        if(NOT \${rm_ret} EQUAL 0)
            message(FATAL_ERROR \"Failed to remove: \$ENV{DESTDIR}\${file}\")
        endif()
    else()
        message(STATUS \"File not found: \$ENV{DESTDIR}\${file}\")
    endif()
endforeach()
")

# ==================== CONFIGURATION FILE TEMPLATE ====================

# Create CMake config file template
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/XzalgoChainConfig.cmake.in
"@PACKAGE_INIT@

include(\${CMAKE_CURRENT_LIST_DIR}/XzalgoChainTargets.cmake)

set_and_check(XzalgoChain_INCLUDE_DIR \"@PACKAGE_CMAKE_INSTALL_INCLUDEDIR@/XzalgoChain\")

# Imported targets
if(NOT TARGET XzalgoChain::XzalgoChain)
    if(EXISTS \${CMAKE_CURRENT_LIST_DIR}/XzalgoChainTargets.cmake)
        include(\${CMAKE_CURRENT_LIST_DIR}/XzalgoChainTargets.cmake)
    endif()
endif()

# Helper function to link XzalgoChain
function(xzalgochain_use_library target)
    target_link_libraries(\${target} PRIVATE XzalgoChain::XzalgoChain)
    target_include_directories(\${target} PRIVATE \${XzalgoChain_INCLUDE_DIR})
endfunction()

check_required_components(XzalgoChain)
")

# ==================== INSTALLATION ====================

include(GNUInstallDirs)

if(XZALGOCHAIN_INSTALL_HEADERS)
    # Install headers
    install(FILES ${XZALGOCHAIN_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/XzalgoChain
        COMPONENT XzalgoChain_Development
    )
endif()

# Install libraries
if(BUILD_STATIC_LIBS)
    install(TARGETS XzalgoChain_static
        EXPORT XzalgoChainTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/XzalgoChain
        COMPONENT XzalgoChain_Development
    )
endif()

if(BUILD_SHARED_LIBS)
    install(TARGETS XzalgoChain_shared
        EXPORT XzalgoChainTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/XzalgoChain
        COMPONENT XzalgoChain_Runtime
    )
endif()

# Install CMake package configuration files
install(EXPORT XzalgoChainTargets
    FILE XzalgoChainTargets.cmake
    NAMESPACE XzalgoChain::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/XzalgoChain
)

# Generate and install package config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/XzalgoChainConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_BINARY_DIR}/XzalgoChainConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/XzalgoChainConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/XzalgoChain
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/XzalgoChainConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/XzalgoChainConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/XzalgoChain
)

# ==================== UNINSTALL TARGET ====================

if(NOT TARGET uninstall)
    configure_file(
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake.in"  # Gunakan yang sudah dibuat
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
    )

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
        COMMENT "Uninstalling XzalgoChain..."
    )
endif()

# ==================== INFORMATION TARGET ====================

add_custom_target(info
    COMMAND ${CMAKE_COMMAND} -E echo "========================================"
    COMMAND ${CMAKE_COMMAND} -E echo "XzalgoChain Build Configuration"
    COMMAND ${CMAKE_COMMAND} -E echo "========================================"
    COMMAND ${CMAKE_COMMAND} -E echo "Platform: ${XZALGOCHAIN_PLATFORM}"
    COMMAND ${CMAKE_COMMAND} -E echo "Architecture: ${XZALGOCHAIN_ARCH}"
    COMMAND ${CMAKE_COMMAND} -E echo "Build Type: ${CMAKE_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} -E echo "Compiler: ${CMAKE_C_COMPILER}"
    COMMAND ${CMAKE_COMMAND} -E echo "OpenMP: ${HAVE_OPENMP}"
    COMMAND ${CMAKE_COMMAND} -E echo "AVX2: $<TARGET_PROPERTY:XzalgoChain_interface,COMPILE_DEFINITIONS>"
    COMMAND ${CMAKE_COMMAND} -E echo "Build Shared: ${BUILD_SHARED_LIBS}"
    COMMAND ${CMAKE_COMMAND} -E echo "Build Static: ${BUILD_STATIC_LIBS}"
    COMMAND ${CMAKE_COMMAND} -E echo "Force Scalar: ${XZALGOCHAIN_FORCE_SCALAR}"
    COMMAND ${CMAKE_COMMAND} -E echo "========================================"
    COMMENT "Display build configuration information"
)

# ==================== CUSTOM TARGETS FOR BUILD VARIANTS ====================

# Debug build
add_custom_target(debug
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug -B build_debug ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build build_debug
    COMMENT "Building debug configuration"
)

# Release build
add_custom_target(release
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release -B build_release ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build build_release
    COMMENT "Building release configuration"
)

# Static build
add_custom_target(static_build
    COMMAND ${CMAKE_COMMAND} -DBUILD_SHARED_LIBS=OFF -B build_static ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build build_static
    COMMENT "Building static libraries only"
)

# Shared build
add_custom_target(shared_build
    COMMAND ${CMAKE_COMMAND} -DBUILD_SHARED_LIBS=ON -B build_shared ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build build_shared
    COMMENT "Building shared libraries only"
)

# Scalar mode build
add_custom_target(scalar_build
    COMMAND ${CMAKE_COMMAND} -DXZALGOCHAIN_FORCE_SCALAR=ON -B build_scalar ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build build_scalar
    COMMENT "Building with scalar mode forced"
)

# No SIMD build
add_custom_target(nosimd_build
    COMMAND ${CMAKE_COMMAND} -DXZALGOCHAIN_ENABLE_SIMD=OFF -B build_nosimd ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build build_nosimd
    COMMENT "Building without SIMD optimizations"
)

# ==================== CROSS-COMPILATION HELPERS ====================

# Android ARM64
add_custom_target(cross-android-arm64
    COMMAND ${CMAKE_COMMAND} -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake
        -DANDROID_ABI=arm64-v8a
        -DANDROID_PLATFORM=android-21
        -B build_android_arm64 ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build build_android_arm64
    COMMENT "Cross-compiling for Android ARM64"
)

# Android ARM32
add_custom_target(cross-android-arm
    COMMAND ${CMAKE_COMMAND} -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake
        -DANDROID_ABI=armeabi-v7a
        -DANDROID_PLATFORM=android-21
        -B build_android_arm ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build build_android_arm
    COMMENT "Cross-compiling for Android ARM32"
)

# Android x86
add_custom_target(cross-android-x86
    COMMAND ${CMAKE_COMMAND} -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake
        -DANDROID_ABI=x86
        -DANDROID_PLATFORM=android-21
        -B build_android_x86 ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build build_android_x86
    COMMENT "Cross-compiling for Android x86"
)

# Android x86_64
add_custom_target(cross-android-x86_64
    COMMAND ${CMAKE_COMMAND} -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake
        -DANDROID_ABI=x86_64
        -DANDROID_PLATFORM=android-21
        -B build_android_x86_64 ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build build_android_x86_64
    COMMENT "Cross-compiling for Android x86_64"
)

# Windows 64-bit (MinGW)
add_custom_target(cross-windows
    COMMAND ${CMAKE_COMMAND} -DCMAKE_SYSTEM_NAME=Windows
        -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc
        -B build_windows64 ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build build_windows64
    COMMENT "Cross-compiling for Windows 64-bit"
)

# Windows 32-bit (MinGW)
add_custom_target(cross-windows-32
    COMMAND ${CMAKE_COMMAND} -DCMAKE_SYSTEM_NAME=Windows
        -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc
        -B build_windows32 ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build build_windows32
    COMMENT "Cross-compiling for Windows 32-bit"
)

# iOS ARM64
add_custom_target(cross-ios-arm64
    COMMAND ${CMAKE_COMMAND} -DCMAKE_SYSTEM_NAME=iOS
        -DCMAKE_OSX_ARCHITECTURES=arm64
        -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
        -B build_ios_arm64 ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build build_ios_arm64
    COMMENT "Cross-compiling for iOS ARM64"
)

# macOS ARM64 (Apple Silicon)
add_custom_target(cross-macos-arm64
    COMMAND ${CMAKE_COMMAND} -DCMAKE_OSX_ARCHITECTURES=arm64
        -B build_macos_arm64 ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build build_macos_arm64
    COMMENT "Cross-compiling for macOS ARM64"
)

# Linux ARM64
add_custom_target(cross-linux-arm64
    COMMAND ${CMAKE_COMMAND} -DCMAKE_SYSTEM_NAME=Linux
        -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc
        -B build_linux_arm64 ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build build_linux_arm64
    COMMENT "Cross-compiling for Linux ARM64"
)

# Linux ARM32
add_custom_target(cross-linux-arm
    COMMAND ${CMAKE_COMMAND} -DCMAKE_SYSTEM_NAME=Linux
        -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc
        -B build_linux_arm ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build build_linux_arm
    COMMENT "Cross-compiling for Linux ARM32"
)

# ==================== CLEAN TARGETS ====================

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove CMakeCache.txt
    COMMENT "Cleaning all build artifacts"
)

# ==================== HELP TARGET ====================

add_custom_target(show_help
    COMMAND ${CMAKE_COMMAND} -E echo "XzalgoChain CMake Build System"
    COMMAND ${CMAKE_COMMAND} -E echo "========================================"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Basic Commands:"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake ..               - Configure project"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build .        - Build project"
    COMMAND ${CMAKE_COMMAND} -E echo "  sudo make install      - Install libraries"
    COMMAND ${CMAKE_COMMAND} -E echo "  make uninstall         - Uninstall libraries"
    COMMAND ${CMAKE_COMMAND} -E echo "  make info              - Show configuration"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Build Variants:"
    COMMAND ${CMAKE_COMMAND} -E echo "  make debug             - Debug build"
    COMMAND ${CMAKE_COMMAND} -E echo "  make release           - Release build"
    COMMAND ${CMAKE_COMMAND} -E echo "  make static_build      - Static libraries only"
    COMMAND ${CMAKE_COMMAND} -E echo "  make shared_build      - Shared libraries only"
    COMMAND ${CMAKE_COMMAND} -E echo "  make scalar_build      - Force scalar mode"
    COMMAND ${CMAKE_COMMAND} -E echo "  make nosimd_build      - Disable SIMD"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Cross-Compilation:"
    COMMAND ${CMAKE_COMMAND} -E echo "  make cross-android-arm64     - Android ARM64"
    COMMAND ${CMAKE_COMMAND} -E echo "  make cross-android-arm       - Android ARM32"
    COMMAND ${CMAKE_COMMAND} -E echo "  make cross-android-x86       - Android x86"
    COMMAND ${CMAKE_COMMAND} -E echo "  make cross-android-x86_64    - Android x86_64"
    COMMAND ${CMAKE_COMMAND} -E echo "  make cross-windows           - Windows 64-bit"
    COMMAND ${CMAKE_COMMAND} -E echo "  make cross-windows-32        - Windows 32-bit"
    COMMAND ${CMAKE_COMMAND} -E echo "  make cross-ios-arm64         - iOS ARM64"
    COMMAND ${CMAKE_COMMAND} -E echo "  make cross-macos-arm64       - macOS ARM64"
    COMMAND ${CMAKE_COMMAND} -E echo "  make cross-linux-arm64       - Linux ARM64"
    COMMAND ${CMAKE_COMMAND} -E echo "  make cross-linux-arm         - Linux ARM32"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Configuration Options:"
    COMMAND ${CMAKE_COMMAND} -E echo "  -DBUILD_SHARED_LIBS=ON      Build shared library"
    COMMAND ${CMAKE_COMMAND} -E echo "  -DBUILD_STATIC_LIBS=ON      Build static library"
    COMMAND ${CMAKE_COMMAND} -E echo "  -DXZALGOCHAIN_FORCE_SCALAR=ON  Force scalar mode"
    COMMAND ${CMAKE_COMMAND} -E echo "  -DXZALGOCHAIN_ENABLE_SIMD=OFF  Disable SIMD"
    COMMAND ${CMAKE_COMMAND} -E echo "  -DXZALGOCHAIN_USE_OPENMP=OFF   Disable OpenMP"
    COMMAND ${CMAKE_COMMAND} -E echo "========================================"
)

# Declare phony targets
set_property(GLOBAL PROPERTY
    XZALGOCHAIN_PHONY_TARGETS
    info debug release static_build shared_build scalar_build nosimd_build
    cross-android-arm64 cross-android-arm cross-android-x86 cross-android-x86_64
    cross-windows cross-windows-32 cross-ios-arm64 cross-macos-arm64
    cross-linux-arm64 cross-linux-arm clean-all show_help uninstall
)

# ==================== PACKAGING ====================

set(CPACK_PACKAGE_NAME "XzalgoChain")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "320-bit Cryptographic Hash Function")
set(CPACK_PACKAGE_VENDOR "Xzrayツ")
set(CPACK_PACKAGE_CONTACT "Xzray@proton.me")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")

# Generator-specific settings
set(CPACK_GENERATOR "TGZ;ZIP")

if(WIN32)
    set(CPACK_GENERATOR "${CPACK_GENERATOR};NSIS")
elseif(APPLE)
    set(CPACK_GENERATOR "${CPACK_GENERATOR};Bundle")
elseif(UNIX)
    set(CPACK_GENERATOR "${CPACK_GENERATOR};DEB;RPM")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Xzrayツ")
    set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
endif()

include(CPack)
