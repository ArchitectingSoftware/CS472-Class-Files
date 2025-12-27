/**
 * =============================================================================
 * CRYPTO-LIB.C - Simple Asymmetric Cipher Library (Implementation)
 * =============================================================================
 * 
 * FILE PURPOSE:
 * This file implements a simple educational asymmetric cipher based on
 * modular multiplicative arithmetic. It's designed to teach cryptography
 * concepts in a network programming context.
 * 
 * ALGORITHM OVERVIEW:
 * The cipher uses modulus 64 arithmetic with a 64-character alphabet.
 * Encryption: C = (P × e) mod 64
 * Decryption: P = (C × d) mod 64
 * Where e and d are multiplicative inverses: (e × d) mod 64 = 1
 * 
 * STUDENTS: You don't need to understand or modify this file!
 * This is provided code that implements the cryptographic operations.
 * Focus on using the functions declared in crypto-lib.h to build your
 * client and server implementations.
 * 
 * However, if you're curious about how the cipher works internally,
 * the comments below explain everything in detail!
 * =============================================================================
 */

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "crypto-lib.h"
#include "protocol.h"


/* =============================================================================
 * CIPHER CONFIGURATION
 * =============================================================================
 */

/**
 * CIPHER_MOD: The modulus for all cipher operations.
 * 
 * We use 64 because:
 * - It's a power of 2 (2^6), making modulo operations efficient
 * - Any odd number 1-63 is coprime with 64 (valid as a key)
 * - 64 characters covers letters, digits, and basic punctuation
 * 
 * This MUST match the size of the alphabet array below.
 */
#define CIPHER_MOD 64

/**
 * THE CIPHER ALPHABET
 * 
 * Maps byte values 0-63 to printable ASCII characters.
 * This defines what characters can be used in encrypted messages.
 * 
 * Index mapping:
 *   0-25:  A-Z (uppercase letters)
 *   26-51: a-z (lowercase letters)
 *   52-61: 0-9 (digits)
 *   62:    ' ' (space)
 *   63:    ',' (comma)
 * 
 * Example: alphabet[7] = 'H', alphabet[30] = 'e'
 * 
 * WHY THESE CHARACTERS?
 * - Covers most common text (letters, numbers, spaces, basic punctuation)
 * - Exactly 64 characters (matches our modulus)
 * - All printable ASCII (easy to debug and display)
 */
static const char alphabet[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',  // 26 letters
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',  // 26 letters (52 total)
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',                // 10 digits (62 total)
    ' ', ','                                                          // 2 punctuation (64 total)
};

/**
 * ASCII TO ALPHABET INDEX LOOKUP TABLE
 * 
 * Fast lookup table to convert ASCII characters to alphabet indices.
 * Given an ASCII character, this table tells you its index (0-63) in our alphabet.
 * 
 * - Valid characters map to their index (0-63)
 * - Invalid characters map to 0xFF (255) to indicate "not supported"
 * 
 * Example lookups:
 *   ascii_to_index['H'] = 7   (uppercase H)
 *   ascii_to_index['e'] = 30  (lowercase e)
 *   ascii_to_index[' '] = 62  (space)
 *   ascii_to_index['?'] = 0xFF (not in alphabet)
 * 
 * WHY A LOOKUP TABLE?
 * - O(1) constant-time lookups (very fast)
 * - Simpler than nested if-statements or switch cases
 * - Only 256 bytes of memory (one entry per possible ASCII value)
 */
static const uint8_t ascii_to_index[256] = {
    // 0-31: Control characters (invalid)
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 0-7
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 8-15
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 16-23
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 24-31
    
    // 32-47: Space and punctuation
    62,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 32-39: ' '=62, rest invalid
    0xFF, 0xFF, 0xFF, 0xFF, 63,   0xFF, 0xFF, 0xFF,  // 40-47: ','=63, rest invalid
    
    // 48-57: Digits '0'-'9'
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  // '0'=52 through '9'=61
    
    // 58-64: More punctuation (invalid)
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    
    // 65-90: Uppercase 'A'-'Z'
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25,
    
    // 91-96: More punctuation (invalid)
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    
    // 97-122: Lowercase 'a'-'z'
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
    44, 45, 46, 47, 48, 49, 50, 51,
    
    // 123-255: Rest invalid
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 123-127
    // ... rest all 0xFF (can initialize remaining in code)
};

/**
 * Random number generator seed flag.
 * Set to 1 after the first call to gen_key_pair() to avoid re-seeding.
 */
static int seeded = 0;


/* =============================================================================
 * INTERNAL HELPER FUNCTIONS (STATIC - STUDENTS DON'T CALL THESE DIRECTLY)
 * =============================================================================
 * These functions implement the mathematics of the cipher.
 * They're "static" so they can only be used within this file.
 */

/**
 * HELPER: is_valid_key()
 * 
 * Checks if a given number can be used as an encryption/decryption key.
 * 
 * MATHEMATICAL REQUIREMENT:
 * For modulus 64 (which is 2^6), a key is valid if and only if:
 * 1. It's greater than 0
 * 2. It's less than 64
 * 3. It's ODD (not divisible by 2)
 * 
 * WHY ONLY ODD NUMBERS?
 * Since 64 = 2^6, any even number shares the factor 2 with 64.
 * For multiplicative inverses to exist, the key must be "coprime" with 64,
 * meaning they share no common factors except 1. Only odd numbers qualify.
 * 
 * EXAMPLES:
 *   is_valid_key(7)  = 1 (true)  - 7 is odd
 *   is_valid_key(8)  = 0 (false) - 8 is even
 *   is_valid_key(63) = 1 (true)  - 63 is odd
 *   is_valid_key(64) = 0 (false) - out of range
 */
static int is_valid_key(uint8_t key) {
    // Key must be in range [1, CIPHER_MOD) and odd
    return (key > 0 && key < CIPHER_MOD && (key & 1) == 1);
}

/**
 * HELPER: get_valid_keys()
 * 
 * Fills an array with all valid keys for our cipher.
 * 
 * For modulus 64, there are exactly 32 valid keys: all odd numbers 1-63.
 * This function populates an array with these values.
 * 
 * PARAMETERS:
 *   keys     - Output array to fill with valid keys
 *   max_keys - Maximum size of the keys array
 * 
 * RETURNS: Number of valid keys found (will be 32 for modulus 64)
 * 
 * USAGE:
 * This is used by generate_single_keypair() to pick a random key.
 */
static int get_valid_keys(uint8_t *keys, int max_keys) {
    int count = 0;
    for (uint8_t k = 1; k < CIPHER_MOD && count < max_keys; k++) {
        if (is_valid_key(k)) {
            keys[count++] = k;
        }
    }
    return count;
}

/**
 * HELPER: find_inverse()
 * 
 * Finds the multiplicative inverse of a key under modulus 64.
 * 
 * MATHEMATICAL CONCEPT:
 * Given an encryption key 'e', we need to find a decryption key 'd' such that:
 *   (e × d) mod 64 = 1
 * 
 * This relationship means that encrypting then decrypting returns the original:
 *   P -> (P × e) mod 64 -> ((P × e) × d) mod 64 = P
 * 
 * ALGORITHM:
 * Uses brute force to try all possible values 1-63. For each candidate 'd',
 * we check if (e × d) mod 64 equals 1.
 * 
 * EXAMPLE:
 *   If e = 7:
 *     7 × 1 = 7 mod 64 = 7   (not 1, keep searching)
 *     7 × 3 = 21 mod 64 = 21 (not 1, keep searching)
 *     ...
 *     7 × 55 = 385 mod 64 = 1 (found it! return 55)
 * 
 * NOTE: This brute force approach is simple but slow. Real cryptography uses
 * the Extended Euclidean Algorithm for efficiency. For our small modulus (64),
 * brute force is fast enough and easier to understand.
 * 
 * PARAMETERS:
 *   e - The encryption key to find an inverse for
 * 
 * RETURNS: The multiplicative inverse 'd', or 0 if none found
 */
static uint8_t find_inverse(uint8_t e) {
    // Try all possible values from 1 to CIPHER_MOD-1
    for (uint8_t d = 1; d < CIPHER_MOD; d++) {
        // Check if (e * d) mod CIPHER_MOD equals 1
        if ((e * d) % CIPHER_MOD == 1) {
            return d;  // Found the inverse!
        }
    }
    return 0;  // No inverse found (should never happen for valid keys)
}

/**
 * HELPER: generate_single_keypair()
 * 
 * Generates one encryption/decryption key pair.
 * 
 * ALGORITHM:
 * 1. Get list of all 32 valid keys (odd numbers 1-63)
 * 2. Pick one at random to be the encryption key 'e'
 * 3. Use find_inverse() to calculate the matching decryption key 'd'
 * 
 * RESULT:
 * We get two keys (e, d) where (e × d) mod 64 = 1
 * - Use 'e' to encrypt: C = (P × e) mod 64
 * - Use 'd' to decrypt: P = (C × d) mod 64
 * 
 * PARAMETERS:
 *   e - Pointer to store the generated encryption key
 *   d - Pointer to store the generated decryption key
 * 
 * NOTE: This generates ONE pair. The public gen_key_pair() function calls
 * this TWICE to create keys for bidirectional communication.
 */
static void generate_single_keypair(uint8_t *e, uint8_t *d) {
    // Get all valid keys
    uint8_t valid_keys[64];
    int num_valid = get_valid_keys(valid_keys, 64);
    
    // Pick a random valid key as 'e'
    *e = valid_keys[rand() % num_valid];
    
    // Find its inverse 'd' using brute force
    *d = find_inverse(*e);
}

/* =============================================================================
 * PUBLIC API IMPLEMENTATIONS
 * =============================================================================
 * These are the functions students will call from their code.
 * See crypto-lib.h for full documentation of each function's purpose.
 */

/**
 * IMPLEMENTATION: gen_key_pair()
 * 
 * Generates two hybrid keys for bidirectional encrypted communication.
 * See crypto-lib.h for full user documentation.
 * 
 * INTERNAL DETAILS:
 * 1. Seeds random number generator on first call (using current time)
 * 2. Generates keypair 1: (e1, d1) where e1 × d1 ≡ 1 (mod 64)
 * 3. Generates keypair 2: (e2, d2) where e2 × d2 ≡ 1 (mod 64)
 * 4. Creates hybrid keys by MIXING the two pairs:
 *    - key1 = [d2 | e1]  (Party 1's key: encrypt with d2, decrypt with e1)
 *    - key2 = [d1 | e2]  (Party 2's key: encrypt with d1, decrypt with e2)
 * 
 * WHY MIX THE KEYS?
 * This creates a bidirectional channel:
 * - Party 1 encrypts with d2, Party 2 decrypts with e2 (keypair 2)
 * - Party 2 encrypts with d1, Party 1 decrypts with e1 (keypair 1)
 * Both parties can send AND receive encrypted data!
 * 
 * EXAMPLE:
 *   Generated: e1=7, d1=55, e2=11, d2=35
 *   key1 = 0x2307 = [35, 7]  (Server: encrypt with 35, decrypt with 7)
 *   key2 = 0x370B = [55, 11] (Client: encrypt with 55, decrypt with 11)
 */
int gen_key_pair(crypto_key_t *key1, crypto_key_t *key2) {
    if (key1 == NULL || key2 == NULL) {
        return RC_INVALID_ARGS;
    }
    
    // Seed random number generator on first call
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }
    
    // Generate keypair 1 (for party1 -> party2 communication)
    uint8_t e1, d1;
    generate_single_keypair(&e1, &d1);
    
    // Generate keypair 2 (for party2 -> party1 communication)
    uint8_t e2, d2;
    generate_single_keypair(&e2, &d2);
    
    // Create hybrid keys by mixing the two keypairs:
    // key1: upper byte = d2 (encrypt to party2), lower byte = e1 (decrypt from party2)
    // key2: upper byte = d1 (encrypt to party1), lower byte = e2 (decrypt from party1)
    
    *key1 = ((crypto_key_t)d2 << 8) | e1;
    *key2 = ((crypto_key_t)d1 << 8) | e2;

    // Debug output to help students understand what keys were generated
    printf("DEBUG key 1: e1=%02x(%u), d1=%02x(%u)\n", e1, e1, d1, d1);
    printf("DEBUG key 2: e2=%02x(%u), d2=%02x(%u)\n", e2, e2, d2, d2);
    printf("DEBUG FULL KEY: k1:%04x k2:%04x\n", *key1, *key2);
    
    return RC_OK;
}

/**
 * IMPLEMENTATION: encrypt()
 * 
 * Encrypts raw byte data using multiplicative cipher.
 * See crypto-lib.h for full user documentation.
 * 
 * ENCRYPTION FORMULA:
 *   For each byte P (plaintext):
 *     C = (P × encryption_key) mod 64
 *   
 *   Example: If P=10 and key=7:
 *     C = (10 × 7) mod 64 = 70 mod 64 = 6
 * 
 * IMPLEMENTATION NOTES:
 * - Extracts encryption key from upper byte of crypto_key_t
 * - Validates that key is odd and in valid range
 * - Processes data byte-by-byte in a simple loop
 * - Both input and output must be in range 0-63
 */
int encrypt(crypto_key_t key, void *encrypted_text, void *clear_text, size_t len) {
    uint8_t enc_key = GET_ENCRYPTION_KEY(key);

    if (encrypted_text == NULL || clear_text == NULL) {
        return RC_INVALID_ARGS;
    }
    
    if (enc_key == 0 || enc_key >= CIPHER_MOD || (enc_key & 1) == 0) {
        return RC_INVALID_TEXT; // Invalid key (must be odd, 1-63)
    }
    
    uint8_t *input = (uint8_t *)clear_text;
    uint8_t *output = (uint8_t *)encrypted_text;
    
    for (size_t i = 0; i < len; i++) {
        // Encrypt: C = (P * key) mod 64
        output[i] = (input[i] * enc_key) % CIPHER_MOD;
    }
    
    return RC_OK;
}

/**
 * IMPLEMENTATION: decrypt()
 * 
 * Decrypts raw byte data using multiplicative cipher.
 * See crypto-lib.h for full user documentation.
 * 
 * DECRYPTION FORMULA:
 *   For each byte C (ciphertext):
 *     P = (C × decryption_key) mod 64
 *   
 *   Example: If C=6 and key=55:
 *     P = (6 × 55) mod 64 = 330 mod 64 = 10
 * 
 * WHY THIS WORKS:
 * Because encryption_key × decryption_key ≡ 1 (mod 64), we have:
 *   decrypt(encrypt(P)) = ((P × e) × d) mod 64
 *                       = (P × (e × d)) mod 64
 *                       = (P × 1) mod 64
 *                       = P
 * The decryption "undoes" the encryption perfectly!
 * 
 * IMPLEMENTATION NOTES:
 * - Extracts decryption key from lower byte of crypto_key_t
 * - Validates that key is odd and in valid range
 * - Identical structure to encrypt(), just uses different key component
 */
int decrypt(crypto_key_t key, void *clear_text, void *encrypted_text, size_t len) {
    uint8_t dec_key = GET_DECRYPTION_KEY(key);

    if (clear_text == NULL || encrypted_text == NULL) {
        return RC_INVALID_ARGS;
    }
    
    if (dec_key == 0 || dec_key >= CIPHER_MOD || (dec_key & 1) == 0) {
        return RC_INVALID_TEXT; // Invalid key (must be odd, 1-63)
    }
    
    uint8_t *input = (uint8_t *)encrypted_text;
    uint8_t *output = (uint8_t *)clear_text;
    
    for (size_t i = 0; i < len; i++) {
        // Decrypt: P = (C * key) mod 64
        output[i] = (input[i] * dec_key) % CIPHER_MOD;
    }
    
    return RC_OK;
}

/**
 * IMPLEMENTATION: print_encrypted_string()
 * 
 * Convenience function for debugging - decrypts and prints data.
 * See crypto-lib.h for full user documentation.
 * 
 * ALGORITHM:
 * 1. Allocates temporary buffer for decrypted bytes
 * 2. Calls decrypt() to decrypt the data
 * 3. Converts each decrypted byte (0-63) to its alphabet character
 * 4. Prints characters to stdout
 * 5. Frees temporary buffer
 * 
 * USAGE NOTE:
 * This is mainly for debugging. Production code should use decrypt_string()
 * to get the decrypted string for further processing.
 */
void print_encrypted_string(crypto_key_t key, void *encrypted_text, size_t len) {
    
    if (encrypted_text == NULL || len == 0) {
        return;
    }
    
    // Allocate temporary buffer for decrypted text
    uint8_t *decrypted = malloc(len);
    if (decrypted == NULL) {
        return;
    }
    
    // Decrypt the data
    if (decrypt(key, decrypted, encrypted_text, len) == RC_OK) {
        // Print each decrypted byte as its corresponding character
        for (size_t i = 0; i < len; i++) {
            if (decrypted[i] < CIPHER_MOD) {
                putchar(alphabet[decrypted[i]]);
            } else {
                putchar('?');  // Invalid index
            }
        }
    }
    
    free(decrypted);
}

/* =============================================================================
 * STRING CONVERSION FUNCTIONS
 * =============================================================================
 * These functions bridge between ASCII strings and the cipher's byte format.
 * Students will use encrypt_string() and decrypt_string() most often.
 */

/**
 * IMPLEMENTATION: string_to_bytes()
 * 
 * Converts ASCII string to byte indices using the alphabet mapping.
 * See crypto-lib.h for full user documentation.
 * 
 * ALGORITHM:
 * 1. For each character in the input string:
 *    a. Look up its index in ascii_to_index table
 *    b. If index is 0xFF, character is not in alphabet - return error
 *    c. Otherwise, store the index (0-63) in output buffer
 * 
 * EXAMPLE:
 *   Input:  "Hello" (ASCII: 'H'=72, 'e'=101, 'l'=108, 'l'=108, 'o'=111)
 *   Output: {7, 30, 37, 37, 40} (alphabet indices)
 * 
 * COMMON ERROR:
 * Students often forget that not all ASCII characters are supported!
 * Only the 64 characters in our alphabet can be encrypted.
 * Punctuation like '.', '!', '?' will cause RC_INVALID_TEXT error.
 */
int string_to_bytes(const uint8_t *str, uint8_t *bytes, size_t len) {
    if (str == NULL || bytes == NULL) {
        return RC_INVALID_ARGS;
    }
    
    
    for (size_t i = 0; i < len; i++) {
        uint8_t index = ascii_to_index[(uint8_t)str[i]];
        
        if (index == 0xFF) {
            // Invalid character not in our alphabet
            return RC_INVALID_TEXT;
        }
        
        bytes[i] = index;
    }
    
    return (int)len;
}

/**
 * IMPLEMENTATION: bytes_to_string()
 * 
 * Converts byte indices back to ASCII string using alphabet mapping.
 * See crypto-lib.h for full user documentation.
 * 
 * ALGORITHM:
 * 1. For each byte in input:
 *    a. Verify it's in valid range (0-63)
 *    b. Look up corresponding character in alphabet array
 *    c. Store character in output string
 * 
 * EXAMPLE:
 *   Input:  {7, 30, 37, 37, 40} (alphabet indices)
 *   Output: "Hello"
 * 
 * IMPORTANT:
 * This function does NOT null-terminate the output!
 * Students must manually add '\0' after calling this function.
 */
int bytes_to_string(const uint8_t *bytes, size_t len, uint8_t *str) {
    if (bytes == NULL || str == NULL) {
        return RC_INVALID_ARGS;
    }
    
    for (size_t i = 0; i < len; i++) {
        if (bytes[i] >= CIPHER_MOD) {
            return RC_INVALID_TEXT;  // Invalid byte value
        }
        str[i] = alphabet[bytes[i]];
    }
    
    return (int)len;
}

/**
 * IMPLEMENTATION: encrypt_string()
 * 
 * High-level function that encrypts an ASCII string in one call.
 * See crypto-lib.h for full user documentation.
 * 
 * THIS IS THE MAIN FUNCTION STUDENTS WILL USE FOR ENCRYPTION!
 * 
 * ALGORITHM:
 * 1. Allocate temporary buffer for byte conversion
 * 2. Convert string to bytes (ASCII -> indices 0-63)
 * 3. Encrypt those bytes using the cipher
 * 4. Free temporary buffer
 * 5. Return encrypted bytes to caller
 * 
 * WORKFLOW EXAMPLE:
 *   Input:  "Hello" (5 ASCII characters)
 *   Step 1: {7, 30, 37, 37, 40} (byte indices)
 *   Step 2: {15, 34, 51, 51, 24} (encrypted with key=7)
 *   Output: {15, 34, 51, 51, 24} (encrypted bytes, ready to send)
 * 
 * MEMORY NOTE:
 * Uses malloc() internally for temporary conversion buffer.
 * All cleanup is automatic - students don't need to worry about it.
 */
int encrypt_string(crypto_key_t key, uint8_t *encrypted_bytes, uint8_t *clear_str, size_t len) {
    if (clear_str == NULL || encrypted_bytes == NULL) {
        return RC_INVALID_ARGS;
    }
    
    uint8_t *temp_bytes = malloc(len);
    if (temp_bytes == NULL) {
        return RC_INVALID_BUFF;
    }
    
    // Convert string to bytes
    int byte_len = string_to_bytes(clear_str, temp_bytes, len);
    if (byte_len < 0) {
        free(temp_bytes);
        return byte_len;  // Return error code
    }
    
    // Encrypt the bytes
    int result = encrypt(key, encrypted_bytes, temp_bytes, byte_len);
    free(temp_bytes);
    
    if (result != RC_OK) {
        return result;
    }
    
    return byte_len;  // Return number of encrypted bytes
}


/**
 * IMPLEMENTATION: decrypt_string()
 * 
 * High-level function that decrypts bytes back to an ASCII string.
 * See crypto-lib.h for full user documentation.
 * 
 * THIS IS THE MAIN FUNCTION STUDENTS WILL USE FOR DECRYPTION!
 * 
 * ALGORITHM:
 * 1. Allocate temporary buffer for decrypted bytes
 * 2. Decrypt the encrypted bytes
 * 3. Convert bytes to ASCII string (indices 0-63 -> characters)
 * 4. Free temporary buffer
 * 5. Return decrypted string to caller
 * 
 * WORKFLOW EXAMPLE:
 *   Input:  {15, 34, 51, 51, 24} (encrypted bytes from network)
 *   Step 1: {7, 30, 37, 37, 40} (decrypted to byte indices)
 *   Step 2: "Hello" (converted to ASCII)
 *   Output: "Hello" (ready to display/process)
 * 
 * IMPORTANT REMINDER:
 * Output is NOT null-terminated! Students must add '\0' themselves:
 *   decrypt_string(key, buffer, encrypted, len);
 *   buffer[len] = '\0';  // Add null terminator
 */
int decrypt_string(crypto_key_t key, uint8_t *clear_str, uint8_t *encrypted_bytes, size_t len) {
    if (clear_str == NULL || encrypted_bytes == NULL) {
        return RC_INVALID_ARGS;
    }

    // Allocate temporary buffer for decrypted bytes
    uint8_t *temp_bytes = malloc(len);
    if (temp_bytes == NULL) {
        return RC_INVALID_BUFF;
    }
    
    // Decrypt the bytes
    int result = decrypt(key, temp_bytes, (void *)encrypted_bytes, len);
    if (result != RC_OK) {
        free(temp_bytes);
        return result;
    }
    
    // Convert bytes to string
    result = bytes_to_string(temp_bytes, len, clear_str);
    free(temp_bytes);
    
    return result;
}


/**
 * IMPLEMENTATION: printable_encrypted_string()
 * 
 * Converts encrypted bytes to readable text WITHOUT decrypting.
 * See crypto-lib.h for full user documentation.
 * 
 * PURPOSE:
 * Shows what encrypted data "looks like" as text, for display/debugging.
 * This is NOT the original message - it's just a readable representation
 * of the encrypted bytes.
 * 
 * ALGORITHM:
 * 1. For each encrypted byte (value 0-63):
 *    a. Look up its character in the alphabet
 *    b. Store that character in output buffer
 * 
 * EXAMPLE:
 *   Encrypted bytes: {15, 34, 51, 51, 24}
 *   Printable form:  "PivvY"  (NOT "Hello" - this is still encrypted!)
 * 
 * USE CASE:
 * Useful for debugging protocol messages - you can see that encrypted
 * data is present and looks "scrambled", without revealing the content.
 */
int printable_encrypted_string(uint8_t *encrypted_buff, uint8_t *char_buff, size_t len) {
    if (encrypted_buff == NULL || char_buff == NULL) {
        return RC_INVALID_ARGS;
    }
    
    for (size_t i = 0; i < len; i++) {
        if (encrypted_buff[i] >= CIPHER_MOD) {
            return RC_INVALID_TEXT;  // Invalid byte value
        }
        char_buff[i] = alphabet[encrypted_buff[i]];
    }
    
    return RC_OK;
}

/* =============================================================================
 * NETWORK PROTOCOL / PDU UTILITIES
 * =============================================================================
 * These functions help package data into protocol messages and display them.
 */

/**
 * IMPLEMENTATION: print_msg_info()
 * 
 * Prints detailed information about a protocol message (PDU) for debugging.
 * See crypto-lib.h for full user documentation.
 * 
 * THIS IS EXTREMELY HELPFUL FOR DEBUGGING STUDENT CODE!
 * 
 * WHAT IT DISPLAYS:
 * - Message type (MSG_DATA, MSG_ENCRYPTED_DATA, MSG_KEY_EXCHANGE, etc.)
 * - Direction (REQUEST from client, RESPONSE from server)
 * - Payload length in bytes
 * - Payload content (formatted based on message type):
 *   * Plain text for MSG_DATA
 *   * Encrypted + decrypted text for MSG_ENCRYPTED_DATA (if key available)
 *   * Key value for MSG_KEY_EXCHANGE
 * 
 * ASYMMETRIC KEY HANDLING:
 * The 'mode' parameter matters because keys work differently for each party:
 * - SERVER_MODE: Can decrypt incoming REQUEST messages (uses decryption key)
 * - CLIENT_MODE: Can decrypt incoming RESPONSE messages (uses decryption key)
 * 
 * DEBUGGING TIP:
 * Add print_msg_info() calls before and after sending/receiving to see
 * exactly what's being transmitted. This catches protocol errors quickly!
 */
void print_msg_info(crypto_msg_t *msg, crypto_key_t key, int mode) {
    if (msg == NULL) return;

    const crypto_pdu_t *pdu = &msg->header;

    if(pdu->direction == DIR_REQUEST) {
        printf(">>>>>>>>>>>>>>> REQUEST >>>>>>>>>>>>>>>\n");
    } else {
        printf("<<<<<<<<<<<<<<< RESPONSE <<<<<<<<<<<<<<<\n");
    }
    
    printf("-------------------------\nPDU Info:\n");
    
    // Print message type
    printf("  Type: ");
    switch(pdu->msg_type) {
        case MSG_KEY_EXCHANGE:     printf("KEY_EXCHANGE"); break;
        case MSG_DATA:             printf("DATA"); break;
        case MSG_ENCRYPTED_DATA:   printf("ENCRYPTED_DATA"); break;
        case MSG_DIG_SIGNATURE:    printf("DIGITAL_SIGNATURE"); break;
        case MSG_HELP_CMD:         printf("HELP_CMD"); break;
        case MSG_CMD_CLIENT_STOP:  printf("CMD_CLIENT_STOP"); break;
        case MSG_CMD_SERVER_STOP:  printf("CMD_SERVER_STOP"); break;
        case MSG_ERROR:            printf("ERROR"); break;
        case MSG_EXIT:             printf("EXIT"); break;
        case MSG_SHUTDOWN:         printf("SHUTDOWN"); break;
        default:                   printf("UNKNOWN(%d)", pdu->msg_type); break;
    }
    printf("\n");
    
    // Print direction
    printf("  Direction: %s\n", 
           pdu->direction == DIR_REQUEST ? "REQUEST" : "RESPONSE");
    
    // Print payload length
    printf("  Payload Length: %u bytes\n", pdu->payload_len);

    if (pdu->payload_len > 0) {
        switch(pdu->msg_type) {
            case MSG_KEY_EXCHANGE:
                if (pdu->payload_len == sizeof(crypto_key_t)) {
                    crypto_key_t *keys = (crypto_key_t *)msg->payload;
                    printf("  Payload: Key=0x%04x\n", keys[0]);
                } else {
                    printf("  Payload: Invalid length for KEY_EXCHANGE\n");
                }
                break;
            case MSG_DATA:
                printf("  Payload (plaintext): %*s\n",pdu->payload_len, msg->payload);
                break;
            case MSG_ENCRYPTED_DATA:
                if (key == NULL_CRYPTO_KEY) {
                    printf("  Payload: Encrypted data but invalid key provided to decrypt\n");
                    break;
                }
                uint8_t *msg_data = malloc(pdu->payload_len + 1);
                if (printable_encrypted_string((uint8_t *)msg->payload, msg_data, pdu->payload_len) == RC_OK) {
                    msg_data[pdu->payload_len] = '\0'; // Null-terminate
                    printf("  Payload (encrypted): \"%s\"\n", msg_data);

                    //Since the keys are asymentric we only can print the decryted string on the REQUEST
                    //to the server or RESPONSE FROM the client
                    if ((mode == SERVER_MODE && pdu->direction == DIR_REQUEST) ||
                        (mode == CLIENT_MODE && pdu->direction == DIR_RESPONSE)) {

                        if(decrypt_string(key, msg_data, msg->payload, pdu->payload_len) > 0) {
                            msg_data[pdu->payload_len] = '\0'; // Null-terminate
                            printf("  Payload (decrypted): \"%s\"\n", msg_data);
                        } else {
                            printf("  Payload: Decryption error\n");
                        }

                    }


                    

                } else {
                    printf("  Payload: Invalid data\n");
                }
                free(msg_data);
                break;
            case MSG_DIG_SIGNATURE:
                printf("  Payload: Digital Signature (%u bytes)\n", pdu->payload_len);
                break;
            case MSG_HELP_CMD:
            case MSG_CMD_CLIENT_STOP:
            case MSG_CMD_SERVER_STOP:
            case MSG_ERROR:
            case MSG_EXIT:
            case MSG_SHUTDOWN:
                printf("  Payload: Command/Status (%u bytes)\n", pdu->payload_len);
                break;
            default:
                printf("  Payload: Unknown message type (%u bytes)\n", pdu->payload_len);
                break;
        }
    } else {
        printf("  No Payload\n");
    }

    if(pdu->direction == DIR_REQUEST) {
        printf(">>>>>>>>>>>>> END REQUEST >>>>>>>>>>>>>\n\n");
    } else {
        printf("<<<<<<<<<<<<< END RESPONSE <<<<<<<<<<<<<\n\n");
    }
}


/* =============================================================================
 * END OF CRYPTO-LIB.C IMPLEMENTATION
 * =============================================================================
 * 
 * STUDENT SUMMARY:
 * 
 * You now have a complete crypto library at your disposal! The main functions
 * you'll use in your client/server implementation are:
 * 
 * KEY MANAGEMENT:
 *   - gen_key_pair()        Generate keys (server side, during handshake)
 * 
 * ENCRYPTION/DECRYPTION:
 *   - encrypt_string()      Encrypt a message before sending
 *   - decrypt_string()      Decrypt a received message
 * 
 * DEBUGGING:
 *   - print_msg_info()      Display PDU contents (VERY useful!)
 * 
 * Focus on implementing your network protocol using these functions.
 * The cryptography is handled for you!
 * =============================================================================
 */

