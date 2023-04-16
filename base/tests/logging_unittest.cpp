#include <muduo/base/logging.h>
#include <muduo/base/threadpool.h>
#include <muduo/base/logFile.h>
#include <muduo/base/timeStamp.h>
#include <iostream>

using namespace muduo;

uint64_t g_total;
FILE* g_file;
std::unique_ptr<muduo::logFile> g_logFile;

void dummyOutput(const char* msg, std::size_t len) {
	g_total += len;
	if (g_file) {
		fwrite(msg, 1, len, g_file);
	} else if (g_logFile) {
		g_logFile->append(msg, len);
	}
}

void bench(const char* type) {
	g_total = 0;
	muduo::logger::setoutput_func(dummyOutput);
	muduo::TimeStamp start(muduo::TimeStamp::now());

	int n = 1000 * 1000;
	const bool kLongLog = false;
	muduo::string empty = " ";
	muduo::string longStr(3000, 'X');
	longStr += " ";
	for (int i = 0; i < n; ++i) {
		LOG_INFO << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz"
			<< (kLongLog ? longStr : empty)
			<< i;
	}
	muduo::TimeStamp end(muduo::TimeStamp::now());
	double seconds = timeDifference(end, start);
	printf("%-12s: %f seconds, %lu bytes, %10.2f msg/s, %.2f MiB/s\n",
		type, seconds, g_total, n / seconds, g_total / seconds / (1024 * 1024));
}

void logInThread() {
	LOG_DEBUG << "MSG<logInThread>MSG";
	usleep(1000);
}

int main() {
	threadpool pool("main_pool");
	pool.start(5);
	pool.run(logInThread);
	pool.run(logInThread);
	pool.run(logInThread);
	pool.run(logInThread);
	pool.run(logInThread);

	LOG_TRACE << "trace";
	LOG_DEBUG << "debug";
	LOG_INFO << "Hello";
	LOG_WARN << "World";
	LOG_ERROR << "Error";

	LOG_INFO << sizeof(muduo::logger);
	LOG_INFO << sizeof(muduo::logStream);
	LOG_INFO << sizeof(muduo::Fmt);
	LOG_INFO << sizeof(muduo::logStream::buffer_t);

	sleep(1);
	bench("non-op");

	char buffer[64 * 1024] = {};

	g_file = fopen("/dev/null", "w");
	setbuffer(g_file, buffer, sizeof buffer);
	bench("/dev/null");
	fclose(g_file);

	g_file = fopen("/tmp/log", "w");
	setbuffer(g_file, buffer, sizeof buffer);
	bench("/tmp/log");
	fclose(g_file);

	g_file = nullptr;

	g_logFile.reset(new muduo::logFile("log_non-thread-safely", 500 * 1000 * 1000, false));
	bench("test_log_non_thread-safely");

	g_logFile.reset(new muduo::logFile("log_thread-safely", 500 * 1000 * 1000, true));
	bench("test_log_thread-safely");
	g_logFile.reset();
}