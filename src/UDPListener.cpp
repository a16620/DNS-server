#include "UDPListener.h"
#include "Logger.h"
#include <chrono>

extern std::unique_ptr<DNSDatabase> db;
extern std::unique_ptr<Logger> logger;

UDPListener::UDPListener(ServerConfig config, MessageBuffer* buffer)
{
	this->config = config;
	output = buffer;

	listener = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (listener == INVALID_SOCKET) {
		throw std::runtime_error("소켓 생성 실패");
	}

	int option = 1;
	setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (char*)&option, sizeof(option));

	sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(config.port);
	serverAddr.sin_addr.s_addr = config.address;

	if (bind(listener, (sockaddr*)&serverAddr, sizeof(sockaddr_in))) {
		closesocket(listener);
		throw std::runtime_error("소켓 바인딩 실패");
	}

	std::promise<bool> initP;
	auto inited = initP.get_future();
	receiving = std::thread([=](std::future<void>&& signal, std::promise<bool>&& isInited) {
		std::chrono::microseconds term(config.minRecvTerm);
		std::unique_ptr<char, std::function<void(char*)>> msgBuffer(nullptr, [](char* block) {
			delete[] block;
			});

		try {
			msgBuffer.reset(new char[1024]);
			isInited.set_value(true);
		}
		catch (std::bad_alloc e) {
			isInited.set_value(false);
			return;
		}

		sockaddr_in host;
		int hostLen = sizeof(host);
		Message msg;
		while (signal.wait_for(term) == std::future_status::timeout) {
			int rBytes = recvfrom(listener, msgBuffer.get(), 512, 0, (sockaddr*)&host, &hostLen);
			if (rBytes < 1)
				continue;
			if (!db->AuthQuery(host.sin_addr.s_addr))
				continue;
			
			msg.host = host.sin_addr.s_addr;
			msg.hostPort = host.sin_port;
			msg.length = rBytes;
			try {
				char* cpy = new char[rBytes];
				memcpy(cpy, msgBuffer.get(), rBytes);
				msg.query = cpy;
				output->Store(msg);
			}
			catch (std::bad_alloc e) {
				logger->Log("패킷 전달 메모리 할당 실패");
				char b[10];
				itoa(rBytes, b, 10);
				logger->Log(b);
			}
		}
		}, std::move(tSignal.get_future()), std::move(initP));

	if (!inited.get()) {
		closesocket(listener);
		throw std::runtime_error("패킷 저장 메모리 할당 실패");
	}
	running = true;
}

UDPListener::~UDPListener()
{
	Stop();
}

ServerConfig UDPListener::Config()
{
	return config;
}

void UDPListener::Stop()
{
	if (!running)
		return;

	running = false;
	tSignal.set_value();
	closesocket(listener);
	if (receiving.joinable())
		receiving.join();
}
