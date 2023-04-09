#include <muduo/base/blockingQueue.h>
#include <muduo/base/countDownLatch.h>
#include <muduo/base/Thread.h>
#include <muduo/base/timeStamp.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <map>
#include <string>
#include <stdio.h>

class Bench {
public:
	Bench(int numThreads)
		: latch_(numThreads)
		, threads_(numThreads) {
		for (int i = 0; i < numThreads; ++i) {
			char name[32];
			snprintf(name, sizeof name, "work thread %d", i);
			threads_.push_back(new muduo::Thread(
				boost::bind(&Bench::threadFunc, this), muduo::string(name)));
		}
		for_each(threads_.begin(), threads_.end(), boost::bind(&muduo::Thread::start, _1));
	}

	void run(int times) {
		printf("waiting for count down latch\n");
		latch_.wait();
		printf("all threads started\n");
		for (int i = 0; i < times; ++i) {
			muduo::TimeStamp now(muduo::TimeStamp::now());
			queue_.put(now);
			usleep(1000);
		}
	}

	void joinAll() {
		for (size_t i = 0; i < threads_.size(); ++i) {
			queue_.put(muduo::TimeStamp::invalid());
		}

		for_each(threads_.begin(), threads_.end(), boost::bind(&muduo::Thread::join, _1));
	}

private:

	void threadFunc() {
		printf("tid=%d, %s started\n", muduo::currentThread::tid(), muduo::currentThread::name());

		std::map<int, int> delays;
		latch_.countdown();
		bool running = true;
		while (running) {
			muduo::TimeStamp t(queue_.take());
			muduo::TimeStamp now(muduo::TimeStamp::now());
			if (t.valid()) {
				int delay = static_cast<int>(timeDifference(now, t) * 1000000); // Convert seconds to microseconds
				// printf("tid=%d, latency = %d us\n",
				//        muduo::CurrentThread::tid(), delay);
				++delays[delay];
			}
			running = t.valid();
		}

		printf("tid=%d, %s stopped\n", muduo::currentThread::tid(), muduo::currentThread::name());
		for (std::map<int, int>::iterator it = delays.begin(); it != delays.end(); ++it) {
			printf("tid = %d, delay = %d, count = %d\n", muduo::currentThread::tid(), it->first, it->second);
		}
	}

private:
	muduo::blockingQueue<muduo::TimeStamp> queue_;
	muduo::countDown_latch latch_;
	boost::ptr_vector<muduo::Thread> threads_;
};

int main(int argc, char* argv[]) {
	int threads = argc > 1 ? atoi(argv[1]) : 1;

	Bench t(threads);
	t.run(100);
	t.joinAll();
}