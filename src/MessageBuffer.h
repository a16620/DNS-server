#pragma once
#include "DNS.h"
#include <queue>
#include <mutex>
#include <atomic>
#include <future>

struct Message {
	ULONG host;
	USHORT hostPort;
	char* query;
};

class MessageBuffer
{
	std::queue<Message> msgQueue;
	std::mutex queueLock;
public:
	void Store(Message & msg);
};

class WorkerThread;

class LoadBalancer {
	std::list<WorkerThread*> workers;
};

class WorkerThread {
	std::atomic_bool available;
	std::condition_variable_any cv;
	std::promise<void> tSignal;
	Message currentWork;
public:
	WorkerThread(std::future<void>&& signal);
	inline bool isAvailable();
	void GiveWork(Message msg);
	void WorkingThread();
	void Stop();
};

class FreeLock {
public:
	void lock() {}
	void unlock() {}
};