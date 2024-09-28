#include "CryptoUtils.hpp"

#include "common/fs/FilePath.hpp"

// We use weak ARC4 due to CMS restrictions
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include <boost/beast/core/detail/base64.hpp>
#include <cryptopp/arc4.h>
#include <cryptopp/files.h>
#include <cryptopp/osrng.h>

RsaKeyPair CryptoUtils::generateRsaKeys(unsigned int keyLength)
{
    CryptoPP::AutoSeededRandomPool rng;

    CryptoPP::InvertibleRSAFunction params;
    params.GenerateRandomWithKeySize(rng, keyLength);

    CryptoPP::RSA::PublicKey publicKey{params};
    CryptoPP::RSA::PrivateKey privateKey{params};

    return {publicKey, privateKey};
}

RsaKeyPair CryptoUtils::loadRsaKeys(const FilePath& publicKeyPath, const FilePath& privateKeyPath)
{
    CryptoPP::FileSource fs1{publicKeyPath.c_str(), true};
    CryptoPP::FileSource fs2{privateKeyPath.c_str(), true};
    
    CryptoPP::RSA::PublicKey publicKey;
    CryptoPP::RSA::PrivateKey privateKey;

    publicKey.BERDecode(fs1);
    privateKey.BERDecode(fs2);

    return {publicKey, privateKey};
}

void CryptoUtils::saveRsaKeys(const RsaKeyPair& keys, const FilePath& publicKeyPath, const FilePath& privateKeyPath)
{
    CryptoPP::FileSink fs1{publicKeyPath.c_str()};
    CryptoPP::FileSink fs2{privateKeyPath.c_str()};

    keys.publicKey.DEREncode(fs1);
    keys.privateKey.DEREncode(fs2);
}

std::string CryptoUtils::decryptPrivateKeyPkcs(const std::string& message, const CryptoPP::RSA::PrivateKey& key)
{
    std::string dectypedMessage;
    CryptoPP::AutoSeededRandomPool rng;
    CryptoPP::RSAES_PKCS1v15_Decryptor decryptor{key};
    CryptoPP::StringSource{
        message, true, new CryptoPP::PK_DecryptorFilter{rng, decryptor, new CryptoPP::StringSink{dectypedMessage}}};

    return dectypedMessage;
}

// TODO: output message should not be created from input message
// TODO: key should be const unsigned char*
std::string CryptoUtils::decryptRc4(const std::string& message, const std::string& key)
{
    auto messageRaw = reinterpret_cast<unsigned char*>(const_cast<char*>(message.c_str()));
    auto keyRaw = reinterpret_cast<unsigned char*>(const_cast<char*>(key.c_str()));

    CryptoPP::Weak::ARC4 rc4{keyRaw, key.size()};
    rc4.ProcessString(messageRaw, message.size());

    return message;
}

std::string CryptoUtils::toBase64(const std::string& text)
{
    std::string base64;
    base64.resize(boost::beast::detail::base64::encoded_size(text.size()));
    boost::beast::detail::base64::encode(&base64[0], text.data(), text.size());
    return base64;
}

std::string CryptoUtils::fromBase64(const std::string& text)
{
    std::string decoded;
    decoded.resize(boost::beast::detail::base64::decoded_size(text.size()));
    boost::beast::detail::base64::decode(&decoded[0], text.data(), text.size());
    return decoded;
}