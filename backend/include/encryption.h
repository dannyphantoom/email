#pragma once

#include <string>
#include <vector>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>

class Encryption {
public:
    static const int KEY_SIZE = 32;  // 256 bits
    static const int IV_SIZE = 16;   // 128 bits
    static const int SALT_SIZE = 32; // 256 bits

    Encryption();
    ~Encryption();

    // AES-256 encryption/decryption
    std::string encryptAES(const std::string& plaintext, const std::string& key);
    std::string decryptAES(const std::string& ciphertext, const std::string& key);
    
    // Key generation
    std::string generateRandomKey();
    std::string generateRandomIV();
    std::string generateRandomSalt();
    
    // Password hashing (bcrypt-like)
    std::string hashPassword(const std::string& password);
    bool verifyPassword(const std::string& password, const std::string& hash);
    
    // JWT token operations
    std::string generateJWT(const std::string& payload, const std::string& secret);
    bool verifyJWT(const std::string& token, const std::string& secret, std::string& payload);
    
    // Base64 encoding/decoding
    std::string base64Encode(const std::string& data);
    std::string base64Decode(const std::string& encoded);
    
    // HMAC for message integrity
    std::string generateHMAC(const std::string& data, const std::string& key);
    bool verifyHMAC(const std::string& data, const std::string& key, const std::string& hmac);

private:
    std::string masterKey_;
    
    // Helper functions
    std::string pbkdf2(const std::string& password, const std::string& salt, int iterations = 10000);
    std::string sha256(const std::string& data);
    std::string hmacSha256(const std::string& data, const std::string& key);
    
    // OpenSSL context
    EVP_CIPHER_CTX* cipherCtx_;
    EVP_MD_CTX* mdCtx_;
    
    void initializeOpenSSL();
    void cleanupOpenSSL();
}; 