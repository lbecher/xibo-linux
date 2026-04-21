#include "Md5Hash.hpp"

#include "common/fs/FileSystem.hpp"

#include <array>
#include <boost/format.hpp>
#include <memory>
#include <openssl/evp.h>
#include <sstream>
#include <stdexcept>

Md5Hash Md5Hash::fromString(std::string_view data)
{
    std::array<unsigned char, EVP_MAX_MD_SIZE> result{};
    unsigned int resultSize = 0;
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    constexpr unsigned int md5ResultSize = 16;

    if (!context
        || EVP_DigestInit_ex(context.get(), EVP_md5(), nullptr) != 1
        || EVP_DigestUpdate(context.get(), data.data(), data.size()) != 1
        || EVP_DigestFinal_ex(context.get(), result.data(), &resultSize) != 1
        || resultSize != md5ResultSize)
    {
        throw std::runtime_error("Failed to calculate MD5 hash");
    }

    std::stringstream stream;
    for (unsigned int i = 0; i < resultSize; ++i)
    {
        stream << boost::format("%02x") % static_cast<short>(result[i]);
    }
    return Md5Hash{stream.str()};
}

Md5Hash Md5Hash::fromFile(const FilePath& path)
{
    auto fileContent = FileSystem::readFromFile(path);

    return Md5Hash::fromString(fileContent);
}

bool operator==(const Md5Hash& first, const Md5Hash& second)
{
    return static_cast<std::string>(first) == static_cast<std::string>(second);
}

bool operator!=(const Md5Hash& first, const Md5Hash& second)
{
    return !(first == second);
}

std::ostream& operator<<(std::ostream& out, const Md5Hash& hash)
{
    return out << static_cast<std::string>(hash);
}
