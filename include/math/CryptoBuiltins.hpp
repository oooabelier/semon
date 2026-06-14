#ifndef SEMON_MATH_CRYPTO_BUILTINS_HPP
#define SEMON_MATH_CRYPTO_BUILTINS_HPP

#include <cstddef>
#include <cstdint>

#include "core/PagedAllocator.hpp"
#include "math/MathModel.hpp"
#include "math/MathTypes.hpp"
#include "circuit/CircuitTopology.hpp"
#include "math/ModularArithmetic.hpp"

namespace semon {
namespace math {

class CryptoBuiltins {
public:
    CryptoBuiltins() = delete;

    // MD5: 4 rounds, 64 steps, 512-bit blocks
    static bool buildMD5(const MathModel& model, const char* input_var, const char* output_prefix,
                         circuit::Circuit& target, core::PagedAllocator* allocator);

    // SHA-256: 64 rounds, 512-bit blocks, 256-bit output
    static bool buildSHA256(const MathModel& model, const char* input_var, const char* output_prefix,
                            circuit::Circuit& target, core::PagedAllocator* allocator);

    // SHA-512: 80 rounds, 1024-bit blocks, 512-bit output
    static bool buildSHA512(const MathModel& model, const char* input_var, const char* output_prefix,
                            circuit::Circuit& target, core::PagedAllocator* allocator);

    // SHA-3 / Keccak-f[1600]: 24 rounds, 1600-bit state
    static bool buildSHA3(const MathModel& model, const char* input_var, const char* output_prefix,
                          circuit::Circuit& target, core::PagedAllocator* allocator);

    // AES-128/192/256: KeyExpansion + 10/12/14 rounds
    static bool buildAES(const MathModel& model, const char* key_var, const char* data_var,
                         const char* output_prefix, std::uint32_t key_bits,
                         circuit::Circuit& target, core::PagedAllocator* allocator);

    // Modular exponentiation: result = base^exp mod modulus (up to 65536 bits)
    static bool buildMODEXP(const MathModel& model, const char* base_var, const char* exp_var,
                            const char* mod_var, const char* output_var,
                            circuit::Circuit& target, core::PagedAllocator* allocator);

    // ECC point addition on NIST prime curves (P-256, P-384, P-521)
    static bool buildECCAdd(const MathModel& model, const char* p1_var, const char* p2_var,
                            const char* curve_params, const char* output_var,
                            circuit::Circuit& target, core::PagedAllocator* allocator);

    // ECC scalar multiplication
    static bool buildECCMul(const MathModel& model, const char* scalar_var, const char* point_var,
                            const char* curve_params, const char* output_var,
                            circuit::Circuit& target, core::PagedAllocator* allocator);

    // ECDSA verification: full verification loop as constraints
    static bool buildECDSAVerify(const MathModel& model, const char* msghash_var,
                                 const char* sig_var, const char* pubkey_var,
                                 const char* curve_params, const char* output_var,
                                 circuit::Circuit& target, core::PagedAllocator* allocator);

private:
    // SHA-256 constants (K[0..63])
    static constexpr std::uint32_t SHA256_K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    // MD5 constants (T[1..64] = floor(2^32 * |sin(i)|))
    static constexpr std::uint32_t MD5_T[64] = {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
        0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
        0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
        0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
        0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
        0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
    };

    static constexpr std::uint32_t MD5_S[64] = {
        7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
        5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
        4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
        6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
    };

    // AES S-Box (computed at runtime via composite field)
    static void generateAESSBox(std::uint8_t sbox[256]);
    static void generateAESInvSBox(std::uint8_t inv_sbox[256]);

    // GF(2^8) multiplication for AES MixColumns
    static std::uint8_t gfMul(std::uint8_t a, std::uint8_t b);

    // SHA-256 Sigma functions
    static std::uint32_t sigma0_256(std::uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
    static std::uint32_t sigma1_256(std::uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
    static std::uint32_t Sigma0_256(std::uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
    static std::uint32_t Sigma1_256(std::uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }
    static std::uint32_t Ch(std::uint32_t x, std::uint32_t y, std::uint32_t z) { return (x & y) ^ (~x & z); }
    static std::uint32_t Maj(std::uint32_t x, std::uint32_t y, std::uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }

    static std::uint32_t rotr(std::uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }
    static std::uint64_t rotr64(std::uint64_t x, int n) { return (x >> n) | (x << (64 - n)); }
};

} // namespace math
} // namespace semon

#endif
