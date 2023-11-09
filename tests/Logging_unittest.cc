#include <base/LogStream.h>
#include <base/Logging.h>
#include <gtest/gtest.h>


using namespace muduo::base;

TEST(LogStreamTests, Booleans) {
    LogStream log_stream;
    const auto& buf = log_stream.GetInternalBuf();
    EXPECT_EQ(buf.ToString(), "");
    log_stream << true;
    EXPECT_EQ(buf.ToString(), "true");
    log_stream << "\n";
    EXPECT_EQ(buf.ToString(), "true\n");
    log_stream << false;
    EXPECT_EQ(buf.ToString(), "true\nfalse");
}

TEST(LogStreamTests, Integers) {
    LogStream log_stream;
    const auto& buf = log_stream.GetInternalBuf();
    EXPECT_EQ(buf.ToString(), "");
    log_stream << 1;
    EXPECT_EQ(buf.ToString(), "1");
    log_stream << 0;
    EXPECT_EQ(buf.ToString(), "10");
    log_stream << -1;
    EXPECT_EQ(buf.ToString(), "10-1");
    log_stream.ResetBuffer();
    log_stream << 0 << " " << "123" << 'x' << 0x64;
    EXPECT_EQ(buf.ToString(), "0 123x100");
}

TEST(LogStreamTests, IntegerLimits) {
    LogStream log_stream;
    const auto& buf = log_stream.GetInternalBuf();
    log_stream << -2147483647;
    EXPECT_EQ(buf.ToString(), "-2147483647");
    log_stream << std::numeric_limits<int>::min();    
    EXPECT_EQ(buf.ToString(), "-2147483647-2147483648");
    log_stream << std::numeric_limits<int>::max();
    EXPECT_EQ(buf.ToString(), "-2147483647-21474836482147483647");
    log_stream.ResetBuffer();

    log_stream << std::numeric_limits<int16_t>::min();
    EXPECT_EQ(buf.ToString(), "-32768");
    log_stream.ResetBuffer();

    log_stream << std::numeric_limits<int16_t>::max();
    EXPECT_EQ(buf.ToString(), "32767");
    log_stream.ResetBuffer();

    log_stream << std::numeric_limits<uint16_t>::min();
    EXPECT_EQ(buf.ToString(), "0");
    log_stream.ResetBuffer();

    log_stream << std::numeric_limits<uint16_t>::max();
    EXPECT_EQ(buf.ToString(), "65535");
    log_stream.ResetBuffer();

    log_stream << std::numeric_limits<int32_t>::min();
    EXPECT_EQ(buf.ToString(), "-2147483648");
    log_stream.ResetBuffer();

    log_stream << std::numeric_limits<int32_t>::max();
    EXPECT_EQ(buf.ToString(), "2147483647");
    log_stream.ResetBuffer();

    log_stream << std::numeric_limits<uint32_t>::min();
    EXPECT_EQ(buf.ToString(), "0");
    log_stream.ResetBuffer();

    log_stream << std::numeric_limits<uint32_t>::max();
    EXPECT_EQ(buf.ToString(), "4294967295");
    log_stream.ResetBuffer();

    log_stream << std::numeric_limits<int64_t>::min();
    EXPECT_EQ(buf.ToString(), "-9223372036854775808");
    log_stream.ResetBuffer();

    log_stream << std::numeric_limits<int64_t>::max();
    EXPECT_EQ(buf.ToString(), "9223372036854775807");
    log_stream.ResetBuffer();

    log_stream << std::numeric_limits<uint64_t>::min();
    EXPECT_EQ(buf.ToString(), "0");
    log_stream.ResetBuffer();

    log_stream << std::numeric_limits<uint64_t>::max();
    EXPECT_EQ(buf.ToString(), "18446744073709551615");
    log_stream.ResetBuffer();
}

TEST(LogStreamTests, Floats) {
    LogStream log_stream;
    const auto& buf = log_stream.GetInternalBuf();

    log_stream << 0.0;
    EXPECT_EQ(buf.ToString(), "0");
    log_stream.ResetBuffer();

    log_stream << 1.0;
    EXPECT_EQ(buf.ToString(), "1");
    log_stream.ResetBuffer();

    log_stream << 0.1;
    EXPECT_EQ(buf.ToString(), "0.1");
    log_stream.ResetBuffer();

    log_stream << 0.05; 
    EXPECT_EQ(buf.ToString(), "0.05");
    log_stream.ResetBuffer();

    double x = 0.5;
    double y = 0.05;
    log_stream << x+y;
    EXPECT_EQ(buf.ToString(), "0.55");
    log_stream.ResetBuffer();

    log_stream << 1.123456789;
    EXPECT_EQ(buf.ToString(), "1.123456789");
    log_stream.ResetBuffer();

    log_stream << 1.123456;
    EXPECT_EQ(buf.ToString(), "1.123456");
    log_stream.ResetBuffer();

    log_stream << -1.123456;
    EXPECT_EQ(buf.ToString(), "-1.123456");
    log_stream.ResetBuffer();
}

TEST(LogStreamTests, Void) {
    LogStream log_stream;
    const auto& buf = log_stream.GetInternalBuf();

    log_stream << reinterpret_cast<void*>(0);
    EXPECT_EQ(buf.ToString(), "0x0");
    log_stream.ResetBuffer();

    log_stream << reinterpret_cast<void*>(8888);
    EXPECT_EQ(buf.ToString(), "0x22b8");
    log_stream.ResetBuffer();
}

TEST(LogStreamTests, Strings) {
    LogStream log_stream;
    const auto& buf = log_stream.GetInternalBuf();

    log_stream << "Hello ";
    EXPECT_EQ(buf.ToString(), "Hello ");

    std::string muduo = "Muduo";
    log_stream << muduo;
    EXPECT_EQ(buf.ToString(), "Hello Muduo");
}

TEST(LogStreamTests, Length) {
    LogStream log_stream;
    const auto& buf = log_stream.GetInternalBuf();

    for (int i = 0; i < static_cast<int>(LogStream::kSmallBuffer) / 10; ++i) {
        log_stream << "123456789 ";
        EXPECT_EQ(buf.GetLength(), 10*(i+1));
        EXPECT_EQ(buf.Avail(), LogStream::kSmallBuffer - 10*(i+1));
    }
}

TEST(LoggerTests, print) {
    muduo::SetOutputHandler([](const char* data, size_t len) {
        std::cout.write(data, len);
    }, false);
    LOG_TRACE << "TRACE";
    LOG_DEBUG << "DEBUG";
    LOG_INFO << "INFO";
    LOG_WARN << "WARNING";
    LOG_ERROR << "ERROR";
    LOG_FATAL << "FATAL";
}

