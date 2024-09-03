#include "KeyManagement.h"
#include <openssl/pem.h>
#include <openssl/err.h>
#include <stdexcept>

void KeyManagement::generateKeys(EVP_PKEY **privateKey, EVP_PKEY **publicKey) {
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, NULL);
    if (!ctx) {
        throw std::runtime_error("Failed to create key context");
    }
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize key generation");
    }
    if (EVP_PKEY_keygen(ctx, privateKey) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to generate private key");
    }
    *publicKey = EVP_PKEY_dup(*privateKey);
    EVP_PKEY_CTX_free(ctx);
}

void KeyManagement::savePrivateKey(EVP_PKEY *key, const std::string &filename) {
    FILE *pkey_file = fopen(filename.c_str(), "wb");
    if (!pkey_file) {
        throw std::runtime_error("Failed to open file for writing private key");
    }
    if (PEM_write_PrivateKey(pkey_file, key, NULL, NULL, 0, NULL, NULL) != 1) {
        fclose(pkey_file);
        throw std::runtime_error("Failed to write private key");
    }
    fclose(pkey_file);
}

void KeyManagement::savePublicKey(EVP_PKEY *key, const std::string &filename) {
    FILE *pkey_file = fopen(filename.c_str(), "wb");
    if (!pkey_file) {
        throw std::runtime_error("Failed to open file for writing public key");
    }
    if (PEM_write_PUBKEY(pkey_file, key) != 1) {
        fclose(pkey_file);
        throw std::runtime_error("Failed to write public key");
    }
    fclose(pkey_file);
}

EVP_PKEY* KeyManagement::loadPrivateKey(const std::string &filename) {
    FILE *pkey_file = fopen(filename.c_str(), "rb");
    if (!pkey_file) {
        throw std::runtime_error("Failed to open file for reading private key");
    }
    EVP_PKEY *pkey = PEM_read_PrivateKey(pkey_file, NULL, NULL, NULL);
    fclose(pkey_file);
    if (!pkey) {
        throw std::runtime_error("Failed to read private key");
    }
    return pkey;
}

EVP_PKEY* KeyManagement::loadPublicKey(const std::string &filename) {
    FILE *pkey_file = fopen(filename.c_str(), "rb");
    if (!pkey_file) {
        throw std::runtime_error("Failed to open file for reading public key");
    }
    EVP_PKEY *pkey = PEM_read_PUBKEY(pkey_file, NULL, NULL, NULL);
    fclose(pkey_file);
    if (!pkey) {
        throw std::runtime_error("Failed to read public key");
    }
    return pkey;
}

int KeyManagement::signMessage(EVP_PKEY *privateKey, const unsigned char *msg, 
                               size_t msgLen, unsigned char **sig, size_t *sigLen) {
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) return -1;

    if (EVP_DigestSignInit(mdctx, NULL, NULL, NULL, privateKey) <= 0) {
        EVP_MD_CTX_free(mdctx);
        return -1;
    }

    if (EVP_DigestSign(mdctx, NULL, sigLen, msg, msgLen) <= 0) {
        EVP_MD_CTX_free(mdctx);
        return -1;
    }

    *sig = (unsigned char *)OPENSSL_malloc(*sigLen);
    if (!(*sig)) {
        EVP_MD_CTX_free(mdctx);
        return -1;
    }

    if (EVP_DigestSign(mdctx, *sig, sigLen, msg, msgLen) <= 0) {
        OPENSSL_free(*sig);
        EVP_MD_CTX_free(mdctx);
        return -1;
    }

    EVP_MD_CTX_free(mdctx);
    return 1;
}

int KeyManagement::verifyMessage(EVP_PKEY *publicKey, const unsigned char *msg, 
                                 size_t msgLen, const unsigned char *sig, size_t sigLen) {
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) return -1;

    if (EVP_DigestVerifyInit(mdctx, NULL, NULL, NULL, publicKey) <= 0) {
        EVP_MD_CTX_free(mdctx);
        return -1;
    }

    int ret = EVP_DigestVerify(mdctx, sig, sigLen, msg, msgLen);

    EVP_MD_CTX_free(mdctx);
    return ret;
}
