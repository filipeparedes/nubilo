#include <auth/passwordHash.h>

#include <gtest/gtest.h>

TEST(PasswordHashTest, EncodePasswordSucceeds) {
    auto result = nubilo::encodePassword("password123");

    ASSERT_TRUE(result.has_value());
}

TEST(PasswordHashTest, TwoHashesOfSamePasswordAreDifferent) {
    auto first = nubilo::encodePassword("password123");
    auto second = nubilo::encodePassword("password123");

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());

    ASSERT_FALSE(first.value() == second.value());
}

TEST(PasswordHashTest, VerifyAcceptsCorrectPassword) {
    auto result = nubilo::encodePassword("password123");
    ASSERT_TRUE(result.has_value());

    bool verified = nubilo::verifyPassword(result.value(), "password123");
    ASSERT_TRUE(verified);
}

TEST(PasswordHashTest, VerifyRejectsWrongPassword) {
    auto result = nubilo::encodePassword("password123");
    ASSERT_TRUE(result.has_value());

    bool verified = nubilo::verifyPassword(result.value(), "123password");
    ASSERT_FALSE(verified);
}