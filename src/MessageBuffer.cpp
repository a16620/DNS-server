#include "MessageBuffer.h"
#include "DNSDatabase.h"

extern std::unique_ptr<DNSDatabase> db;

void MessageBuffer::Store(Message& msg)
{
	queueLock.lock();
	msgQueue.push(msg);
	queueLock.unlock();
}

WorkerThread::WorkerThread(std::future<void>&& signal) : available(true), tSignal(std::move(signal))
{
	currentWork = Message();
}

inline bool WorkerThread::isAvailable()
{
	return available;
}

void WorkerThread::GiveWork(Message msg)
{
	available = false;
	currentWork = msg;
	cv.notify_one();
}

void WorkerThread::WorkingThread()
{
	std::chrono::microseconds term(5);
	FreeLock freelock;
	auto signal = tSignal.get_future();
	while (signal.wait_for(term) == std::future_status::timeout) {
		cv.wait(freelock, [&] { return !available; });
		if (currentWork.query == nullptr)
			continue;

		int count;
		ULONG* addresses;
		std::string domain;

		if (db->DNSQuery(domain, &addresses, &count)) {

			delete[] addresses;
		}

		delete[] currentWork.query;
		available = false;
	}
}

void WorkerThread::Stop()
{
	tSignal.set_value();
	available = false;
	currentWork.query = nullptr;
	cv.notify_one();
}
