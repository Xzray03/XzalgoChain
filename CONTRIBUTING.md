# Contributing to XzalgoChain

Thank you for your interest in contributing to XzalgoChain! This document outlines the guidelines and requirements for contributions to ensure the hash function maintains its cryptographic strength and produces consistent results across all platforms.

## Core Principles

### 1. Hash Consistency
**The hash output must be identical across all implementations and platforms.**

- The same input must always produce the same 320-bit output
- Results must be reproducible across:
  - Different architectures (x86, ARM, etc.)
  - Different SIMD implementations (AVX2, NEON, scalar)
  - Different compilers and optimization levels
  - Different operating systems
  - Different endianness (little/big-endian)

### 2. Security First
**Never compromise cryptographic security for performance or convenience.**

- All operations must maintain the designed security margin
- No shortcuts that could weaken resistance to attacks
- Constant-time operations for security-critical paths
- No data-dependent branches or memory access patterns

### 3. Code Quality
**Maintain high standards of code quality and maintainability.**

- Clear, readable, and well-documented code
- Comprehensive test coverage
- No undefined behavior
- Strict adherence to language standards (C11)

## Contribution Types

### 1. Optimizations
Performance improvements while maintaining identical output:

- **Allowed**: Better instruction selection, reduced instruction count
- **Allowed**: Improved memory access patterns, better cache utilization
- **Allowed**: Compiler-specific optimizations with fallbacks
- **Not Allowed**: Algorithmic changes that alter output
- **Not Allowed**: Skipping security-critical operations
- **Not Allowed**: Introducing platform-specific output differences

### 2. Porting
Adding support for new platforms:

- **Required**: Exact output match with reference implementation
- **Required**: Endianness handling
- **Required**: Alignment considerations
- **Required**: Comprehensive testing on target platform
- **Bonus**: SIMD optimizations where available

### 3. Bug Fixes
Fixing issues while maintaining security:

- **Documented**: Clear explanation of the issue
- **Tested**: Regression tests added
- **Verified**: No security implications
- **Reviewed**: Security impact assessment

### 4. Documentation
Improving clarity and completeness:

- **Clear**: Easy to understand for various skill levels
- **Accurate**: Technically correct and up-to-date
- **Comprehensive**: Covers both usage and internals
- **Examples**: Practical, tested examples

## Development Workflow

### 1. Issue First
Before writing code, create an issue describing:
- The problem or improvement
- Why it's needed
- Potential approach
- Security implications (if any)

### 2. Fork and Branch
- Fork the repository
- Create a feature branch: `feature/description` or `fix/issue-number`
- Keep changes focused on a single purpose

### 3. Development Guidelines

#### Code Style
```c
/* Function names: snake_case, descriptive */
static inline uint64_t calculate_round_constant(int round) {
    /* Indent with 4 spaces, no tabs */
    /* Opening brace on same line */
    if (condition) {
        /* Comments explain why, not what */
        operation1();
        operation2();
    }
    
    return result;
}

/* Type definitions: _t suffix, descriptive */
typedef struct {
    uint64_t lane[4];  /* Comments explain fields */
} vec256_t;

/* Constants: UPPER_CASE with descriptive names */
#define ROTATION_CONSTANT_17 17
```

#### Required Elements
Every function must include:
- Purpose description
- Parameter documentation
- Return value documentation
- Any side effects or thread safety notes
- References to algorithm specification where relevant

Example:
```c
/**
 * Apply ARX mixing to a 256-bit vector
 * 
 * Core mixing function combining addition, rotation, XOR, and lane mixing.
 * Must produce identical results across all implementations.
 * 
 * @param v Input vector to mix
 * @param salt Salt vector for entropy injection
 * @param rc Round constant vector
 * @param r1 First rotation amount (must be 0-63)
 * @param r2 Second rotation amount (must be 0-63)
 * @return Mixed vector with all lanes diffused
 * 
 * @note This is a critical security function - modifications require
 *       extensive security review and avalanche testing
 * @thread_safe Yes (operates on local data only)
 */
static inline vec256_t arx_mix_vector(vec256_t v, vec256_t salt, 
                                      vec256_t rc, int r1, int r2);
```

### 4. Testing Requirements

#### Mandatory Tests for ALL Changes
```bash
# 1. Build with all configurations
cd tests
make

# 2. Run test suite
cd bin

# 3. Verify hash consistency across platforms
./consistent_test    # Must match reference outputs

# 4. Run security tests
./avalanche_test
./sac_test
./bic_test
./differential_test

# 5. Performance benchmarks (for optimization contributions)
./benchmark
```

#### Test Categories

| Test | Purpose | Minimum Requirement |
|------|---------|---------------------|
| **Unit Tests** | Individual function correctness | 100% pass |
| **Vector Tests** | Known answer tests | Match reference exactly |
| **Avalanche** | Bit change propagation | >50% average |
| **SAC** | Strict Avalanche Criterion | Meet thresholds |
| **BIC** | Bit Independence Criterion | No correlations |
| **Entropy** | Output distribution | Near-ideal |
| **Benchmark** | Performance measurement | No regression* |

*Optimizations must show improvement, bug fixes may have minimal regression if justified

### 5. Security Review Checklist

For any change affecting the algorithm:

- [ ] Does output match reference implementation for all test vectors?
- [ ] Are there any new branching operations that depend on secret data?
- [ ] Are there any new memory accesses that could leak information?
- [ ] Has avalanche effect been measured and verified (~50%)?
- [ ] Have SAC and BIC tests been run with no failures?
- [ ] Has differential cryptanalysis resistance been verified?
- [ ] Have all constant-time properties been maintained?
- [ ] Has the change been reviewed by maintainers?

### 6. Pull Request Process

#### PR Template
```
## Description
Brief description of the change

## Type of Change
- [ ] Bug fix
- [ ] New feature/optimization
- [ ] Documentation
- [ ] Port to new platform

## Impact on Output
- [ ] No change in hash output
- [ ] Change in hash output (requires justification)

## Testing Performed
- [ ] Unit tests
- [ ] Consistent tests
- [ ] Avalanche tests
- [ ] SAC/BIC tests
- [ ] Performance benchmarks

## Security Implications
Describe any security considerations:

## Platform Testing
- [ ] x86/x64
- [ ] ARM
- [ ] Other: __________

## Checklist
- [ ] Code follows style guidelines
- [ ] Documentation updated
- [ ] Tests added/updated
- [ ] All tests passing
- [ ] Security review completed
- [ ] No regression in output consistency
```

## Performance Optimization Guidelines

### Acceptable Optimizations
- Better instruction selection (e.g., using SIMD where beneficial)
- Reduced instruction count while maintaining exact operations
- Improved memory layout for better cache utilization
- Compiler hints (`__restrict`, `__builtin_expect`, etc.)
- Platform-specific intrinsics with fallbacks

### Unacceptable Optimizations
- Changing operation order that could affect output
- Removing operations considered "redundant" but security-critical
- Introducing platform-specific output differences
- Using non-constant-time operations in security paths

## Platform-Specific Guidelines

### x86/x64 (AVX2)
```c
/* Always provide scalar fallback */
#ifdef __AVX2__
    /* AVX2 optimized version */
    result = avx2_implementation(input);
#else
    /* Scalar fallback */
    result = scalar_implementation(input);
#endif
```

### ARM (NEON)
```c
/* Handle both 32-bit and 64-bit ARM */
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    /* NEON implementation */
    result = neon_implementation(input);
#else
    /* Scalar fallback */
    result = scalar_implementation(input);
#endif
```

### Endianness
```c
/* Always handle endianness explicitly */
#if XZALGOCHAIN_BIG_ENDIAN
    /* Convert from big-endian to little-endian */
    value = byteswap64(raw_value);
#else
    /* Direct use on little-endian */
    value = raw_value;
#endif
```

## Documentation Requirements

### Code Comments
- Document non-obvious security considerations
- Reference algorithm specification sections
- Note any platform-specific behaviors

### User Documentation
- Update README.md for user-facing changes
- Update ALGORITHM.md for algorithm changes
- Update TEST_(yourname).md for new test results
- Document new API functions

### Test Documentation
- Describe what each test verifies
- Document expected results
- Note any platform-specific test requirements

## Communication

### Before Contributing
- Search existing issues and PRs
- Discuss major changes in an issue first
- Ask questions if anything is unclear

### During Development
- Keep PRs focused and manageable in size
- Respond to review comments promptly
- Be open to feedback and suggestions

### After Merging
- Monitor for any regression reports
- Help with follow-up improvements
- Document lessons learned

## Review Process

### Stage 1: Automated Checks
- Builds on all supported platforms
- All tests pass
- No warnings
- Code style compliance

### Stage 2: Code Review
- At least two maintainers review
- Focus on correctness and security
- Verify output consistency
- Check for undefined behavior

### Stage 3: Security Review
- Cryptographic review for algorithm changes
- Constant-time verification
- Side-channel analysis
- Attack surface assessment

### Stage 4: Integration
- Merge to development branch
- Extended testing period
- Performance regression testing
- Final security audit

## Recognition

Contributors will be recognized in:
- CONTRIBUTORS.md file
- Release notes
- Documentation credits

Significant contributions may include:
- Algorithm improvements (with security review)
- New platform ports
- Major performance optimizations
- Comprehensive testing infrastructure

## Questions?

If you have questions about contributing:
- Open a discussion issue
- Ask in code review comments
- Contact maintainers directly

Remember: **Hash consistency and security are paramount.** Every contribution must maintain these properties above all else.

---

Thank you for helping make XzalgoChain better and more secure!
