#include <muduo/base/Endian.h>
#include <gtest/gtest.h>
#include <limits>

using namespace muduo::base::endian;

TEST(Endian, Convert2Big) {

    auto signed_s = NativeToBig(std::numeric_limits<short>::max());
    auto signed_i = NativeToBig(std::numeric_limits<int>::max());
    auto signed_l = NativeToBig(std::numeric_limits<long>::max());
    auto signed_ll = NativeToBig(std::numeric_limits<long long>::max());

    uint16_t unsigned_s = NativeToBig(std::numeric_limits<unsigned short>::max());
    uint32_t unsigned_i = NativeToBig(std::numeric_limits<unsigned int>::max());
    uint64_t unsigned_l = NativeToBig(std::numeric_limits<unsigned long>::max());
    unsigned long long unsigned_ll = NativeToBig(std::numeric_limits<unsigned long long>::max());

    ASSERT_EQ(BigToNative(signed_s), std::numeric_limits<short>::max());
    ASSERT_EQ(BigToNative(signed_i), std::numeric_limits<int>::max());
    ASSERT_EQ(BigToNative(signed_l), std::numeric_limits<long>::max());
    ASSERT_EQ(BigToNative(signed_ll), std::numeric_limits<long long>::max());
    ASSERT_EQ(BigToNative(unsigned_s), std::numeric_limits<unsigned short>::max());
    ASSERT_EQ(BigToNative(unsigned_i), std::numeric_limits<unsigned int>::max());
    ASSERT_EQ(BigToNative(unsigned_l), std::numeric_limits<unsigned long>::max());
    ASSERT_EQ(BigToNative(unsigned_ll), std::numeric_limits<unsigned long long>::max());
}

TEST(Endian, Convert2Little) {

    auto signed_s = NativeToLittle(std::numeric_limits<short>::min());
    auto signed_i = NativeToLittle(std::numeric_limits<int>::min());
    auto signed_l = NativeToLittle(std::numeric_limits<long>::min());
    auto signed_ll = NativeToLittle(std::numeric_limits<long long>::min());

    uint16_t unsigned_s = NativeToLittle(std::numeric_limits<unsigned short>::min());
    uint32_t unsigned_i = NativeToLittle(std::numeric_limits<unsigned int>::min());
    uint64_t unsigned_l = NativeToLittle(std::numeric_limits<unsigned long>::min());
    unsigned long long unsigned_ll = NativeToLittle(std::numeric_limits<unsigned long long>::min());

    ASSERT_EQ(LittleToNative(signed_s), std::numeric_limits<short>::min());
    ASSERT_EQ(LittleToNative(signed_i), std::numeric_limits<int>::min());
    ASSERT_EQ(LittleToNative(signed_l), std::numeric_limits<long>::min());
    ASSERT_EQ(LittleToNative(signed_ll), std::numeric_limits<long long>::min());
    ASSERT_EQ(LittleToNative(unsigned_s), std::numeric_limits<unsigned short>::min());
    ASSERT_EQ(LittleToNative(unsigned_i), std::numeric_limits<unsigned int>::min());
    ASSERT_EQ(LittleToNative(unsigned_l), std::numeric_limits<unsigned long>::min());
    ASSERT_EQ(LittleToNative(unsigned_ll), std::numeric_limits<unsigned long long>::min());
}
