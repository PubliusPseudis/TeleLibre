#ifndef KEYMANAGEMENT_H
#define KEYMANAGEMENT_H

#include <openssl/evp.h>
#include <string>

class KeyManagement {
public:
    static void generateKeys(EVP_PKEY **privateKey, EVP_PKEY **publicKey);
    static void savePrivateKey(EVP_PKEY *key, const std::string &filename);
    static void savePublicKey(EVP_PKEY *key, const std::string &filename);
    static EVP_PKEY* loadPrivateKey(const std::string &filename);
    static EVP_PKEY* loadPublicKey(const std::string &filename);
    static int signMessage(EVP_PKEY *privateKey, const unsigned char *msg, 
                           size_t msgLen, unsigned char **sig, size_t *sigLen);
    static int verifyMessage(EVP_PKEY *publicKey, const unsigned char *msg, 
                             size_t msgLen, const unsigned char *sig, size_t sigLen);
};

#endif
