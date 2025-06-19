#include "encryption.h"
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>
#include <cstring>

Encryption::Encryption() : cipherCtx_(nullptr), mdCtx_(nullptr) {
    initializeOpenSSL();
    masterKey_ = generateRandomKey();
}

Encryption::~Encryption() {
    cleanupOpenSSL();
}

void Encryption::initializeOpenSSL() {
    // OpenSSL 3.0+ doesn't need explicit initialization
    // The library is automatically initialized when needed
    
    // Create contexts
    cipherCtx_ = EVP_CIPHER_CTX_new();
    mdCtx_ = EVP_MD_CTX_new();
}

void Encryption::cleanupOpenSSL() {
    if (cipherCtx_) {
        EVP_CIPHER_CTX_free(cipherCtx_);
        cipherCtx_ = nullptr;
    }
    if (mdCtx_) {
        EVP_MD_CTX_free(mdCtx_);
        mdCtx_ = nullptr;
    }
}

std::string Encryption::encryptAES(const std::string& plaintext, const std::string& key) {
    if (!cipherCtx_) {
        std::cerr << "Cipher context not initialized" << std::endl;
        return "";
    }
    
    // Generate random IV
    std::string iv = generateRandomIV();
    
    // Initialize encryption
    if (EVP_EncryptInit_ex(cipherCtx_, EVP_aes_256_cbc(), nullptr, 
                          reinterpret_cast<const unsigned char*>(key.c_str()),
                          reinterpret_cast<const unsigned char*>(iv.c_str())) != 1) {
        std::cerr << "Failed to initialize encryption" << std::endl;
        return "";
    }
    
    // Encrypt
    std::string ciphertext;
    ciphertext.resize(plaintext.length() + EVP_MAX_BLOCK_LENGTH);
    
    int len;
    if (EVP_EncryptUpdate(cipherCtx_, reinterpret_cast<unsigned char*>(&ciphertext[0]), &len,
                         reinterpret_cast<const unsigned char*>(plaintext.c_str()),
                         plaintext.length()) != 1) {
        std::cerr << "Failed to encrypt data" << std::endl;
        return "";
    }
    
    int finalLen;
    if (EVP_EncryptFinal_ex(cipherCtx_, reinterpret_cast<unsigned char*>(&ciphertext[len]), &finalLen) != 1) {
        std::cerr << "Failed to finalize encryption" << std::endl;
        return "";
    }
    
    ciphertext.resize(len + finalLen);
    
    // Prepend IV to ciphertext
    return iv + ciphertext;
}

std::string Encryption::decryptAES(const std::string& ciphertext, const std::string& key) {
    if (!cipherCtx_) {
        std::cerr << "Cipher context not initialized" << std::endl;
        return "";
    }
    
    if (ciphertext.length() <= IV_SIZE) {
        std::cerr << "Ciphertext too short" << std::endl;
        return "";
    }
    
    // Extract IV
    std::string iv = ciphertext.substr(0, IV_SIZE);
    std::string encryptedData = ciphertext.substr(IV_SIZE);
    
    // Initialize decryption
    if (EVP_DecryptInit_ex(cipherCtx_, EVP_aes_256_cbc(), nullptr,
                          reinterpret_cast<const unsigned char*>(key.c_str()),
                          reinterpret_cast<const unsigned char*>(iv.c_str())) != 1) {
        std::cerr << "Failed to initialize decryption" << std::endl;
        return "";
    }
    
    // Decrypt
    std::string plaintext;
    plaintext.resize(encryptedData.length());
    
    int len;
    if (EVP_DecryptUpdate(cipherCtx_, reinterpret_cast<unsigned char*>(&plaintext[0]), &len,
                         reinterpret_cast<const unsigned char*>(encryptedData.c_str()),
                         encryptedData.length()) != 1) {
        std::cerr << "Failed to decrypt data" << std::endl;
        return "";
    }
    
    int finalLen;
    if (EVP_DecryptFinal_ex(cipherCtx_, reinterpret_cast<unsigned char*>(&plaintext[len]), &finalLen) != 1) {
        std::cerr << "Failed to finalize decryption" << std::endl;
        return "";
    }
    
    plaintext.resize(len + finalLen);
    return plaintext;
}

std::string Encryption::generateRandomKey() {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, 255);
    
    std::string key;
    key.resize(KEY_SIZE);
    for (int i = 0; i < KEY_SIZE; i++) {
        key[i] = static_cast<char>(distribution(generator));
    }
    return key;
}

std::string Encryption::generateRandomIV() {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, 255);
    
    std::string iv;
    iv.resize(IV_SIZE);
    for (int i = 0; i < IV_SIZE; i++) {
        iv[i] = static_cast<char>(distribution(generator));
    }
    return iv;
}

std::string Encryption::generateRandomSalt() {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, 255);
    
    std::string salt;
    salt.resize(SALT_SIZE);
    for (int i = 0; i < SALT_SIZE; i++) {
        salt[i] = static_cast<char>(distribution(generator));
    }
    return salt;
}

std::string Encryption::hashPassword(const std::string& password) {
    std::string salt = generateRandomSalt();
    std::string hash = pbkdf2(password, salt, 10000);
    return salt + ":" + hash;
}

bool Encryption::verifyPassword(const std::string& password, const std::string& hash) {
    size_t colonPos = hash.find(':');
    if (colonPos == std::string::npos) {
        return false;
    }
    
    std::string salt = hash.substr(0, colonPos);
    std::string storedHash = hash.substr(colonPos + 1);
    
    std::string computedHash = pbkdf2(password, salt, 10000);
    return computedHash == storedHash;
}

std::string Encryption::pbkdf2(const std::string& password, const std::string& salt, int iterations) {
    std::string result;
    result.resize(32); // SHA-256 output size
    
    if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                          reinterpret_cast<const unsigned char*>(salt.c_str()), salt.length(),
                          iterations, EVP_sha256(), result.length(),
                          reinterpret_cast<unsigned char*>(&result[0])) != 1) {
        std::cerr << "PBKDF2 failed" << std::endl;
        return "";
    }
    
    return result;
}

std::string Encryption::generateJWT(const std::string& payload, const std::string& secret) {
    // Simple JWT implementation (header.payload.signature)
    std::string header = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    std::string encodedHeader = base64Encode(header);
    std::string encodedPayload = base64Encode(payload);
    
    std::string data = encodedHeader + "." + encodedPayload;
    std::string signature = hmacSha256(data, secret);
    std::string encodedSignature = base64Encode(signature);
    
    return data + "." + encodedSignature;
}

bool Encryption::verifyJWT(const std::string& token, const std::string& secret, std::string& payload) {
    size_t firstDot = token.find('.');
    size_t secondDot = token.find('.', firstDot + 1);
    
    if (firstDot == std::string::npos || secondDot == std::string::npos) {
        return false;
    }
    
    std::string header = token.substr(0, firstDot);
    std::string tokenPayload = token.substr(firstDot + 1, secondDot - firstDot - 1);
    std::string signature = token.substr(secondDot + 1);
    
    std::string data = header + "." + tokenPayload;
    std::string expectedSignature = hmacSha256(data, secret);
    std::string expectedEncodedSignature = base64Encode(expectedSignature);
    
    if (signature != expectedEncodedSignature) {
        return false;
    }
    
    payload = base64Decode(tokenPayload);
    return true;
}

std::string Encryption::base64Encode(const std::string& data) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, data.c_str(), data.length());
    BIO_flush(bio);
    
    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    
    std::string result(bufferPtr->data, bufferPtr->length);
    
    BIO_free_all(bio);
    return result;
}

std::string Encryption::base64Decode(const std::string& encoded) {
    BIO* bio = BIO_new_mem_buf(encoded.c_str(), encoded.length());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    
    std::string result;
    result.resize(encoded.length());
    
    int decodedLength = BIO_read(bio, &result[0], result.length());
    if (decodedLength > 0) {
        result.resize(decodedLength);
    } else {
        result.clear();
    }
    
    BIO_free_all(bio);
    return result;
}

std::string Encryption::generateHMAC(const std::string& data, const std::string& key) {
    return hmacSha256(data, key);
}

bool Encryption::verifyHMAC(const std::string& data, const std::string& key, const std::string& hmac) {
    std::string expectedHmac = hmacSha256(data, key);
    return hmac == expectedHmac;
}

std::string Encryption::sha256(const std::string& data) {
    // Use EVP interface for OpenSSL 3.0 compatibility
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        return "";
    }
    
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    
    if (EVP_DigestUpdate(ctx, data.c_str(), data.length()) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    
    EVP_MD_CTX_free(ctx);
    
    std::stringstream ss;
    for (unsigned int i = 0; i < hashLen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string Encryption::hmacSha256(const std::string& data, const std::string& key) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    
    HMAC(EVP_sha256(), key.c_str(), key.length(),
         reinterpret_cast<const unsigned char*>(data.c_str()), data.length(),
         hash, &hashLen);
    
    std::stringstream ss;
    for (unsigned int i = 0; i < hashLen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
} 