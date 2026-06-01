#include <gtest/gtest.h>

#include "common/crypto/CryptoUtils.hpp"

TEST(CryptoUtils, FromBase64TrimsPaddingBytes)
{
    ASSERT_EQ("hello", CryptoUtils::fromBase64("aGVsbG8="));
    ASSERT_EQ("hi", CryptoUtils::fromBase64("aGk="));
}

TEST(CryptoUtils, Base64RoundTripPreservesBinaryNuls)
{
    std::string original{"a\0b", 3};

    ASSERT_EQ(original, CryptoUtils::fromBase64(CryptoUtils::toBase64(original)));
}
