#include <muduo/base/logStream.h>
#include <muduo/base/Types.h>
#include <limits>
#include <stdint.h>

//#define BOOST_TEST_MODULE LogStreamTest
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using muduo::string;

BOOST_AUTO_TEST_CASE(testLogStreamBooleans) {
    muduo::logStream os;
    const muduo::logStream::buffer_t& buf = os.buffer();
    BOOST_CHECK_EQUAL(buf.to_string_t(), string(""));
    os << true;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("1"));
    os << '\n';
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("1\n"));
    os << false;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("1\n0"));
}

BOOST_AUTO_TEST_CASE(testLogStreamIntegers) {
    muduo::logStream os;
    const muduo::logStream::buffer_t& buf = os.buffer();
    BOOST_CHECK_EQUAL(buf.to_string_t(), string(""));
    os << 1;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("1"));
    os << 0;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("10"));
    os << -1;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("10-1"));
    os.reset_buffer();

    os << 0 << " " << 123 << 'x' << 0x64;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("0 123x100"));
}

BOOST_AUTO_TEST_CASE(testLogStreamIntegerLimits) {
    muduo::logStream os;
    const muduo::logStream::buffer_t& buf = os.buffer();
    os << -2147483647;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("-2147483647"));
    os << static_cast<int>(-2147483647 - 1);
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("-2147483647-2147483648"));
    os << ' ';
    os << 2147483647;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("-2147483647-2147483648 2147483647"));
    os.reset_buffer();

    os << std::numeric_limits<int16_t>::min();
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("-32768"));
    os.reset_buffer();

    os << std::numeric_limits<int16_t>::max();
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("32767"));
    os.reset_buffer();

    os << std::numeric_limits<uint16_t>::min();
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("0"));
    os.reset_buffer();

    os << std::numeric_limits<uint16_t>::max();
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("65535"));
    os.reset_buffer();

    os << std::numeric_limits<int32_t>::min();
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("-2147483648"));
    os.reset_buffer();

    os << std::numeric_limits<int32_t>::max();
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("2147483647"));
    os.reset_buffer();

    os << std::numeric_limits<uint32_t>::min();
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("0"));
    os.reset_buffer();

    os << std::numeric_limits<uint32_t>::max();
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("4294967295"));
    os.reset_buffer();

    os << std::numeric_limits<int64_t>::min();
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("-9223372036854775808"));
    os.reset_buffer();

    os << std::numeric_limits<int64_t>::max();
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("9223372036854775807"));
    os.reset_buffer();

    os << std::numeric_limits<uint64_t>::min();
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("0"));
    os.reset_buffer();

    os << std::numeric_limits<uint64_t>::max();
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("18446744073709551615"));
    os.reset_buffer();

    int16_t a = 0;
    int32_t b = 0;
    int64_t c = 0;
    os << a;
    os << b;
    os << c;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("000"));
}

BOOST_AUTO_TEST_CASE(testLogStreamFloats) {
    // muduo::logStream os;
    // const muduo::logStream::buffer_t& buf = os.buffer();

    // os << 0.0;
    // BOOST_CHECK_EQUAL(buf.to_string_t(), string("0.00"));
    // os.reset_buffer();

    // os << 1.0;
    // BOOST_CHECK_EQUAL(buf.to_string_t(), string("1.00"));
    // os.reset_buffer();

    // os << 0.1;
    // BOOST_CHECK_EQUAL(buf.to_string_t(), string("0.10"));
    // os.reset_buffer();

    // os << 0.05;
    // BOOST_CHECK_EQUAL(buf.to_string_t(), string("0.05"));
    // os.reset_buffer();

    // os << 0.15;
    // BOOST_CHECK_EQUAL(buf.to_string_t(), string("0.15"));
    // os.reset_buffer();

    // double a = 0.1;
    // os << a;
    // BOOST_CHECK_EQUAL(buf.to_string_t(), string("0.10"));
    // os.reset_buffer();

    // double b = 0.05;
    // os << b;
    // BOOST_CHECK_EQUAL(buf.to_string_t(), string("0.05"));
    // os.reset_buffer();

    // double c = 0.15;
    // os << c;
    // BOOST_CHECK_EQUAL(buf.to_string_t(), string("0.15"));
    // os.reset_buffer();

    // os << a + b;
    // BOOST_CHECK_EQUAL(buf.to_string_t(), string("0.15"));
    // os.reset_buffer();

    // BOOST_CHECK(a + b != c);

    // os << 1.23456789;
    // BOOST_CHECK_EQUAL(buf.to_string_t(), string("1.23"));
    // os.reset_buffer();

    // os << 1.234567;
    // BOOST_CHECK_EQUAL(buf.to_string_t(), string("1.23"));
    // os.reset_buffer();

    // os << -123.456;
    // BOOST_CHECK_EQUAL(buf.to_string_t(), string("-123.46"));
    // os.reset_buffer();

    muduo::logStream os;
    const muduo::logStream::buffer_t& buf = os.buffer();
    
    os << 0.0;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("0"));
    os.reset_buffer();

    os << 1.0;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("1"));
    os.reset_buffer();

    os << 0.1;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("0.1"));
    os.reset_buffer();

    os << 0.05;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("0.05"));
    os.reset_buffer();

    os << 0.15;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("0.15"));
    os.reset_buffer();

    double a = 0.1;
    os << a;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("0.1"));
    os.reset_buffer();

    double b = 0.05;
    os << b;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("0.05"));
    os.reset_buffer();

    double c = 0.15;
    os << c;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("0.15"));
    os.reset_buffer();

    os << a+b;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("0.15"));
    os.reset_buffer();

    BOOST_CHECK(a+b != c);

    os << 1.23456789;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("1.23456789"));
    os.reset_buffer();

    os << 1.234567;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("1.234567"));
    os.reset_buffer();

    os << -123.456;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("-123.456"));
    os.reset_buffer();    
}

BOOST_AUTO_TEST_CASE(testLogStreamVoid) {
    muduo::logStream os;
    const muduo::logStream::buffer_t& buf = os.buffer();

    os << static_cast<void*>(0);
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("0x0"));
    os.reset_buffer();

    os << reinterpret_cast<void*>(8888);
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("0x22b8"));
    os.reset_buffer();
}

BOOST_AUTO_TEST_CASE(testLogStreamStrings) {
    muduo::logStream os;
    const muduo::logStream::buffer_t& buf = os.buffer();

    os << "Hello ";
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("Hello "));

    string chenshuo = "Shuo Chen";
    os << chenshuo;
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("Hello Shuo Chen"));
}

BOOST_AUTO_TEST_CASE(testLogStreamFmts) {
    muduo::logStream os;
    const muduo::logStream::buffer_t& buf = os.buffer();

    os << muduo::Fmt("%4d", 1);
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("   1"));
    os.reset_buffer();

    os << muduo::Fmt("%5.2f", 1.2);
    BOOST_CHECK_EQUAL(buf.to_string_t(), string(" 1.20"));
    os.reset_buffer();

    os << muduo::Fmt("%4.2f", 1.2) << muduo::Fmt("%4d", 43);
    BOOST_CHECK_EQUAL(buf.to_string_t(), string("1.20  43"));
    os.reset_buffer();
}

BOOST_AUTO_TEST_CASE(testLogStreamLong) {
    muduo::logStream os;
    const muduo::logStream::buffer_t& buf = os.buffer();
    for (int i = 0; i < 399; ++i) {
        os << "123456789 ";
        BOOST_CHECK_EQUAL(buf.size(), 10 * (i + 1));
        BOOST_CHECK_EQUAL(buf.free_size(), 4000 - 10 * (i + 1));
    }

    os << "abcdefghi ";
    BOOST_CHECK_EQUAL(buf.size(), 3990);
    BOOST_CHECK_EQUAL(buf.free_size(), 10);

    os << "abcdefghi";
    BOOST_CHECK_EQUAL(buf.size(), 3999);
    BOOST_CHECK_EQUAL(buf.free_size(), 1);
}
