/*
 * XzalgoChain - 320-bit Cryptographic Hash Function
 * Copyright 2026 Xzrayツ
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Standard C library headers for basic I/O, memory management, and string operations */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

/* Platform detection and conditional includes
 * Each platform block defines platform-specific macros and includes necessary headers
 * This ensures cross-platform compatibility for file operations and command-line parsing
 */

/* Android platform */
#if defined(__ANDROID__)
    #define PLATFORM_ANDROID 1
    #include <unistd.h>      /* POSIX operating system API */
    #include <getopt.h>       /* Command-line option parsing */

/* Linux platform */
#elif defined(__linux__)
    #define PLATFORM_LINUX 1
    #include <unistd.h>
    #include <getopt.h>

/* macOS platform (including Apple's variation of BSD) */
#elif defined(__APPLE__) || defined(__MACH__)
    #define PLATFORM_MACOS 1
    #include <unistd.h>
    #include <getopt.h>
    #include <sys/param.h>    /* System parameters and limits */

/* FreeBSD platform */
#elif defined(__FreeBSD__)
    #define PLATFORM_FREEBSD 1
    #include <unistd.h>
    #include <getopt.h>
    #include <sys/param.h>

/* Windows platform (various possible macros) */
#elif defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
    #define PLATFORM_WINDOWS 1
    #define _CRT_SECURE_NO_WARNINGS  /* Disable Microsoft security warnings */
    #include <windows.h>               /* Windows API */
    #include <io.h>                     /* Low-level I/O functions */
    #include <fcntl.h>                  /* File control options */
    #ifdef _MSC_VER
        #include "getopt_win.h"         /* Windows-specific getopt for MSVC */
    #else
        #include <unistd.h>
        #include <getopt.h>
    #endif

/* iOS platform */
#elif defined(__IOS__) || defined(TARGET_OS_IPHONE) || defined(TARGET_IPHONE_SIMULATOR)
    #define PLATFORM_IOS 1
    #include <unistd.h>
    #include <getopt.h>
    #include <sys/param.h>

/* Unknown/other platform */
#else
    #define PLATFORM_UNKNOWN 1
    #include <unistd.h>
    #include <getopt.h>
#endif

/* Windows-specific implementation of fmemopen
 * Since Windows doesn't have fmemopen, we implement a custom memory stream
 * using funopen (available in some Windows environments) or custom functions
 */
#ifdef PLATFORM_WINDOWS
    #define STDIN_FILENO 0  /* Standard input file descriptor number */

    #include <stdio.h>
    
    /* Memory stream structure for Windows fmemopen emulation */
    typedef struct {
        unsigned char *buffer;  /* Pointer to memory buffer */
        size_t size;            /* Size of buffer */
        size_t position;        /* Current read position */
    } mem_stream_t;
    
    /* Read function for memory stream */
    static int mem_stream_read(void *ctx, char *buf, int size) {
        mem_stream_t *ms = (mem_stream_t *)ctx;
        int bytes_to_read = size;
        
        /* Ensure we don't read beyond buffer bounds */
        if (ms->position + bytes_to_read > ms->size)
            bytes_to_read = ms->size - ms->position;
        
        if (bytes_to_read > 0) {
            memcpy(buf, ms->buffer + ms->position, bytes_to_read);
            ms->position += bytes_to_read;
        }
        
        return bytes_to_read;
    }
    
    /* Seek function for memory stream */
    static int mem_stream_seek(void *ctx, long offset, int whence) {
        mem_stream_t *ms = (mem_stream_t *)ctx;
        size_t new_pos;
        
        /* Calculate new position based on whence */
        switch (whence) {
            case SEEK_SET:
                new_pos = offset;
                break;
            case SEEK_CUR:
                new_pos = ms->position + offset;
                break;
            case SEEK_END:
                new_pos = ms->size + offset;
                break;
            default:
                return -1;
        }
        
        /* Validate new position */
        if (new_pos > ms->size)
            return -1;
        
        ms->position = new_pos;
        return 0;
    }
    
    /* Windows emulation of fmemopen using funopen */
    static FILE *fmemopen_win(void *buf, size_t size, const char *mode) {
        mem_stream_t *ms = (mem_stream_t *)malloc(sizeof(mem_stream_t));
        if (!ms)
            return NULL;
        
        ms->buffer = (unsigned char *)buf;
        ms->size = size;
        ms->position = 0;
        
        /* funopen creates a FILE stream from custom I/O functions */
        return funopen(ms, mem_stream_read, NULL, mem_stream_seek, NULL);
    }
#else
    /* For non-Windows platforms, use standard fmemopen from stdio.h */
    #include <stdio.h>
#endif

/* Include the XzalgoChain implementation header
 * XZALGOCHAIN_IMPLEMENTATION macro ensures the function bodies are included
 */
#define XZALGOCHAIN_IMPLEMENTATION
#include "XzalgoChain/XzalgoChain.h"

/* Buffer size for reading files/streams - 16KB for efficient I/O */
#define BUFFER_SIZE 16384

/* Global verbosity and quiet mode flags */
static int verbose_mode = 0;  /* Enable detailed output */
static int quiet_mode   = 0;  /* Suppress normal output */

/**
 * Get human-readable platform name based on detected macros
 * @return String containing platform name
 */
static const char* get_platform_name(void) {
    #if defined(PLATFORM_LINUX)
        return "Linux";
    #elif defined(PLATFORM_MACOS)
        return "macOS";
    #elif defined(PLATFORM_FREEBSD)
        return "FreeBSD";
    #elif defined(PLATFORM_ANDROID)
        return "Android";
    #elif defined(PLATFORM_IOS)
        return "iOS";
    #elif defined(PLATFORM_WINDOWS)
        return "Windows";
    #else
        return "Unknown";
    #endif
}

/**
 * Print brief usage information
 * @param prog Program name (argv[0])
 */
static void print_usage(const char *prog) {
    /* Extract program name from full path */
    const char *prog_name = prog;
    #ifdef PLATFORM_WINDOWS
        const char *last_slash = strrchr(prog, '\\');
        if (last_slash)
            prog_name = last_slash + 1;
    #else
        const char *last_slash = strrchr(prog, '/');
        if (last_slash)
            prog_name = last_slash + 1;
    #endif
    
    fprintf(stderr,
            "Usage: %s [OPTIONS] [FILE]\n"
            "Try '%s -h' for help.\n",
            prog_name, prog_name);
}

/**
 * Print detailed help information with usage examples
 * @param prog Program name (argv[0])
 */
static void print_help(const char *prog) {
    /* Extract program name from full path */
    const char *prog_name = prog;
    #ifdef PLATFORM_WINDOWS
        const char *last_slash = strrchr(prog, '\\');
        if (last_slash)
            prog_name = last_slash + 1;
    #else
        const char *last_slash = strrchr(prog, '/');
        if (last_slash)
            prog_name = last_slash + 1;
    #endif
    
    printf("XzalgoChain 320-bit hash utility (Version%.8s)\n\n", xzalgochain_version() + 11);
    printf("Platform: %s\n\n", get_platform_name());
    printf("Usage: %s [OPTIONS] [FILE]\n\n", prog_name);

    /* Operation modes explanation */
    printf("Modes:\n");
    printf("  RAW:\n");
    printf("    %s\n", prog_name);
    printf("    Reads data from standard input (stdin).\n\n");

    printf("  File:\n");
    printf("    %s file.txt\n", prog_name);
    printf("    Opens file and streams its contents internally.\n\n");

    printf("  String:\n");
    printf("    %s -i \"text\"\n", prog_name);
    printf("    Hashes the exact bytes of the provided string.\n\n");

    printf("  Check:\n");
    printf("    %s -c HASH [FILE|-i \"text\"]\n", prog_name);
    printf("    Verifies computed hash against HASH.\n");
    printf("    If no FILE or -i is provided, stdin is used.\n\n");

    /* Standard input explanation */
    printf("Using stdin (Standard Input):\n");
    printf("  stdin allows data to be piped or redirected into the program.\n");
    printf("  The hash is computed over the exact byte stream received.\n\n");

    printf("  Examples:\n");
    #ifdef PLATFORM_WINDOWS
        printf("    echo Hello | %s\n", prog_name);
        printf("    type file.txt | %s\n\n", prog_name);
    #else
        printf("    echo -n \"Hello\" | %s\n", prog_name);
        printf("    printf \"Hello\" | %s\n", prog_name);
        printf("    %s < file.txt\n", prog_name);
        printf("    cat file.txt | %s\n\n", prog_name);
    #endif

    /* Important notes about input handling */
    printf("  Important:\n");
    #ifdef PLATFORM_WINDOWS
        printf("    'echo' in Windows always appends a newline (CR+LF).\n");
    #else
        printf("    'echo' without -n appends a newline (\\n).\n");
    #endif
    printf("    This changes the hashed bytes and therefore the result.\n");
    printf("    The utility never modifies input data.\n\n");

    /* Command-line options */
    printf("Options:\n");
    printf("  -i STRING   Hash string\n");
    printf("  -c HASH     Check mode\n");
    printf("  -f          Force scalar mode (disable SIMD)\n");
    printf("  -q          Quiet\n");
    printf("  -v          Version\n");
    printf("  -V          Verbose\n");
    printf("  -h          Help\n");
}

/**
 * Print version and system information
 * Displays platform, architecture, SIMD support status
 */
static void print_version(void) {
    int simd_type = xzalgochain_get_simd_type();
    const char *simd_name = "None";
    int avx2_detected = 0;
    int neon_detected = 0;
    int force_seq = xzalgochain_is_forced_scalar();

    /* Detect SIMD support based on architecture */
    if (xzalgochain_is_x86()) {
        avx2_detected = xzalgochain_avx2_supported();
    }

    if (xzalgochain_is_arm()) {
        neon_detected = xzalgochain_neon_supported();
    }

    /* Get SIMD type name */
    if (simd_type == SIMD_AVX2) {
        simd_name = "AVX2";
    } else if (simd_type == SIMD_NEON) {
        simd_name = "NEON";
    }

    /* Print all version and system information */
    printf("%s\n", xzalgochain_version());
    printf("Platform: %s (%s)\n", get_platform_name(), xzalgochain_platform_info());
    printf("Architecture: %s\n", xzalgochain_is_64bit() ? "64-bit" : "32-bit");
    printf("CPU Type: %s\n", xzalgochain_is_x86() ? "x86" : (xzalgochain_is_arm() ? "ARM" : "Unknown"));

    if (xzalgochain_is_x86()) {
        printf("AVX2 Support: %s\n", avx2_detected ? "Yes" : "No");
    }

    if (xzalgochain_is_arm()) {
        printf("NEON Support: %s\n", neon_detected ? "Yes" : "No");
    }

    printf("Active SIMD: %s (Type %d)\n", simd_name, simd_type);
    printf("Force Scalar: %s\n", force_seq ? "Yes" : "No");
}

/**
 * Verbose output function (printf-style)
 * Only prints if verbose mode is enabled and quiet mode is disabled
 */
static void verbose(const char *fmt, ...) {
    if (verbose_mode && !quiet_mode) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
}

/**
 * Read from a stream and compute its hash
 * @param fp FILE pointer to read from
 * @param desc Description of input source (for verbose output)
 * @param hash Output buffer for computed hash (must be XZALGOCHAIN_HASH_SIZE bytes)
 * @return 0 on success, -1 on error
 */
static int hash_stream(FILE *fp, const char *desc, uint8_t *hash) {
    XzalgoChain_CTX ctx;          /* Hash context */
    uint8_t buffer[BUFFER_SIZE];   /* Read buffer */
    size_t total = 0;              /* Total bytes read (for verbose output) */

    /* Initialize the hash context */
    xzalgochain_init(&ctx);

    /* Read and process data in chunks */
    while (1) {
        size_t r = fread(buffer, 1, BUFFER_SIZE, fp);

        if (r > 0) {
            xzalgochain_update(&ctx, buffer, r);
            total += r;
            verbose("Read %zu bytes from %s\r",
                    total, desc ? desc : "stdin");
        }

        /* Check for read errors or EOF */
        if (r < BUFFER_SIZE) {
            if (ferror(fp)) {
                if (!quiet_mode) {
                    fprintf(stderr,
                            "Error reading %s: %s\n",
                            desc ? desc : "stdin",
                            strerror(errno));
                }

                /* Clear sensitive data from context on error */
                xzalgochain_ctx_wipe(&ctx);
                return -1;
            }
            break;  /* EOF reached */
        }
    }

    /* Finalize hash computation */
    xzalgochain_final(&ctx, hash);
    /* Wipe context to clear sensitive data */
    xzalgochain_ctx_wipe(&ctx);

    if (verbose_mode && !quiet_mode)
        fprintf(stderr, "\n");

    return 0;
}

/**
 * Open input stream based on provided arguments
 * @param filename File to open (or NULL)
 * @param string_input String to hash (or NULL)
 * @param label_out Output parameter for input description
 * @return FILE pointer to read from, or NULL on error
 */
static FILE *open_input_stream(const char *filename,
                               const char *string_input,
                               const char **label_out)
{
    /* String input mode */
    if (string_input) {
        *label_out = string_input;

        /* Use fmemopen (or Windows emulation) to create stream from string */
        #ifdef PLATFORM_WINDOWS
        return fmemopen_win((void *)string_input,
                            strlen(string_input),
                            "rb");
        #else
        return fmemopen((void *)string_input,
                        strlen(string_input),
                        "rb");
        #endif
    }

    /* File input mode */
    if (filename) {
        /* Windows Unicode path support */
        #ifdef PLATFORM_WINDOWS
        wchar_t wfilename[MAX_PATH];
        int wlen = MultiByteToWideChar(CP_UTF8, 0, filename, -1, wfilename, MAX_PATH);
        if (wlen > 0) {
            FILE *fp = _wfopen(wfilename, L"rb");
            if (fp) {
                *label_out = filename;
                return fp;
            }
        }
        #endif

        /* Standard file open (fallback for Windows, primary for others) */
        FILE *fp = fopen(filename, "rb");
        if (!fp)
            return NULL;
        *label_out = filename;
        return fp;
    }

    /* Standard input mode */
    *label_out = "stdin";

    #ifdef PLATFORM_WINDOWS
    /* Set stdin to binary mode on Windows to avoid CR/LF translation */
    _setmode(_fileno(stdin), _O_BINARY);
    #endif

    return stdin;
}

/**
 * Print hash in hexadecimal format
 * @param hash Hash bytes to print
 * @param label Input description (file, string, or stdin)
 */
static void print_hash(const uint8_t *hash, const char *label) {
    /* Print each byte as two hex digits */
    for (int i = 0; i < XZALGOCHAIN_HASH_SIZE; ++i)
        printf("%02x", hash[i]);

    /* Format output based on input type */
    if (label) {
        if (strcmp(label, "stdin") == 0) {
            printf("\n");
        } else if (strchr(label, '/') || strchr(label, '\\')) {
            printf("  %s\n", label);
        } else {
            printf("  \"%s\"\n", label);
        }
    } else {
        printf("\n");
    }
}

/**
 * Parse hexadecimal hash string to byte array
 * @param s Input hex string
 * @param hash Output byte array (must be XZALGOCHAIN_HASH_SIZE bytes)
 * @return 0 on success, -1 on invalid format
 */
static int parse_hash(const char *s, uint8_t *hash) {
    size_t len = strlen(s);
    
    /* Handle possible trailing newline */
    if (len == XZALGOCHAIN_HASH_SIZE * 2 + 1 && (s[len-1] == '\n' || s[len-1] == '\r'))
        len--;
    
    /* Validate length (should be exactly twice the hash size) */
    if (len != XZALGOCHAIN_HASH_SIZE * 2)
        return -1;

    /* Parse each pair of hex digits */
    for (size_t i = 0; i < XZALGOCHAIN_HASH_SIZE; ++i) {
        unsigned int b;
        if (sscanf(s + (i * 2), "%02x", &b) != 1)
            return -1;
        hash[i] = (uint8_t)b;
    }

    return 0;
}

/* Windows getopt implementation (if not provided by compiler) */
#ifdef PLATFORM_WINDOWS
#ifndef HAVE_GETOPT
int optind = 1;
int optopt;
char *optarg;

/**
 * Simple getopt implementation for Windows
 * @param argc Argument count
 * @param argv Argument vector
 * @param optstring Option string (e.g., "i:c:qvVhf")
 * @return Option character, or -1 if done, or '?' on error
 */
static int getopt_win(int argc, char * const argv[], const char *optstring) {
    static char *nextchar = NULL;
    
    if (optind >= argc)
        return -1;
    
    if (nextchar == NULL || *nextchar == '\0') {
        nextchar = argv[optind];
        if (nextchar[0] != '-' || nextchar[1] == '\0') {
            return -1;
        }
        nextchar++;
    }
    
    char opt = *nextchar++;
    const char *pos = strchr(optstring, opt);
    
    if (pos == NULL || opt == ':') {
        optopt = opt;
        return '?';
    }
    
    if (pos[1] == ':') {
        if (*nextchar != '\0') {
            optarg = nextchar;
            nextchar = NULL;
        } else if (optind + 1 < argc) {
            optarg = argv[++optind];
            nextchar = NULL;
        } else {
            optopt = opt;
            return ':';
        }
    }
    
    return opt;
}

#define getopt getopt_win
#endif
#endif

/**
 * Main program entry point
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, non-zero on error
 */
int main(int argc, char **argv) {
    int opt;
    const char *check_str = NULL;    /* Hash to check against */
    const char *string_input = NULL; /* String input mode */
    const char *filename = NULL;      /* File input mode */

    #ifdef PLATFORM_WINDOWS
        /* Set stdout to binary mode on Windows to avoid output corruption */
        _setmode(_fileno(stdout), _O_BINARY);
    #endif

    /* Parse command-line options */
    while ((opt = getopt(argc, argv, "i:c:qvVhf")) != -1) {
        switch (opt) {
            case 'i': string_input = optarg; break;  /* String input */
            case 'c': check_str = optarg; break;     /* Check mode */
            case 'q': quiet_mode = 1; break;          /* Quiet mode */
            case 'v': print_version(); return 0;      /* Version info */
            case 'V': verbose_mode = 1; break;        /* Verbose mode */
            case 'h': print_help(argv[0]); return 0;  /* Help */
            case 'f':                                   /* Force scalar mode */
                xzalgochain_force_scalar(1);
                if (verbose_mode) fprintf(stderr, "Force scalar mode enabled\n");
                break;
            case '?':
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    /* Get filename from remaining arguments */
    if (optind < argc)
        filename = argv[optind];

    /* Validate that filename and string input are mutually exclusive */
    if (filename && string_input) {
        print_usage(argv[0]);
        return 1;
    }

    /* Open input stream based on arguments */
    const char *label = NULL;
    FILE *input = open_input_stream(filename, string_input, &label);

    if (!input) {
        if (!quiet_mode)
            fprintf(stderr, "Cannot open input: %s\n", strerror(errno));
        return 1;
    }

    /* Buffer for computed hash */
    uint8_t hash[XZALGOCHAIN_HASH_SIZE];

    /* Compute hash from input stream */
    if (hash_stream(input, label, hash) != 0) {
        if (input != stdin)
            fclose(input);
        return 1;
    }

    /* Close input if it's not stdin */
    if (input != stdin)
        fclose(input);

    /* Check mode: verify hash against expected value */
    if (check_str) {
        uint8_t expected[XZALGOCHAIN_HASH_SIZE];

        /* Parse expected hash from string */
        if (parse_hash(check_str, expected) != 0) {
            if (!quiet_mode)
                fprintf(stderr, "Invalid hash format\n");
            return 1;
        }

        /* Compare computed hash with expected */
        if (xzalgochain_equals(expected, hash)) {
            if (!quiet_mode)
                printf("%s: OK\n", label);
            return 0;
        } else {
            if (!quiet_mode)
                printf("%s: FAILED\n", label);
            return 1;
        }
    }

    /* Normal mode: output computed hash */
    if (!quiet_mode)
        print_hash(hash, label);

    return 0;
}
