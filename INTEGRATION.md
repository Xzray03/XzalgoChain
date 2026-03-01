# Integration Examples

This document provides examples of how to use XzalgoChain library (static and shared) from various programming languages.

## Library Structure
After building, you will have:
- **Static library**: `libXzalgoChain.a` (Unix) / `XzalgoChain.lib` (Windows)
- **Shared library**: `libXzalgoChain.so` (Linux) / `libXzalgoChain.dylib` (macOS) / `XzalgoChain.dll` (Windows)
- **Header files**: All `.h` files in the `XzalgoChain/` directory

---

## C

### Using Static Library
```c
// example.c
#include <stdio.h>
#include <string.h>
#include "XzalgoChain/XzalgoChain.h"

int main() {
    uint8_t hash[XZALGOCHAIN_HASH_SIZE];
    const char *message = "Hello World";
    
    // Single-shot hash
    xzalgochain_lib((uint8_t*)message, strlen(message), hash);
    
    printf("Hash: ");
    for(int i = 0; i < XZALGOCHAIN_HASH_SIZE; i++) printf("%02x", hash[i]);
    printf("\n");
    
    // Multi-part hashing
    XzalgoChain_CTX ctx;
    xzalgochain_init_lib(&ctx);
    xzalgochain_update_lib(&ctx, (uint8_t*)"Hello ", 6);
    xzalgochain_update_lib(&ctx, (uint8_t*)"World", 5);
    xzalgochain_final_lib(&ctx, hash);
    
    // Get SIMD information
    printf("SIMD Type: %s\n", xzalgochain_get_simd_name_lib());
    printf("Version: %s\n", xzalgochain_version_lib());
    printf("Platform: %s\n", xzalgochain_platform_info_lib());
    
    return 0;
}
```

**Compile and link:**
```bash
# Static linking
gcc -o example example.c -I. -L. -lXzalgoChain

# Dynamic linking
gcc -o example example.c -I. -L. -lXzalgoChain

# With custom install path
gcc -o example example.c -I/usr/local/include -L/usr/local/lib -lXzalgoChain
```

### Using Shared Library with dlopen/dlclose
```c
// dynamic_example.c
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "XzalgoChain/XzalgoChain.h"

typedef void (*hash_func)(const uint8_t*, size_t, uint8_t*);
typedef const char* (*string_func)(void);

int main() {
    void *handle = dlopen("./libXzalgoChain.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error: %s\n", dlerror());
        return 1;
    }
    
    hash_func xzalgochain_lib = (hash_func)dlsym(handle, "xzalgochain_lib");
    string_func get_simd = (string_func)dlsym(handle, "xzalgochain_get_simd_name_lib");
    string_func get_version = (string_func)dlsym(handle, "xzalgochain_version_lib");
    
    uint8_t hash[XZALGOCHAIN_HASH_SIZE];
    xzalgochain_lib((uint8_t*)"Hello", 5, hash);
    
    printf("SIMD: %s\n", get_simd());
    printf("Version: %s\n", get_version());
    
    dlclose(handle);
    return 0;
}
```

---

## C++

### Using Static Library
```cpp
// example.cpp
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include "XzalgoChain/XzalgoChain.h"

class XzalgoHasher {
private:
    XzalgoChain_CTX ctx;
    
public:
    XzalgoHasher() {
        xzalgochain_init_lib(&ctx);
    }
    
    ~XzalgoHasher() {
        xzalgochain_ctx_wipe_lib(&ctx);
    }
    
    void update(const std::vector<uint8_t>& data) {
        xzalgochain_update_lib(&ctx, data.data(), data.size());
    }
    
    void update(const std::string& data) {
        xzalgochain_update_lib(&ctx, (uint8_t*)data.data(), data.size());
    }
    
    std::vector<uint8_t> final() {
        std::vector<uint8_t> hash(XZALGOCHAIN_HASH_SIZE);
        xzalgochain_final_lib(&ctx, hash.data());
        return hash;
    }
    
    void reset() {
        xzalgochain_ctx_reset_lib(&ctx);
    }
    
    static std::vector<uint8_t> hash(const std::string& data) {
        std::vector<uint8_t> hash(XZALGOCHAIN_HASH_SIZE);
        xzalgochain_lib((uint8_t*)data.data(), data.size(), hash.data());
        return hash;
    }
    
    static std::string hash_hex(const std::string& data) {
        auto hash = hash(data);
        std::stringstream ss;
        for(auto b : hash) ss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
        return ss.str();
    }
    
    static bool equals(const std::vector<uint8_t>& h1, const std::vector<uint8_t>& h2) {
        if (h1.size() != XZALGOCHAIN_HASH_SIZE || h2.size() != XZALGOCHAIN_HASH_SIZE)
            return false;
        return xzalgochain_equals_lib(h1.data(), h2.data()) != 0;
    }
};

int main() {
    // Static method usage
    auto hash1 = XzalgoHasher::hash("Hello World");
    std::cout << "Hash (hex): " << XzalgoHasher::hash_hex("Hello World") << std::endl;
    
    // Streaming interface
    XzalgoHasher hasher;
    hasher.update("Hello ");
    hasher.update("World");
    auto hash2 = hasher.final();
    
    // Compare hashes
    std::cout << "Hashes equal: " << (XzalgoHasher::equals(hash1, hash2) ? "yes" : "no") << std::endl;
    
    // SIMD info
    std::cout << "SIMD: " << xzalgochain_get_simd_name_lib() << std::endl;
    std::cout << "Version: " << xzalgochain_version_lib() << std::endl;
    
    return 0;
}
```

**Compile:**
```bash
# Static linking
g++ -std=c++11 -o example example.cpp -I. -L. -lXzalgoChain

# Dynamic linking
g++ -std=c++11 -o example example.cpp -I. -L. -lXzalgoChain
```

---

## C#

### Using P/Invoke with Shared Library
```csharp
// XzalgoChain.cs
using System;
using System.Runtime.InteropServices;
using System.Text;

public class XzalgoChain
{
    public const int HASH_SIZE = 40;
    
    // Platform-specific library name
    private const string LibName =
        #if _WIN32
            "XzalgoChain.dll"
        #elif __APPLE__
            "libXzalgoChain.dylib"
        #else
            "libXzalgoChain.so"
        #endif
    
    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void xzalgochain_lib(byte[] data, ulong len, byte[] output);
    
    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr xzalgochain_get_simd_name_lib();
    
    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr xzalgochain_version_lib();
    
    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr xzalgochain_platform_info_lib();
    
    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int xzalgochain_avx2_supported_lib();
    
    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int xzalgochain_neon_supported_lib();
    
    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void xzalgochain_init_lib(out XzalgoChain_CTX ctx);
    
    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void xzalgochain_update_lib(ref XzalgoChain_CTX ctx, byte[] data, ulong len);
    
    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void xzalgochain_final_lib(ref XzalgoChain_CTX ctx, byte[] output);
    
    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void xzalgochain_ctx_wipe_lib(ref XzalgoChain_CTX ctx);
    
    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int xzalgochain_equals_lib(byte[] h1, byte[] h2);
    
    [StructLayout(LayoutKind.Sequential)]
    public struct XzalgoChain_CTX
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 5)]
        public ulong[] h;
        
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 100)]
        public ulong[] little_box_state; // Flattened [10][10]
        
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 25)]
        public ulong[] big_box_state; // Flattened [5][5]
        
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 128)]
        public byte[] buffer;
        
        public ulong buffer_len;
        public ulong total_bits;
        public byte simd_type;
    }
    
    public static byte[] Hash(byte[] data)
    {
        byte[] hash = new byte[HASH_SIZE];
        xzalgochain_lib(data, (ulong)data.Length, hash);
        return hash;
    }
    
    public static string Hash(string text)
    {
        byte[] data = Encoding.UTF8.GetBytes(text);
        byte[] hash = Hash(data);
        return BitConverter.ToString(hash).Replace("-", "").ToLower();
    }
    
    public static string GetSIMDInfo()
    {
        string name = Marshal.PtrToStringAnsi(xzalgochain_get_simd_name_lib());
        string version = Marshal.PtrToStringAnsi(xzalgochain_version_lib());
        string platform = Marshal.PtrToStringAnsi(xzalgochain_platform_info_lib());
        bool avx2 = xzalgochain_avx2_supported_lib() != 0;
        bool neon = xzalgochain_neon_supported_lib() != 0;
        return $"XzalgoChain v{version}\nPlatform: {platform}\nSIMD: {name}\nAVX2: {avx2}, NEON: {neon}";
    }
    
    public static bool Equals(byte[] h1, byte[] h2)
    {
        if (h1.Length != HASH_SIZE || h2.Length != HASH_SIZE)
            return false;
        return xzalgochain_equals_lib(h1, h2) != 0;
    }
}

// Program.cs
class Program
{
    static void Main()
    {
        Console.WriteLine(XzalgoChain.GetSIMDInfo());
        Console.WriteLine();
        
        // Single-shot hash
        string hash = XzalgoChain.Hash("Hello World");
        Console.WriteLine($"Hash: {hash}");
        
        // Multi-part hashing
        XzalgoChain.XzalgoChain_CTX ctx;
        XzalgoChain.xzalgochain_init_lib(out ctx);
        
        byte[] data1 = Encoding.UTF8.GetBytes("Hello ");
        byte[] data2 = Encoding.UTF8.GetBytes("World");
        
        XzalgoChain.xzalgochain_update_lib(ref ctx, data1, (ulong)data1.Length);
        XzalgoChain.xzalgochain_update_lib(ref ctx, data2, (ulong)data2.Length);
        
        byte[] finalHash = new byte[XzalgoChain.HASH_SIZE];
        XzalgoChain.xzalgochain_final_lib(ref ctx, finalHash);
        
        Console.WriteLine($"Multi-part: {BitConverter.ToString(finalHash).Replace("-", "").ToLower()}");
        
        // Cleanup
        XzalgoChain.xzalgochain_ctx_wipe_lib(ref ctx);
    }
}
```

**Build and run:**
```bash
# Compile
csc Program.cs XzalgoChain.cs

# Run with library in same directory
mono Program.exe  # On Unix
./Program.exe     # On Windows
```

---

## Rust

### Using FFI Bindings
```rust
// build.rs
fn main() {
    println!("cargo:rustc-link-search=native=.");
    println!("cargo:rustc-link-lib=XzalgoChain");
    println!("cargo:rerun-if-changed=wrapper.h");
}

// src/main.rs
use std::ffi::CStr;
use std::os::raw::c_char;

pub const XZALGOCHAIN_HASH_SIZE: usize = 40;

#[repr(C)]
pub struct XzalgoChain_CTX {
    h: [u64; 5],
    little_box_state: [[u64; 10]; 10],
    big_box_state: [[u64; 5]; 5],
    buffer: [u8; 128],
    buffer_len: usize,
    total_bits: u64,
    simd_type: u8,
}

extern "C" {
    // Core functions
    fn xzalgochain_lib(data: *const u8, len: usize, output: *mut u8);
    fn xzalgochain_init_lib(ctx: *mut XzalgoChain_CTX);
    fn xzalgochain_update_lib(ctx: *mut XzalgoChain_CTX, data: *const u8, len: usize);
    fn xzalgochain_final_lib(ctx: *mut XzalgoChain_CTX, output: *mut u8);
    
    // Context management
    fn xzalgochain_ctx_reset_lib(ctx: *mut XzalgoChain_CTX);
    fn xzalgochain_ctx_wipe_lib(ctx: *mut XzalgoChain_CTX);
    
    // Utility
    fn xzalgochain_copy_lib(dst: *mut u8, src: *const u8);
    fn xzalgochain_equals_lib(h1: *const u8, h2: *const u8) -> i32;
    
    // Info functions
    fn xzalgochain_version_lib() -> *const c_char;
    fn xzalgochain_platform_info_lib() -> *const c_char;
    fn xzalgochain_get_simd_name_lib() -> *const c_char;
    fn xzalgochain_get_simd_type_lib() -> i32;
    
    // SIMD detection
    fn xzalgochain_avx2_supported_lib() -> i32;
    fn xzalgochain_neon_supported_lib() -> i32;
    
    // Force scalar mode
    fn xzalgochain_force_scalar_lib(force: i32);
    fn xzalgochain_is_forced_scalar_lib() -> i32;
}

pub fn hash(data: &[u8]) -> [u8; XZALGOCHAIN_HASH_SIZE] {
    let mut output = [0u8; XZALGOCHAIN_HASH_SIZE];
    unsafe {
        xzalgochain_lib(data.as_ptr(), data.len(), output.as_mut_ptr());
    }
    output
}

pub fn hash_string(data: &str) -> String {
    let hash = hash(data.as_bytes());
    hash.iter().map(|b| format!("{:02x}", b)).collect()
}

pub struct Hasher {
    ctx: XzalgoChain_CTX,
}

impl Hasher {
    pub fn new() -> Self {
        let mut ctx = unsafe { std::mem::zeroed() };
        unsafe { xzalgochain_init_lib(&mut ctx) };
        Hasher { ctx }
    }
    
    pub fn update(&mut self, data: &[u8]) {
        unsafe { xzalgochain_update_lib(&mut self.ctx, data.as_ptr(), data.len()) };
    }
    
    pub fn finalize(mut self) -> [u8; XZALGOCHAIN_HASH_SIZE] {
        let mut output = [0u8; XZALGOCHAIN_HASH_SIZE];
        unsafe { xzalgochain_final_lib(&mut self.ctx, output.as_mut_ptr()) };
        output
    }
    
    pub fn reset(&mut self) {
        unsafe { xzalgochain_ctx_reset_lib(&mut self.ctx) };
    }
}

impl Drop for Hasher {
    fn drop(&mut self) {
        unsafe { xzalgochain_ctx_wipe_lib(&mut self.ctx) };
    }
}

pub fn equals(h1: &[u8], h2: &[u8]) -> bool {
    if h1.len() != XZALGOCHAIN_HASH_SIZE || h2.len() != XZALGOCHAIN_HASH_SIZE {
        return false;
    }
    unsafe { xzalgochain_equals_lib(h1.as_ptr(), h2.as_ptr()) != 0 }
}

fn main() {
    unsafe {
        let version = CStr::from_ptr(xzalgochain_version_lib()).to_str().unwrap();
        let platform = CStr::from_ptr(xzalgochain_platform_info_lib()).to_str().unwrap();
        let simd_name = CStr::from_ptr(xzalgochain_get_simd_name_lib()).to_str().unwrap();
        
        println!("XzalgoChain v{}", version);
        println!("Platform: {}", platform);
        println!("SIMD: {}", simd_name);
        
        let avx2 = xzalgochain_avx2_supported_lib() != 0;
        let neon = xzalgochain_neon_supported_lib() != 0;
        println!("AVX2: {}, NEON: {}", avx2, neon);
    }
    
    // Single-shot hash
    let hash = hash_string("Hello World");
    println!("\nHash of 'Hello World': {}", hash);
    
    // Multi-part hashing
    let mut hasher = Hasher::new();
    hasher.update(b"Hello ");
    hasher.update(b"World");
    let result = hasher.finalize();
    
    println!("Multi-part: {}", result.iter().map(|b| format!("{:02x}", b)).collect::<String>());
}
```

---

## Python

### Using ctypes
```python
import ctypes
import os
import platform

# Load the appropriate library
if platform.system() == "Windows":
    lib_name = "XzalgoChain.dll"
elif platform.system() == "Darwin":
    lib_name = "libXzalgoChain.dylib"
else:
    lib_name = "libXzalgoChain.so"

lib = ctypes.CDLL(os.path.join(os.path.dirname(__file__), lib_name))

# Constants
XZALGOCHAIN_HASH_SIZE = 40

# Define function prototypes
lib.xzalgochain_lib.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(ctypes.c_uint8)]
lib.xzalgochain_lib.restype = None

lib.xzalgochain_version_lib.argtypes = []
lib.xzalgochain_version_lib.restype = ctypes.c_char_p

lib.xzalgochain_platform_info_lib.argtypes = []
lib.xzalgochain_platform_info_lib.restype = ctypes.c_char_p

lib.xzalgochain_get_simd_name_lib.argtypes = []
lib.xzalgochain_get_simd_name_lib.restype = ctypes.c_char_p

lib.xzalgochain_get_simd_type_lib.argtypes = []
lib.xzalgochain_get_simd_type_lib.restype = ctypes.c_int

lib.xzalgochain_avx2_supported_lib.argtypes = []
lib.xzalgochain_avx2_supported_lib.restype = ctypes.c_int

lib.xzalgochain_neon_supported_lib.argtypes = []
lib.xzalgochain_neon_supported_lib.restype = ctypes.c_int

lib.xzalgochain_equals_lib.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint8)]
lib.xzalgochain_equals_lib.restype = ctypes.c_int

# Context structure
class XzalgoChainCTX(ctypes.Structure):
    _fields_ = [
        ("h", ctypes.c_uint64 * 5),
        ("little_box_state", (ctypes.c_uint64 * 10) * 10),
        ("big_box_state", (ctypes.c_uint64 * 5) * 5),
        ("buffer", ctypes.c_uint8 * 128),
        ("buffer_len", ctypes.c_size_t),
        ("total_bits", ctypes.c_uint64),
        ("simd_type", ctypes.c_uint8)
    ]

lib.xzalgochain_init_lib.argtypes = [ctypes.POINTER(XzalgoChainCTX)]
lib.xzalgochain_init_lib.restype = None

lib.xzalgochain_update_lib.argtypes = [ctypes.POINTER(XzalgoChainCTX), ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t]
lib.xzalgochain_update_lib.restype = None

lib.xzalgochain_final_lib.argtypes = [ctypes.POINTER(XzalgoChainCTX), ctypes.POINTER(ctypes.c_uint8)]
lib.xzalgochain_final_lib.restype = None

lib.xzalgochain_ctx_wipe_lib.argtypes = [ctypes.POINTER(XzalgoChainCTX)]
lib.xzalgochain_ctx_wipe_lib.restype = None

def hash_data(data):
    """Hash bytes data and return bytes"""
    hash_array = (ctypes.c_uint8 * XZALGOCHAIN_HASH_SIZE)()
    lib.xzalgochain_lib(ctypes.cast(data, ctypes.POINTER(ctypes.c_uint8)), 
                        len(data), hash_array)
    return bytes(hash_array)

def hash_string(text):
    """Hash string and return hex string"""
    return hash_data(text.encode('utf-8')).hex()

def hash_hex(data):
    """Hash bytes and return hex string"""
    return hash_data(data).hex()

class XzalgoHasher:
    def __init__(self):
        self.ctx = XzalgoChainCTX()
        lib.xzalgochain_init_lib(ctypes.byref(self.ctx))
    
    def update(self, data):
        if isinstance(data, str):
            data = data.encode('utf-8')
        lib.xzalgochain_update_lib(ctypes.byref(self.ctx),
                                   ctypes.cast(data, ctypes.POINTER(ctypes.c_uint8)),
                                   len(data))
    
    def finalize(self):
        hash_array = (ctypes.c_uint8 * XZALGOCHAIN_HASH_SIZE)()
        lib.xzalgochain_final_lib(ctypes.byref(self.ctx), hash_array)
        return bytes(hash_array)
    
    def __del__(self):
        if hasattr(self, 'ctx'):
            lib.xzalgochain_ctx_wipe_lib(ctypes.byref(self.ctx))

# Example usage
if __name__ == "__main__":
    # Library information
    print(f"Version: {lib.xzalgochain_version_lib().decode()}")
    print(f"Platform: {lib.xzalgochain_platform_info_lib().decode()}")
    print(f"SIMD: {lib.xzalgochain_get_simd_name_lib().decode()}")
    print(f"AVX2: {bool(lib.xzalgochain_avx2_supported_lib())}")
    print(f"NEON: {bool(lib.xzalgochain_neon_supported_lib())}")
    
    # Single-shot hash
    print(f"\nHash of 'Hello World': {hash_string('Hello World')}")
    
    # Multi-part hashing
    hasher = XzalgoHasher()
    hasher.update("Hello ")
    hasher.update("World")
    print(f"Multi-part hash: {hasher.finalize().hex()}")
    
    # File hashing
    def hash_file(filename):
        hasher = XzalgoHasher()
        with open(filename, 'rb') as f:
            for chunk in iter(lambda: f.read(8192), b''):
                hasher.update(chunk)
        return hasher.finalize()
    
    # Test with small file
    with open('test.txt', 'w') as f:
        f.write('Hello World')
    print(f"File hash: {hash_file('test.txt').hex()}")
```

---

## Go

### Using cgo
```go
// main.go
package main

/*
#cgo LDFLAGS: -L. -lXzalgoChain
#cgo CFLAGS: -I.
#include <stdlib.h>
#include "XzalgoChain/XzalgoChain.h"
*/
import "C"
import (
    "encoding/hex"
    "fmt"
    "unsafe"
)

const HashSize = 40

// Hash returns the XzalgoChain hash of the input data
func Hash(data []byte) []byte {
    output := make([]byte, HashSize)
    C.xzalgochain_lib(
        (*C.uint8_t)(unsafe.Pointer(&data[0])),
        C.size_t(len(data)),
        (*C.uint8_t)(unsafe.Pointer(&output[0])),
    )
    return output
}

// HashString returns hex string of hash
func HashString(s string) string {
    return hex.EncodeToString(Hash([]byte(s)))
}

// Hasher provides streaming hash computation
type Hasher struct {
    ctx C.XzalgoChain_CTX
}

func NewHasher() *Hasher {
    h := &Hasher{}
    C.xzalgochain_init_lib(&h.ctx)
    return h
}

func (h *Hasher) Write(data []byte) (int, error) {
    if len(data) > 0 {
        C.xzalgochain_update_lib(
            &h.ctx,
            (*C.uint8_t)(unsafe.Pointer(&data[0])),
            C.size_t(len(data)),
        )
    }
    return len(data), nil
}

func (h *Hasher) Sum() []byte {
    output := make([]byte, HashSize)
    C.xzalgochain_final_lib(&h.ctx, (*C.uint8_t)(unsafe.Pointer(&output[0])))
    return output
}

func (h *Hasher) Reset() {
    C.xzalgochain_ctx_reset_lib(&h.ctx)
}

func (h *Hasher) Close() {
    C.xzalgochain_ctx_wipe_lib(&h.ctx)
}

// Utility functions
func Equals(h1, h2 []byte) bool {
    if len(h1) != HashSize || len(h2) != HashSize {
        return false
    }
    return C.xzalgochain_equals_lib(
        (*C.uint8_t)(unsafe.Pointer(&h1[0])),
        (*C.uint8_t)(unsafe.Pointer(&h2[0])),
    ) != 0
}

// SIMD information
func GetVersion() string {
    return C.GoString(C.xzalgochain_version_lib())
}

func GetPlatformInfo() string {
    return C.GoString(C.xzalgochain_platform_info_lib())
}

func GetSIMDName() string {
    return C.GoString(C.xzalgochain_get_simd_name_lib())
}

func GetSIMDType() int {
    return int(C.xzalgochain_get_simd_type_lib())
}

func AVX2Supported() bool {
    return C.xzalgochain_avx2_supported_lib() != 0
}

func NEONSupported() bool {
    return C.xzalgochain_neon_supported_lib() != 0
}

func ForceScalar(force bool) {
    val := 0
    if force {
        val = 1
    }
    C.xzalgochain_force_scalar_lib(C.int(val))
}

func IsForcedScalar() bool {
    return C.xzalgochain_is_forced_scalar_lib() != 0
}

func main() {
    fmt.Printf("XzalgoChain %s\n", GetVersion())
    fmt.Printf("Platform: %s\n", GetPlatformInfo())
    fmt.Printf("SIMD: %s (type %d)\n", GetSIMDName(), GetSIMDType())
    fmt.Printf("AVX2: %v, NEON: %v\n", AVX2Supported(), NEONSupported())
    
    // Single-shot hash
    hash := HashString("Hello World")
    fmt.Printf("\nHash of 'Hello World': %s\n", hash)
    
    // Multi-part hashing
    hasher := NewHasher()
    defer hasher.Close()
    
    hasher.Write([]byte("Hello "))
    hasher.Write([]byte("World"))
    result := hasher.Sum()
    fmt.Printf("Multi-part hash: %s\n", hex.EncodeToString(result))
    
    // Compare hashes
    fmt.Printf("Hashes equal: %v\n", Equals(Hash([]byte("Hello World")), result))
    
    // Force scalar mode
    ForceScalar(true)
    fmt.Printf("\nForced scalar: %v\n", IsForcedScalar())
    fmt.Printf("SIMD after forcing: %s\n", GetSIMDName())
}
```

**Build and run:**
```bash
# Set environment for cgo
export CGO_LDFLAGS="-L."
export CGO_CFLAGS="-I."

# Build
go build -o example

# Run
./example
```

---

## Ruby

### Using FFI
```ruby
# xzalgochain.rb
require 'ffi'

module XzalgoChain
  extend FFI::Library
  
  HASH_SIZE = 40
  
  # Load the appropriate library
  if FFI::Platform.windows?
    ffi_lib 'XzalgoChain.dll'
  elsif FFI::Platform.mac?
    ffi_lib 'libXzalgoChain.dylib'
  else
    ffi_lib 'libXzalgoChain.so'
  end
  
  # Core functions
  attach_function :xzalgochain_lib, [:pointer, :size_t, :pointer], :void
  attach_function :xzalgochain_init_lib, [:pointer], :void
  attach_function :xzalgochain_update_lib, [:pointer, :pointer, :size_t], :void
  attach_function :xzalgochain_final_lib, [:pointer, :pointer], :void
  attach_function :xzalgochain_ctx_wipe_lib, [:pointer], :void
  attach_function :xzalgochain_ctx_reset_lib, [:pointer], :void
  
  # Utility functions
  attach_function :xzalgochain_copy_lib, [:pointer, :pointer], :void
  attach_function :xzalgochain_equals_lib, [:pointer, :pointer], :int
  
  # Info functions
  attach_function :xzalgochain_version_lib, [], :string
  attach_function :xzalgochain_platform_info_lib, [], :string
  attach_function :xzalgochain_get_simd_name_lib, [], :string
  attach_function :xzalgochain_get_simd_type_lib, [], :int
  
  # SIMD detection
  attach_function :xzalgochain_avx2_supported_lib, [], :int
  attach_function :xzalgochain_neon_supported_lib, [], :int
  
  # Force scalar
  attach_function :xzalgochain_force_scalar_lib, [:int], :void
  attach_function :xzalgochain_is_forced_scalar_lib, [], :int
  
  # Context class
  class Context < FFI::Struct
    layout(
      :h, [:uint64, 5],
      :little_box_state, [[:uint64, 10], 10],
      :big_box_state, [[:uint64, 5], 5],
      :buffer, [:uint8, 128],
      :buffer_len, :size_t,
      :total_bits, :uint64,
      :simd_type, :uint8
    )
    
    def initialize
      super()
      XzalgoChain.xzalgochain_init_lib(self)
    end
    
    def update(data)
      data = data.to_s if data.is_a?(Symbol)
      data = data.dup.force_encoding('BINARY') if data.is_a?(String)
      
      XzalgoChain.xzalgochain_update_lib(
        self,
        FFI::MemoryPointer.from_string(data),
        data.bytesize
      )
    end
    
    def final
      output = FFI::MemoryPointer.new(:uint8, XzalgoChain::HASH_SIZE)
      XzalgoChain.xzalgochain_final_lib(self, output)
      output.read_bytes(XzalgoChain::HASH_SIZE)
    end
    
    def reset
      XzalgoChain.xzalgochain_ctx_reset_lib(self)
    end
    
    def wipe
      XzalgoChain.xzalgochain_ctx_wipe_lib(self)
    end
  end
  
  def self.hash(data)
    ctx = Context.new
    ctx.update(data)
    ctx.final
  ensure
    ctx.wipe if ctx
  end
  
  def self.hash_hex(data)
    hash(data).unpack('H*').first
  end
  
  def self.equals(h1, h2)
    return false unless h1.bytesize == HASH_SIZE && h2.bytesize == HASH_SIZE
    
    p1 = FFI::MemoryPointer.new(:uint8, HASH_SIZE)
    p1.write_bytes(h1)
    p2 = FFI::MemoryPointer.new(:uint8, HASH_SIZE)
    p2.write_bytes(h2)
    
    xzalgochain_equals_lib(p1, p2) != 0
  ensure
    p1.free if p1
    p2.free if p2
  end
end

# Example usage
if __FILE__ == $0
  puts "XzalgoChain #{XzalgoChain.xzalgochain_version_lib}"
  puts "Platform: #{XzalgoChain.xzalgochain_platform_info_lib}"
  puts "SIMD: #{XzalgoChain.xzalgochain_get_simd_name_lib} (type #{XzalgoChain.xzalgochain_get_simd_type_lib})"
  puts "AVX2: #{XzalgoChain.xzalgochain_avx2_supported_lib == 1}"
  puts "NEON: #{XzalgoChain.xzalgochain_neon_supported_lib == 1}"
  
  # Single-shot hash
  hash = XzalgoChain.hash_hex("Hello World")
  puts "\nHash of 'Hello World': #{hash}"
  
  # Multi-part hashing
  ctx = XzalgoChain::Context.new
  ctx.update("Hello ")
  ctx.update("World")
  result = ctx.final.unpack('H*').first
  ctx.wipe
  puts "Multi-part hash: #{result}"
  
  # Compare
  hash1 = XzalgoChain.hash("Hello World")
  hash2 = XzalgoChain.hash("Hello World")
  puts "Hashes equal: #{XzalgoChain.equals(hash1, hash2)}"
  
  # File hashing
  File.write('test.txt', 'Hello World')
  ctx = XzalgoChain::Context.new
  File.open('test.txt', 'rb') do |f|
    while chunk = f.read(8192)
      ctx.update(chunk)
    end
  end
  puts "File hash: #{ctx.final.unpack('H*').first}"
  ctx.wipe
end
```

**Install FFI and run:**
```bash
gem install ffi
ruby xzalgochain.rb
```

---

## Java

### Using JNI
```java
// XzalgoChain.java
public class XzalgoChain {
    public static final int HASH_SIZE = 40;
    
    static {
        // Load native library
        String os = System.getProperty("os.name").toLowerCase();
        String libName;
        
        if (os.contains("win")) {
            libName = "XzalgoChain.dll";
        } else if (os.contains("mac")) {
            libName = "libXzalgoChain.dylib";
        } else {
            libName = "libXzalgoChain.so";
        }
        
        System.loadLibrary(libName.replace("lib", "").replace(".dll", "").replace(".so", "").replace(".dylib", ""));
    }
    
    // Native method declarations
    public static native void xzalgochain_lib(byte[] data, long len, byte[] output);
    public static native String xzalgochain_version_lib();
    public static native String xzalgochain_platform_info_lib();
    public static native String xzalgochain_get_simd_name_lib();
    public static native int xzalgochain_get_simd_type_lib();
    public static native int xzalgochain_avx2_supported_lib();
    public static native int xzalgochain_neon_supported_lib();
    public static native int xzalgochain_equals_lib(byte[] h1, byte[] h2);
    
    // Context handling
    public static class Context {
        private long nativeHandle; // Will hold pointer to C context
        
        public Context() {
            init();
        }
        
        private native void init();
        public native void update(byte[] data, long len);
        public native byte[] finalize();
        public native void reset();
        public native void wipe();
        
        @Override
        protected void finalize() throws Throwable {
            wipe();
            super.finalize();
        }
    }
    
    // Convenience methods
    public static byte[] hash(byte[] data) {
        byte[] output = new byte[HASH_SIZE];
        xzalgochain_lib(data, data.length, output);
        return output;
    }
    
    public static String hashHex(String data) {
        try {
            byte[] hash = hash(data.getBytes("UTF-8"));
            StringBuilder sb = new StringBuilder();
            for (byte b : hash) {
                sb.append(String.format("%02x", b & 0xff));
            }
            return sb.toString();
        } catch (Exception e) {
            return null;
        }
    }
    
    public static boolean equals(byte[] h1, byte[] h2) {
        if (h1.length != HASH_SIZE || h2.length != HASH_SIZE)
            return false;
        return xzalgochain_equals_lib(h1, h2) != 0;
    }
    
    public static void main(String[] args) {
        System.out.println("XzalgoChain " + xzalgochain_version_lib());
        System.out.println("Platform: " + xzalgochain_platform_info_lib());
        System.out.println("SIMD: " + xzalgochain_get_simd_name_lib() + 
                           " (type " + xzalgochain_get_simd_type_lib() + ")");
        System.out.println("AVX2: " + (xzalgochain_avx2_supported_lib() != 0));
        System.out.println("NEON: " + (xzalgochain_neon_supported_lib() != 0));
        
        // Single-shot hash
        String hash = hashHex("Hello World");
        System.out.println("\nHash of 'Hello World': " + hash);
    }
}
```

**JNI C wrapper (xzalgochain_jni.c):**
```c
#include <jni.h>
#include <string.h>
#include "XzalgoChain/XzalgoChain.h"
#include "XzalgoChain.h" // Generated by javac -h

JNIEXPORT void JNICALL Java_XzalgoChain_xzalgochain_1lib
(JNIEnv *env, jclass cls, jbyteArray data, jlong len, jbyteArray output) {
    jbyte *dataPtr = (*env)->GetByteArrayElements(env, data, NULL);
    jbyte *outPtr = (*env)->GetByteArrayElements(env, output, NULL);
    
    xzalgochain_lib((uint8_t*)dataPtr, (size_t)len, (uint8_t*)outPtr);
    
    (*env)->ReleaseByteArrayElements(env, data, dataPtr, JNI_ABORT);
    (*env)->ReleaseByteArrayElements(env, output, outPtr, 0);
}

JNIEXPORT jstring JNICALL Java_XzalgoChain_xzalgochain_1version_1lib
(JNIEnv *env, jclass cls) {
    return (*env)->NewStringUTF(env, xzalgochain_version_lib());
}

JNIEXPORT jstring JNICALL Java_XzalgoChain_xzalgochain_1platform_1info_1lib
(JNIEnv *env, jclass cls) {
    return (*env)->NewStringUTF(env, xzalgochain_platform_info_lib());
}

JNIEXPORT jstring JNICALL Java_XzalgoChain_xzalgochain_1get_1simd_1name_1lib
(JNIEnv *env, jclass cls) {
    return (*env)->NewStringUTF(env, xzalgochain_get_simd_name_lib());
}

JNIEXPORT jint JNICALL Java_XzalgoChain_xzalgochain_1get_1simd_1type_1lib
(JNIEnv *env, jclass cls) {
    return (jint)xzalgochain_get_simd_type_lib();
}

JNIEXPORT jint JNICALL Java_XzalgoChain_xzalgochain_1avx2_1supported_1lib
(JNIEnv *env, jclass cls) {
    return (jint)xzalgochain_avx2_supported_lib();
}

JNIEXPORT jint JNICALL Java_XzalgoChain_xzalgochain_1neon_1supported_1lib
(JNIEnv *env, jclass cls) {
    return (jint)xzalgochain_neon_supported_lib();
}

JNIEXPORT jint JNICALL Java_XzalgoChain_xzalgochain_1equals_1lib
(JNIEnv *env, jclass cls, jbyteArray h1, jbyteArray h2) {
    jbyte *h1Ptr = (*env)->GetByteArrayElements(env, h1, NULL);
    jbyte *h2Ptr = (*env)->GetByteArrayElements(env, h2, NULL);
    
    jint result = (jint)xzalgochain_equals_lib((uint8_t*)h1Ptr, (uint8_t*)h2Ptr);
    
    (*env)->ReleaseByteArrayElements(env, h1, h1Ptr, JNI_ABORT);
    (*env)->ReleaseByteArrayElements(env, h2, h2Ptr, JNI_ABORT);
    
    return result;
}

// ... other JNI implementations for Context class
```

**Compile and run:**
```bash
# Generate JNI headers
javac -h . XzalgoChain.java

# Compile JNI wrapper
gcc -shared -fpic -o libxzalgochain_jni.so \
    -I"$JAVA_HOME/include" \
    -I"$JAVA_HOME/include/linux" \
    -L. -lXzalgoChain \
    xzalgochain_jni.c

# Run Java
java -Djava.library.path=. XzalgoChain
```

---

## Build Configuration Examples

### CMake for C/C++ projects
```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(MyProject)

# Find XzalgoChain
find_path(XZALGOCHAIN_INCLUDE_DIR NAMES XzalgoChain/XzalgoChain.h)
find_library(XZALGOCHAIN_LIBRARY NAMES XzalgoChain)

if(NOT XZALGOCHAIN_INCLUDE_DIR OR NOT XZALGOCHAIN_LIBRARY)
    message(FATAL_ERROR "XzalgoChain not found")
endif()

# Add executable
add_executable(myapp main.c)
target_include_directories(myapp PRIVATE ${XZALGOCHAIN_INCLUDE_DIR})
target_link_libraries(myapp PRIVATE ${XZALGOCHAIN_LIBRARY})

# Or use package config
find_package(XzalgoChain REQUIRED)
target_link_libraries(myapp PRIVATE XzalgoChain::XzalgoChain)
```

### pkg-config
```bash
# Check if XzalgoChain is installed
pkg-config --modversion XzalgoChain

# Get compiler flags
pkg-config --cflags XzalgoChain

# Get linker flags
pkg-config --libs XzalgoChain

# Use in Makefile
CFLAGS += $(shell pkg-config --cflags XzalgoChain)
LDFLAGS += $(shell pkg-config --libs XzalgoChain)
```

### Package config file (XzalgoChain.pc)
```pkgconfig
prefix=/usr/local
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
includedir=${prefix}/include

Name: XzalgoChain
Description: 320-bit Cryptographic Hash Function
Version: 0.0.1.2
Libs: -L${libdir} -lXzalgoChain
Cflags: -I${includedir}
```

---

## Notes

1. **Function naming**: All exported functions use the `_lib` suffix to avoid naming conflicts with internal implementations.

2. **Library naming**: Adjust library names according to your platform:
   - Linux: `libXzalgoChain.so`
   - macOS: `libXzalgoChain.dylib`
   - Windows: `XzalgoChain.dll` or `XzalgoChain.lib`

3. **Constants**: Always use `XZALGOCHAIN_HASH_SIZE` (40) for hash buffer sizes.

4. **Memory security**: Always call `xzalgochain_ctx_wipe_lib()` on contexts when done to clear sensitive data.

5. **Linking**: When linking statically, ensure all dependencies are included:
   ```bash
   # Static linking with OpenMP
   gcc -static -o example example.c -I. -L. -lXzalgoChain -fopenmp
   ```

6. **SIMD detection**: Always check available SIMD at runtime:
   ```c
   if (xzalgochain_avx2_supported_lib()) {
       // AVX2 optimized path available
   }
   ```

7. **Thread safety**: The library is thread-safe when each thread uses its own context.

8. **Error handling**: Functions generally don't return error codes - ensure valid inputs and sufficient buffer sizes.

9. **Version checking**: Use `xzalgochain_version_lib()` to verify compatibility at runtime.

10. **Platform info**: `xzalgochain_platform_info_lib()` provides detailed platform detection for debugging.
