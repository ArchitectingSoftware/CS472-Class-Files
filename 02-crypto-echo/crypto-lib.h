/**
 * =============================================================================
 * CRYPTO-LIB.H - Simple Asymmetric Cipher Library (Header)
 * =============================================================================
 * 
 * PURPOSE:
 * This library provides a simple educational asymmetric cipher using modular
 * multiplicative arithmetic. It is designed to teach the concepts of public-key
 * encryption in a network programming context.
 * 
 * WARNING: This cipher is for EDUCATIONAL USE ONLY - it is NOT cryptographically
 * secure and should NEVER be used in production systems.
 * 
 * KEY CONCEPTS:
 * - Uses modulus 64 arithmetic with a custom 64-character alphabet
 * - Implements asymmetric encryption (different keys for encrypt/decrypt)
 * - Generates hybrid keys that support bidirectional communication
 * - Each party gets a key with both encryption and decryption components
 * 
 * WHAT STUDENTS WILL USE THIS FOR:
 * Students will use these functions to implement encrypted client-server
 * communication. The library handles all the cryptographic operations;
 * students focus on the network protocol and message handling.
 * =============================================================================
 */

#ifndef __CRYPTO_LIB_H__
#define __CRYPTO_LIB_H__

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include "protocol.h"

/* =============================================================================
 * RETURN CODE CONSTANTS
 * =============================================================================
 * These are returned by most functions to indicate success or specific errors.
 * Always check return values to handle errors gracefully.
 */
#define RC_OK 0                  // Operation succeeded
#define RC_INVALID_ARGS -1       // NULL pointer or invalid parameter passed
#define RC_INVALID_TEXT -2       // Text contains characters not in our alphabet
#define RC_INVALID_BUFF -3       // Buffer too small or memory allocation failed
#define RC_CRYPTO_ERR   -4       // Encryption/decryption operation failed


/* =============================================================================
 * MODE CONSTANTS
 * =============================================================================
 * Used by print_msg_info() to determine which perspective to print from.
 * This affects what information can be displayed (since keys are asymmetric).
 */
#define SERVER_MODE 1            // Print from server perspective
#define CLIENT_MODE 2            // Print from client perspective


/* =============================================================================
 * CRYPTO KEY TYPE AND UTILITIES
 * =============================================================================
 */

/**
 * Special value indicating an invalid/uninitialized key.
 * Check for this value before attempting encryption/decryption.
 */
#define NULL_CRYPTO_KEY 0xFFFF

/**
 * CRYPTO KEY TYPE:
 * 
 * A crypto_key_t is a 16-bit value that contains TWO separate 8-bit keys:
 *   - Upper byte (bits 8-15): Encryption key (use GET_ENCRYPTION_KEY to extract)
 *   - Lower byte (bits 0-7):  Decryption key (use GET_DECRYPTION_KEY to extract)
 * 
 * Example: If key = 0x2307
 *   - Encryption key = 0x23 (35 decimal) - used to encrypt messages you send
 *   - Decryption key = 0x07 (7 decimal)  - used to decrypt messages you receive
 * 
 * WHY HYBRID KEYS?
 * In asymmetric encryption, you need different keys for sending vs receiving.
 * Combining them into one uint16_t simplifies key management in your code.
 */
typedef uint16_t crypto_key_t;

/**
 * MACRO: GET_ENCRYPTION_KEY(key)
 * 
 * Extracts the encryption key (upper byte) from a hybrid crypto_key_t.
 * Use this key when calling encrypt() or encrypt_string().
 * 
 * Example: 
 *   crypto_key_t my_key = 0x2307;
 *   uint8_t enc = GET_ENCRYPTION_KEY(my_key);  // Returns 0x23 (35)
 */
#define GET_ENCRYPTION_KEY(key) ((uint8_t)((key) >> 8))

/**
 * MACRO: GET_DECRYPTION_KEY(key)
 * 
 * Extracts the decryption key (lower byte) from a hybrid crypto_key_t.
 * Use this key when calling decrypt() or decrypt_string().
 * 
 * Example:
 *   crypto_key_t my_key = 0x2307;
 *   uint8_t dec = GET_DECRYPTION_KEY(my_key);  // Returns 0x07 (7)
 */
#define GET_DECRYPTION_KEY(key) ((uint8_t)((key) & 0xFF))

/* =============================================================================
 * CORE CRYPTOGRAPHIC FUNCTIONS
 * =============================================================================
 * These are the fundamental functions for key generation and encryption.
 * Most students will use the higher-level string functions below instead.
 */

/**
 * FUNCTION: gen_key_pair()
 * 
 * Generate two complementary hybrid keys for bidirectional encrypted communication.
 * 
 * HOW IT WORKS:
 * 1. Generates two random key pairs (e1,d1) and (e2,d2) where each pair consists
 *    of multiplicative inverses under modulus 64
 * 2. Creates hybrid keys by mixing the pairs:
 *    - key1 gets d2 (for encrypting) and e1 (for decrypting)
 *    - key2 gets d1 (for encrypting) and e2 (for decrypting)
 * 3. This allows Party 1 and Party 2 to both send and receive encrypted data
 * 
 * TYPICAL USAGE:
 * - Server generates keys and sends key2 to the client during handshake
 * - Both parties store their respective keys for the session
 * 
 * PARAMETERS:
 *   key1 - Pointer to receive the first hybrid key (typically for the server)
 *   key2 - Pointer to receive the second hybrid key (typically for the client)
 * 
 * RETURNS:
 *   RC_OK (0) on success
 *   RC_INVALID_ARGS (-1) if either pointer is NULL
 * 
 * EXAMPLE:
 *   crypto_key_t server_key, client_key;
 *   if (gen_key_pair(&server_key, &client_key) == RC_OK) {
 *       // Send client_key to the client over the network
 *       // Server keeps server_key for its own use
 *   }
 */
int gen_key_pair(crypto_key_t *key1, crypto_key_t *key2);

/**
 * FUNCTION: encrypt()
 * 
 * Encrypts raw byte data using the multiplicative cipher.
 * 
 * HOW IT WORKS:
 * Applies the formula: C = (P × key) mod 64 to each byte
 * where P is plaintext (0-63) and C is ciphertext (0-63)
 * 
 * IMPORTANT NOTES:
 * - Input bytes MUST be in range 0-63 (use string_to_bytes() for text)
 * - Output bytes will also be in range 0-63
 * - This is a LOW-LEVEL function; most students should use encrypt_string()
 * - The key parameter should be the ENCRYPTION part of your crypto_key_t
 *   (extract it using GET_ENCRYPTION_KEY macro)
 * 
 * PARAMETERS:
 *   key            - The crypto_key_t containing both enc/dec keys
 *   encrypted_text - Output buffer for encrypted data (must be at least 'len' bytes)
 *   clear_text     - Input buffer containing plaintext (bytes must be 0-63)
 *   len            - Number of bytes to encrypt
 * 
 * RETURNS:
 *   RC_OK (0) on success
 *   RC_INVALID_ARGS (-1) if any pointer is NULL
 *   RC_INVALID_TEXT (-2) if key is invalid (even or out of range)
 * 
 * EXAMPLE:
 *   uint8_t plain[5] = {10, 20, 30, 40, 50};  // Already in 0-63 range
 *   uint8_t cipher[5];
 *   if (encrypt(my_key, cipher, plain, 5) == RC_OK) {
 *       // cipher now contains encrypted data
 *   }
 */
int encrypt(crypto_key_t key, void *encrypted_text, void *clear_text, size_t len);

/**
 * FUNCTION: decrypt()
 * 
 * Decrypts raw byte data using the multiplicative cipher.
 * 
 * HOW IT WORKS:
 * Applies the formula: P = (C × key) mod 64 to each byte
 * where C is ciphertext (0-63) and P is recovered plaintext (0-63)
 * 
 * IMPORTANT NOTES:
 * - Input must be previously encrypted data (values 0-63)
 * - Output will be in range 0-63 (use bytes_to_string() to convert to text)
 * - This is a LOW-LEVEL function; most students should use decrypt_string()
 * - The key parameter should be the DECRYPTION part of your crypto_key_t
 *   (extract it using GET_DECRYPTION_KEY macro)
 * 
 * PARAMETERS:
 *   key            - The crypto_key_t containing both enc/dec keys
 *   clear_text     - Output buffer for decrypted data (must be at least 'len' bytes)
 *   encrypted_text - Input buffer containing ciphertext
 *   len            - Number of bytes to decrypt
 * 
 * RETURNS:
 *   RC_OK (0) on success
 *   RC_INVALID_ARGS (-1) if any pointer is NULL
 *   RC_INVALID_TEXT (-2) if key is invalid (even or out of range)
 * 
 * EXAMPLE:
 *   uint8_t cipher[5] = {15, 25, 35, 45, 55};  // Previously encrypted
 *   uint8_t plain[5];
 *   if (decrypt(my_key, plain, cipher, 5) == RC_OK) {
 *       // plain now contains decrypted data (still in 0-63 range)
 *   }
 */
int decrypt(crypto_key_t key, void *clear_text, void *encrypted_text, size_t len);

/**
 * FUNCTION: print_encrypted_string()
 * 
 * Decrypts and prints encrypted data to stdout.
 * 
 * PURPOSE:
 * Convenience function for debugging - decrypts data and prints it as
 * readable characters using the 64-character alphabet.
 * 
 * PARAMETERS:
 *   key            - Crypto key (uses decryption component)
 *   encrypted_text - Buffer containing encrypted bytes
 *   len            - Number of bytes to decrypt and print
 * 
 * RETURNS: void (prints directly to stdout)
 * 
 * NOTE: This function allocates temporary memory internally.
 */
void print_encrypted_string(crypto_key_t key, void *encrypted_text, size_t len);


/* =============================================================================
 * HIGH-LEVEL STRING CONVERSION FUNCTIONS
 * =============================================================================
 * These functions handle conversion between ASCII strings and the cipher's
 * 64-character alphabet. USE THESE for working with text messages.
 */

/**
 * FUNCTION: string_to_bytes()
 * 
 * Converts an ASCII string to byte indices in the cipher's alphabet.
 * 
 * HOW IT WORKS:
 * Maps each character to its index (0-63) in the alphabet:
 *   'A'-'Z' -> 0-25, 'a'-'z' -> 26-51, '0'-'9' -> 52-61, ' ' -> 62, ',' -> 63
 * 
 * IMPORTANT:
 * - Only characters in the 64-character alphabet are supported
 * - Invalid characters will cause the function to fail with RC_INVALID_TEXT
 * - This must be called BEFORE encrypting text
 * 
 * PARAMETERS:
 *   str   - Input ASCII string (does not need to be null-terminated if using len)
 *   bytes - Output buffer for byte indices (0-63 values)
 *   len   - Number of characters to convert
 * 
 * RETURNS:
 *   Number of bytes converted (same as len) on success
 *   RC_INVALID_ARGS (-1) if any pointer is NULL
 *   RC_INVALID_TEXT (-2) if string contains unsupported characters
 * 
 * EXAMPLE:
 *   uint8_t input[] = "Hello";
 *   uint8_t bytes[5];
 *   if (string_to_bytes(input, bytes, 5) > 0) {
 *       // bytes now contains: {7, 30, 37, 37, 40}
 *   }
 */
int string_to_bytes(const uint8_t *str, uint8_t *bytes, size_t len);

/**
 * FUNCTION: bytes_to_string()
 * 
 * Converts byte indices back to ASCII characters using the cipher's alphabet.
 * 
 * HOW IT WORKS:
 * Maps each byte index (0-63) to its corresponding character in the alphabet.
 * This is the inverse of string_to_bytes().
 * 
 * IMPORTANT:
 * - Input bytes must be in range 0-63
 * - Output is NOT null-terminated (caller must add '\0' if needed)
 * - This is called AFTER decrypting to get readable text
 * 
 * PARAMETERS:
 *   bytes - Input buffer containing byte indices (0-63 values)
 *   len   - Number of bytes to convert
 *   str   - Output buffer for ASCII characters
 * 
 * RETURNS:
 *   Number of characters written (same as len) on success
 *   RC_INVALID_ARGS (-1) if any pointer is NULL
 *   RC_INVALID_TEXT (-2) if any byte is >= 64
 * 
 * EXAMPLE:
 *   uint8_t bytes[5] = {7, 30, 37, 37, 40};
 *   uint8_t str[6];  // +1 for null terminator
 *   if (bytes_to_string(bytes, 5, str) > 0) {
 *       str[5] = '\0';  // Add null terminator
 *       // str now contains: "Hello"
 *   }
 */
int bytes_to_string(const uint8_t *bytes, size_t len, uint8_t *str);

/**
 * FUNCTION: encrypt_string()
 * 
 * Encrypts an ASCII string in one step (convert to bytes + encrypt).
 * 
 * HOW IT WORKS:
 * 1. Converts the string to byte indices (0-63) using string_to_bytes()
 * 2. Encrypts those bytes using encrypt()
 * 3. Returns the encrypted byte array
 * 
 * THIS IS THE FUNCTION MOST STUDENTS WILL USE for encrypting messages!
 * 
 * IMPORTANT NOTES:
 * - Input string can only contain the 64 supported characters
 * - Output is encrypted bytes (0-63 values), not printable text
 * - Use the ENCRYPTION key from your crypto_key_t (upper byte)
 * 
 * PARAMETERS:
 *   key             - Crypto key (uses encryption component)
 *   encrypted_bytes - Output buffer for encrypted data
 *   clear_str       - Input ASCII string to encrypt
 *   len             - Length of string to encrypt
 * 
 * RETURNS:
 *   Number of encrypted bytes on success (same as len)
 *   RC_INVALID_ARGS (-1) if any pointer is NULL
 *   RC_INVALID_TEXT (-2) if string contains unsupported characters
 *   RC_INVALID_BUFF (-3) if memory allocation fails
 *   RC_CRYPTO_ERR (-4) if encryption fails
 * 
 * EXAMPLE:
 *   crypto_key_t key = 0x2307;  // Server key
 *   uint8_t msg[] = "Hello";
 *   uint8_t encrypted[5];
 *   int result = encrypt_string(key, encrypted, msg, 5);
 *   if (result > 0) {
 *       // Send 'encrypted' over the network
 *   }
 */
int encrypt_string(crypto_key_t key, uint8_t *encrypted_bytes, uint8_t *clear_str, size_t len);

/**
 * FUNCTION: decrypt_string()
 * 
 * Decrypts encrypted bytes back to an ASCII string in one step.
 * 
 * HOW IT WORKS:
 * 1. Decrypts the bytes using decrypt()
 * 2. Converts byte indices back to ASCII using bytes_to_string()
 * 3. Returns the readable string
 * 
 * THIS IS THE FUNCTION MOST STUDENTS WILL USE for decrypting messages!
 * 
 * IMPORTANT NOTES:
 * - Input must be previously encrypted data
 * - Output is NOT null-terminated (add '\0' yourself if needed)
 * - Use the DECRYPTION key from your crypto_key_t (lower byte)
 * 
 * PARAMETERS:
 *   key             - Crypto key (uses decryption component)
 *   clear_str       - Output buffer for decrypted ASCII string
 *   encrypted_bytes - Input buffer containing encrypted data
 *   len             - Number of bytes to decrypt
 * 
 * RETURNS:
 *   Number of characters written on success (same as len)
 *   RC_INVALID_ARGS (-1) if any pointer is NULL
 *   RC_INVALID_BUFF (-3) if memory allocation fails
 *   RC_CRYPTO_ERR (-4) if decryption fails
 * 
 * EXAMPLE:
 *   crypto_key_t key = 0x2307;  // Server key
 *   uint8_t encrypted[5] = {...};  // Received from network
 *   uint8_t plaintext[6];  // +1 for null terminator
 *   int result = decrypt_string(key, plaintext, encrypted, 5);
 *   if (result > 0) {
 *       plaintext[5] = '\0';  // Add null terminator
 *       printf("Decrypted: %s\n", plaintext);
 *   }
 */
int decrypt_string(crypto_key_t key, uint8_t *clear_str, uint8_t *encrypted_bytes, size_t len);

/**
 * FUNCTION: printable_encrypted_string()
 * 
 * Converts encrypted bytes to printable characters WITHOUT decrypting.
 * 
 * PURPOSE:
 * Shows what the encrypted data "looks like" as characters, without revealing
 * the actual content. Useful for debugging and displaying encrypted messages.
 * 
 * HOW IT WORKS:
 * Each encrypted byte (0-63) is mapped directly to its corresponding character
 * in the alphabet, WITHOUT performing decryption. This is NOT the original text!
 * 
 * PARAMETERS:
 *   encrypted_buff - Input buffer containing encrypted bytes (0-63 values)
 *   char_buff      - Output buffer for printable characters
 *   len            - Number of bytes to convert
 * 
 * RETURNS:
 *   RC_OK (0) on success
 *   RC_INVALID_ARGS (-1) if any pointer is NULL
 *   RC_INVALID_TEXT (-2) if any byte is >= 64
 * 
 * EXAMPLE:
 *   uint8_t encrypted[5] = {15, 25, 35, 45, 55};
 *   uint8_t printable[6];
 *   if (printable_encrypted_string(encrypted, printable, 5) == RC_OK) {
 *       printable[5] = '\0';
 *       printf("Encrypted as text: %s\n", printable);  // Shows cipher text
 *   }
 */
int printable_encrypted_string(uint8_t *encrypted_buff, uint8_t *char_buff, size_t len);


/* =============================================================================
 * NETWORK MESSAGE UTILITIES
 * =============================================================================
 * These functions help build and display protocol messages (PDUs).
 * Students will use these when implementing the client-server protocol.
 */

/**
 * FUNCTION: print_msg_info()
 * 
 * Prints detailed information about a protocol message (PDU) for debugging.
 * 
 * PURPOSE:
 * Displays the PDU header fields and payload in a readable format. For encrypted
 * messages, it attempts to decrypt and display the content if keys are available.
 * This is VERY useful for debugging your protocol implementation!
 * 
 * HOW IT WORKS:
 * - Prints message type, direction, and payload length
 * - For MSG_KEY_EXCHANGE: displays the key value
 * - For MSG_DATA: displays plaintext
 * - For MSG_ENCRYPTED_DATA: displays encrypted form AND decrypted form (if possible)
 * 
 * PARAMETERS:
 *   msg  - Pointer to the crypto_msg_t to display
 *   key  - The crypto_key_t for this session (use NULL_CRYPTO_KEY if no key yet)
 *   mode - SERVER_MODE or CLIENT_MODE (affects which keys can decrypt)
 * 
 * RETURNS: void (prints to stdout)
 * 
 * IMPORTANT:
 * The 'mode' parameter matters because keys are asymmetric:
 * - In SERVER_MODE: can decrypt REQUEST messages, encrypts RESPONSE messages
 * - In CLIENT_MODE: can decrypt RESPONSE messages, encrypts REQUEST messages
 * 
 * EXAMPLE:
 *   crypto_msg_t *received_msg = (crypto_msg_t *)buffer;
 *   print_msg_info(received_msg, my_key, CLIENT_MODE);
 *   // Prints detailed information about the received message
 */
void print_msg_info(crypto_msg_t *msg, crypto_key_t key, int mode);




#endif // __CRYPTO_LIB_H__