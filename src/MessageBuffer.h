#pragma once
#include "DNS.h"
#include <queue>
#include <mutex>
#include <atomic>
#include <future>
#include <vector>

struct Message {
	ULONG host;
	USHORT hostPort;
	USHORT length;
	char* query;
};

class WorkerThread;

class LoadBalancer {
	std::vector<WorkerThread*> workers;
public:
	LoadBalancer(int workerCount);
	~LoadBalancer();
	bool Select(Message& msg);
	void Stop();
};

class MessageBuffer
{
	std::queue<Message> msgQueue;
	std::mutex queueLock;
	LoadBalancer loadBalancer;
	std::promise<void> tSignal;
	std::thread _workingThread;
public:
	void _WorkingThread();
	MessageBuffer();
	~MessageBuffer();
	void Store(Message & msg);
	void Stop();
	void Release();
};

class WorkerThread {
	std::atomic_bool available, running;
	std::condition_variable cv;
	std::mutex wLock;
	Message currentWork;
	std::thread _workingThread;
public:
	void _WorkingThread();
	WorkerThread();
	inline bool isAvailable();
	void GiveWork(Message msg);
	void Stop();
};
