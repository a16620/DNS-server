#include "MessageBuffer.h"
#include "UDPListener.h"
#include "DNSDatabase.h"
#include "Logger.h"

extern std::unique_ptr<DNSDatabase> db;
extern std::unique_ptr<Logger> logger;
extern UDPSocket udpSender;

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

WorkerThread::WorkerThread() : available(true), running(true), packetBuffer(nullptr, [](char* ptr) { delete[] ptr; })
{
	currentWork = Message();
	try {

		packetBuffer.reset(new char[WorkerBufferSize]);
	}
	catch (std::bad_alloc e) {
		logger->Log("워커 패킷 버퍼 할당 실패");
		throw std::runtime_error("워커 패킷 버퍼 할당 실패");
	}
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
	DNSHEADER header;
	sockaddr_in tad;
	memset(&tad, 0, sizeof(tad));
	tad.sin_family = AF_INET;
	while (running) {
		std::unique_lock<std::mutex> lk(wLock);
		cv.wait(lk, [&] { return !available; });
		if (currentWork.query == nullptr)
		{
			available = true;
			continue;
		}
		//currentWork.query[currentWork.length] = 0;

		std::vector<std::string> domains;
		if (Transcription::Interpret(currentWork.query, &domains, &header))
		{
			std::unordered_map<std::string, Transcription::DMAP> dmap;
			for (auto& domain : domains)
			{
				int count;
				ULONG* addresses;
				if (db->DNSQuery(domain, &addresses, &count)) {
					if (dmap.count(domain) == 0)
						dmap.insert(std::make_pair(domain, Transcription::DMAP{ count, addresses }));
					else
						delete[] addresses;
				}
			}
			int plen;
			if (Transcription::ParseTo(packetBuffer.get(), dmap, header.id, &plen))
			{
				//전송
				tad.sin_addr.s_addr = currentWork.host;
				tad.sin_port = currentWork.hostPort;
				if (udpSender.SendTo(packetBuffer.get(), plen, 0, (sockaddr*)&tad, sizeof(tad)) == -1)
				{
					char tnum[10];
					itoa(WSAGetLastError(), tnum, 10);
					logger->Log(tnum);
				}
			}

			for (auto dm : dmap)
			{
				delete[] dm.second.arr;
			}
		}

		memset(packetBuffer.get(), 0, WorkerBufferSize);

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
