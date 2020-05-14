#include "MessageBuffer.h"
#include "DNSDatabase.h"
#include "Logger.h"

extern std::unique_ptr<DNSDatabase> db;
extern std::unique_ptr<Logger> logger;

MessageBuffer::MessageBuffer() : loadBalancer(1)
{
	_workingThread = std::thread([](MessageBuffer* mb) { mb->_WorkingThread(); }, this);
}

MessageBuffer::~MessageBuffer()
{
	Stop();
	Release();
}

void MessageBuffer::Store(Message& msg)
{
	queueLock.lock();
	msgQueue.push(msg);
	queueLock.unlock();
}

void MessageBuffer::Stop()
{
	tSignal.set_value();
	if (_workingThread.joinable())
		_workingThread.join();
	loadBalancer.Stop();
}

void MessageBuffer::Release()
{
	queueLock.lock();
	while (!msgQueue.empty())
	{
		auto qry = msgQueue.front().query;
		msgQueue.pop();
		if (qry != nullptr)
			delete[] qry;
	}
	queueLock.unlock();
}

void MessageBuffer::_WorkingThread()
{
	std::chrono::microseconds term(10);
	auto signal = tSignal.get_future();
	Message msg;
	while (signal.wait_for(term) == std::future_status::timeout) {
		queueLock.lock();
		if (msgQueue.empty()) {
			queueLock.unlock();
			continue;
		}
		msg = msgQueue.front();
		msgQueue.pop();
		queueLock.unlock();
		while (!loadBalancer.Select(msg) && signal.wait_for(term) == std::future_status::timeout);
	}
}

WorkerThread::WorkerThread() : available(true), running(true)
{
	currentWork = Message();
	_workingThread = std::thread([] (WorkerThread* wt) { wt->_WorkingThread(); }, this);
}

inline bool WorkerThread::isAvailable()
{
	return available;
}

void WorkerThread::GiveWork(Message msg)
{
	available = false;
	wLock.lock();
	currentWork = msg;
	wLock.unlock();
	cv.notify_one();
}

void WorkerThread::_WorkingThread()
{
	while (running) {
		std::unique_lock<std::mutex> lk(wLock);
		cv.wait(lk, [&] { return !available; });
		if (currentWork.query == nullptr)
		{
			available = true;
			continue;
		}
		currentWork.query[currentWork.length] = 0;
		logger->Log(currentWork.query);

		available = true;
		printf("work end");
		continue;

		int count;
		ULONG* addresses;
		std::string domain;
		if (db->DNSQuery(domain, &addresses, &count)) {

			delete[] addresses;
		}

		delete[] currentWork.query;
		currentWork.query = nullptr;
		available = true;
	}
	available = false;
}

void WorkerThread::Stop()
{
	if (!running)
		return;
	running = false;
	available = false;
	currentWork.query = nullptr;
	cv.notify_one();
	if (_workingThread.joinable())
		_workingThread.join();
}

LoadBalancer::LoadBalancer(int workerCount)
{
	for (int i = 0; i < workerCount; ++i)
	{
		workers.push_back(new WorkerThread());
	}
}

LoadBalancer::~LoadBalancer()
{
	Stop();
}

bool LoadBalancer::Select(Message& msg)
{
	for (auto worker : workers)
	{
		if (worker->isAvailable())
		{
			worker->GiveWork(msg);
			return true;
		}
	}
	return false;
}

void LoadBalancer::Stop()
{
	for (auto worker : workers)
	{
		worker->Stop();
		delete worker;
	}
	workers.clear();
}
