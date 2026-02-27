# XzalgoChain - 320-bit Cryptographic Hash Function
# Makefile - Build configuration
# Part of XzalgoChain project (Apache 2.0 License)

# Compiler selection - using clang as the default compiler
# clang offers excellent cross-compilation support and diagnostic messages
CC = gcc

# Directory containing the XzalgoChain header files
XZALGO_DIR = XzalgoChain

# Include paths for header files
INCLUDES = -I$(XZALGO_DIR)

# Target executable name (platform-specific suffix will be added for Windows)
TARGET = xzalgo320sum

# Source files to compile
SOURCES = xzalgo320sum.c

# Object files derived from source files
OBJECTS = $(SOURCES:.c=.o)

# Installation prefix (default: /usr/local)
PREFIX = /usr/local

# Detect operating system and architecture for platform-specific optimizations
# 2>/dev/null suppresses error messages if uname is not available
UNAME_S := $(shell uname -s 2>/dev/null || echo "Unknown")
UNAME_M := $(shell uname -m 2>/dev/null || echo "Unknown")

# Base compiler flags:
# -Wall -Wextra: Enable most warning messages
# -O3: Aggressive optimization for maximum performance
# -flto: Enable Link Time Optimization for better optimization across files
CFLAGS = -Wall -Wextra -O3 -flto

# Linker flags
LDFLAGS = -flto

# Platform-specific configuration blocks
# Each block defines flags, defines, and architecture optimizations for different OSes

# Linux platform configuration
ifeq ($(UNAME_S),Linux)
    # Define Linux and enable POSIX.1-2008 and XSI extensions
    CFLAGS += -DLINUX -D_XOPEN_SOURCE=700
    # Add dl library for dynamic linking support
    LDFLAGS += -ldl
    
    # Architecture-specific optimizations for Linux
    ifeq ($(UNAME_M),x86_64)
        # x86_64: Enable native optimizations, AVX2 and FMA instructions
        CFLAGS += -march=native -mtune=native -mavx2 -mfma
    else ifeq ($(UNAME_M),i686)
        # 32-bit x86: Same optimizations as x86_64
        CFLAGS += -march=native -mtune=native -mavx2 -mfma
    else ifeq ($(UNAME_M),aarch64)
        # ARM64: Enable ARMv8-A with FP and SIMD extensions
        CFLAGS += -march=armv8-a+fp+simd
    else ifeq ($(UNAME_M),armv7l)
        # ARMv7: Enable NEON SIMD and hard-float ABI
        CFLAGS += -march=armv7-a -mfpu=neon -mfloat-abi=hard
    else ifeq ($(UNAME_M),armv6l)
        # ARMv6: Enable VFP floating point
        CFLAGS += -march=armv6 -mfpu=vfp
    endif

# macOS/Darwin platform configuration
else ifeq ($(UNAME_S),Darwin)
    # Define Apple and Darwin source for platform-specific features
    CFLAGS += -DAPPLE -D_DARWIN_C_SOURCE
    
    # Architecture-specific optimizations for macOS
    ifeq ($(UNAME_M),x86_64)
        # Intel Mac: Native optimizations with AVX2/FMA
        CFLAGS += -march=native -mtune=native -mavx2 -mfma
    else ifeq ($(UNAME_M),arm64)
        # Apple Silicon: Target ARM64 with Apple A14 CPU optimizations
        CFLAGS += -arch arm64 -mcpu=apple-a14
        # Distinguish between macOS and iOS using sysctl
        ifeq ($(shell sysctl -n hw.model 2>/dev/null | grep -i "iPad\|iPhone"),)
            # No iPad/iPhone in model name -> macOS
            CFLAGS += -DMACOS
        else
            # iOS device detected
            CFLAGS += -DIOS -mios-version-min=12.0
        endif
    endif

# Windows platform configuration
else ifeq ($(OS),Windows_NT)
    # Add .exe extension for Windows executables
    TARGET := $(TARGET).exe
    # Define Windows macros
    CFLAGS += -DWIN32 -D_WIN32 -DWIN64
    # Statically link runtime libraries for better portability
    LDFLAGS += -static-libgcc -static-libstdc++
    
    # Architecture-specific optimizations for Windows
    ifeq ($(UNAME_M),x86_64)
        # 64-bit Windows
        CFLAGS += -m64 -march=native -mtune=native -mavx2 -mfma
    else
        # 32-bit Windows
        CFLAGS += -m32 -march=native -mtune=native -mavx2 -mfma
    endif

# FreeBSD platform configuration
else ifeq ($(UNAME_S),FreeBSD)
    # Define FreeBSD and BSD visibility
    CFLAGS += -DFREEBSD -D__BSD_VISIBLE
    # Add execinfo library for backtrace support
    LDFLAGS += -lexecinfo
    # Architecture-specific optimizations for FreeBSD
    ifeq ($(UNAME_M),amd64)
        # AMD64 (x86_64) optimizations
        CFLAGS += -march=native -mtune=native -mavx2 -mfma
    endif
endif

# Android platform configuration (can be combined with other OS detection)
ifdef ANDROID
    # Define Android and minimum API level 21 (Android 5.0)
    CFLAGS += -DANDROID -D__ANDROID_API__=21
    # Architecture-specific optimizations for Android
    ifeq ($(UNAME_M),aarch64)
        # ARM64: Enable ARMv8-A with FP/SIMD
        CFLAGS += -march=armv8-a+fp+simd
    else ifeq ($(UNAME_M),armv7l)
        # ARMv7: Enable NEON with softfp ABI (standard for Android)
        CFLAGS += -march=armv7-a -mfpu=neon -mfloat-abi=softfp
    endif
endif

# OpenMP detection for parallel processing support
# Test if compiler supports OpenMP by compiling a minimal program
HAVE_OPENMP := $(shell echo "int main(){}" | $(CC) -fopenmp -x c - -o /dev/null 2>/dev/null && echo yes)
ifeq ($(HAVE_OPENMP),yes)
    # Enable OpenMP if available
    CFLAGS += -fopenmp
    LDFLAGS += -fopenmp
endif

# SIMD instruction set extensions based on architecture
# These provide hardware-accelerated cryptographic operations

# x86/x86_64 architecture SIMD extensions
ifeq ($(UNAME_M),$(filter $(UNAME_M),x86_64 i686 i386))
    # SSE4.2: String and text processing instructions
    # PCLMUL: Carry-less multiplication (for GHASH)
    # AES: AES-NI instructions for hardware-accelerated AES
    CFLAGS += -msse4.2 -mpclmul -maes

# ARM64 architecture SIMD extensions
else ifeq ($(UNAME_M),$(filter $(UNAME_M),aarch64 arm64))
    # NEON is always available on ARM64, define macros for it
    CFLAGS += -D__ARM_NEON -D__ARM_NEON__

# ARMv7 architecture SIMD extensions
else ifeq ($(UNAME_M),$(filter $(UNAME_M),armv7l armv7a armv8l))
    # Enable NEON SIMD for ARMv7
    CFLAGS += -D__ARM_NEON -D__ARM_NEON__ -mfpu=neon
endif

# Debug mode configuration
ifdef DEBUG
    # Enable debugging symbols, disable optimizations, define DEBUG macro
    CFLAGS += -g -O0 -DDEBUG
else
    # Define NDEBUG to disable assertions in release builds
    CFLAGS += -DNDEBUG
endif

# Profiling support (for gprof)
ifdef PROFILE
    # Add profiling hooks
    CFLAGS += -pg
    LDFLAGS += -pg
endif

# Sanitizer support for debugging memory issues
ifdef SANITIZE
    # Enable AddressSanitizer and UndefinedBehaviorSanitizer
    CFLAGS += -fsanitize=address -fsanitize=undefined -g
    LDFLAGS += -fsanitize=address -fsanitize=undefined
endif

# Static linking
ifdef STATIC
    # Produce a fully statically linked executable
    CFLAGS += -static
    LDFLAGS += -static
endif

# Size-optimized build
ifdef TINY
    # Optimize for size and strip symbols
    CFLAGS += -Os -s
endif

# Verbose output mode
ifdef VERBOSE
    # Define VERBOSE macro for additional runtime logging
    CFLAGS += -DVERBOSE
endif

# Default target: check dependencies, build, and run a quick test
all: check-deps $(TARGET)
	@echo "=========================================="
	@echo "Build complete: $(TARGET)"
	@echo "Platform: $(UNAME_S) $(UNAME_M)"
	@echo "OpenMP: $(HAVE_OPENMP)"
	@echo "Running $(TARGET)..."
	@./$(TARGET) -v
	@$(MAKE) clean-obj

# Link object files to create the final executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Compile C source files to object files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean only object files, keep executable
clean-obj:
	@echo "Cleaning object files..."
	@rm -f $(OBJECTS)
	@echo "Clean complete"

# Full clean: remove both objects and executable
clean: clean-obj
	@rm -f $(TARGET)

# Install the executable to the system
install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/

# Remove the installed executable
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

# Build variants - each cleans first then builds with specific flags
debug: clean
	$(MAKE) DEBUG=1 all

profile: clean
	$(MAKE) PROFILE=1 all

sanitize: clean
	$(MAKE) SANITIZE=1 all

static: clean
	$(MAKE) STATIC=1 all

tiny: clean
	$(MAKE) TINY=1 all

verbose: clean
	$(MAKE) VERBOSE=1 all

# Build only (no cleanup or test run)
build: $(TARGET)

# Run the executable with version flag
run: $(TARGET)
	@./$(TARGET) -v

# Check if required dependencies exist before building
check-deps:
	@test -d "$(XZALGO_DIR)" || (echo "$(XZALGO_DIR) not found!" && exit 1)
	@test -f "$(XZALGO_DIR)/XzalgoChain.h" || (echo "XzalgoChain.h not found!" && exit 1)
	@echo "Checking dependencies for $(UNAME_S) $(UNAME_M)..."

# Run basic tests on the executable
test: $(TARGET)
	@echo "Running tests on $(UNAME_S) $(UNAME_M)..."
	@./$(TARGET) -v
	@./$(TARGET) -h
	@./$(TARGET) -i "Hello World"

# Cross-compilation targets for various platforms
# Each target sets appropriate compiler and platform defines

# Windows cross-compilation (64-bit and 32-bit)
cross-windows:
	$(MAKE) CC=x86_64-w64-mingw32-gcc OS=Windows_NT clean all

cross-windows-32:
	$(MAKE) CC=i686-w64-mingw32-gcc OS=Windows_NT clean all

# Android cross-compilation for various architectures
cross-android-arm64:
	$(MAKE) CC=aarch64-linux-android21-clang ANDROID=1 clean all

cross-android-arm:
	$(MAKE) CC=armv7a-linux-androideabi21-clang ANDROID=1 clean all

cross-android-x86:
	$(MAKE) CC=i686-linux-android21-clang ANDROID=1 clean all

cross-android-x86_64:
	$(MAKE) CC=x86_64-linux-android21-clang ANDROID=1 clean all

# iOS cross-compilation (requires Xcode)
cross-ios-arm64:
	$(MAKE) CC=clang -arch arm64 CFLAGS="-isysroot $$(xcrun --sdk iphoneos --show-sdk-path)" clean all

# macOS ARM64 (Apple Silicon) native/cross build
cross-macos-arm64:
	$(MAKE) CC=clang -arch arm64 clean all

# Linux ARM cross-compilation
cross-linux-arm64:
	$(MAKE) CC=aarch64-linux-gnu-gcc clean all

cross-linux-arm:
	$(MAKE) CC=arm-linux-gnueabihf-gcc clean all

# Display system and build configuration information
info:
	@echo "System: $(UNAME_S)"
	@echo "Architecture: $(UNAME_M)"
	@echo "Compiler: $(CC)"
	@echo "OpenMP: $(HAVE_OPENMP)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"

# Display help information with all available targets
help:
	@echo "XzalgoChain Makefile - Multi-platform support"
	@echo ""
	@echo "Platform targets:"
	@echo "  all                    - Default build for current platform"
	@echo "  info                   - Show platform information"
	@echo ""
	@echo "Cross-compilation targets:"
	@echo "  cross-windows          - Build for Windows 64-bit"
	@echo "  cross-windows-32       - Build for Windows 32-bit"
	@echo "  cross-android-arm64    - Build for Android ARM64"
	@echo "  cross-android-arm      - Build for Android ARM32"
	@echo "  cross-android-x86      - Build for Android x86"
	@echo "  cross-android-x86_64   - Build for Android x86_64"
	@echo "  cross-ios-arm64        - Build for iOS ARM64"
	@echo "  cross-macos-arm64      - Build for macOS ARM64"
	@echo "  cross-linux-arm64      - Build for Linux ARM64"
	@echo "  cross-linux-arm        - Build for Linux ARM32"
	@echo ""
	@echo "Build variants:"
	@echo "  debug                  - Debug build"
	@echo "  profile                - Profile build"
	@echo "  sanitize               - Sanitizer build"
	@echo "  static                 - Static build"
	@echo "  tiny                   - Size-optimized build"
	@echo "  verbose                - Verbose build"
	@echo ""
	@echo "Other targets:"
	@echo "  build                  - Build only"
	@echo "  run                    - Run the program"
	@echo "  test                   - Run tests"
	@echo "  clean                  - Clean all"
	@echo "  clean-obj              - Clean objects only"
	@echo "  install                - Install to $(PREFIX)"
	@echo "  uninstall              - Uninstall"
	@echo "  help                   - Show this help"

# Declare phony targets (targets that don't represent actual files)
.PHONY: all build run clean clean-obj install uninstall debug profile sanitize static tiny verbose test info help
.PHONY: cross-windows cross-windows-32 cross-android-arm64 cross-android-arm cross-android-x86 cross-android-x86_64
.PHONY: cross-ios-arm64 cross-macos-arm64 cross-linux-arm64 cross-linux-arm

# Dependency: object files depend on the header file
$(OBJECTS): $(XZALGO_DIR)/XzalgoChain.h
