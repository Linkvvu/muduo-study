#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <muduo/net/inetAddress.h>

BOOST_AUTO_TEST_CASE(the_inet_address_constructor) {
    using muduo::net::inet_address;
    using muduo::string;

    inet_address addr1(1234);
    BOOST_CHECK_EQUAL(addr1.ip(), string("0.0.0.0"));
    BOOST_CHECK_EQUAL(addr1.ipAndPort(), string("0.0.0.0:1234"));

    inet_address addr2("1.2.3.4", 8888);
    BOOST_CHECK_EQUAL(addr2.ip(), string("1.2.3.4"));
    BOOST_CHECK_EQUAL(addr2.ipAndPort(), string("1.2.3.4:8888"));

    inet_address addr3("255.255.255.255", 65535);
    BOOST_CHECK_EQUAL(addr3.ip(), string("255.255.255.255"));
    BOOST_CHECK_EQUAL(addr3.ipAndPort(), string("255.255.255.255:65535"));
}