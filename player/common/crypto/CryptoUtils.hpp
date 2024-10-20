#pragma once

#include <cstdint>
#include <cryptopp/rsa.h>
#include <cryptopp/osrng.h>
#include <cryptopp/files.h>
#include <cryptopp/base64.h>

class FilePath;

struct RsaKeyPair
{
    CryptoPP::RSA::PublicKey publicKey;
    CryptoPP::RSA::PrivateKey privateKey;
};

namespace CryptoUtils
{
    RsaKeyPair generateRsaKeys(unsigned int keyLength);
    RsaKeyPair loadRsaKeys(const FilePath& publicKeyPath, const FilePath& privateKeyPath);
    void saveRsaKeys(const RsaKeyPair& keys, const FilePath& publicKeyPath, const FilePath& privateKeyPath);

    std::string decryptPrivateKeyPkcs(const std::string& message, const CryptoPP::RSA::PrivateKey& key);
    std::string decryptRc4(const std::string& message, const std::string& key);

    std::string toBase64(const std::string& text);
    std::string fromBase64(const std::string& text);

    template <typename Key>
    std::string keyToString(const Key& key)
    {
        std::string resultString;
        CryptoPP::Base64Encoder encoder(new CryptoPP::StringSink(resultString), true);
        key.DEREncode(encoder);
        return resultString;
    }
}