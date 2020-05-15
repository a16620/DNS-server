#pragma once
#include "DNS.h"
#include "MessageBuffer.h"
#include "DNSDatabase.h"
#include <thread>
#include <future>
#pragma comment(lib, "ws2_32.lib")

struct ServerConfig {
	ULONG address;
	USHORT port;
	ULONG minRecvTerm;
};


class UDPListener
{
	SOCKET listener;
	ServerConfig config;
	MessageBuffer* output;
	std::thread receiving;
	std::promise<void> tSignal;
	bool running;
public:
	UDPListener(ServerConfig config, MessageBuffer* buffer);
	~UDPListener();
	ServerConfig Config();
	void Stop();
};

class UDPSocket {
	std::mutex lock;
public:
	SOCKET s;
	UDPSocket()
	{
		s = INVALID_SOCKET;
	}
	UDPSocket(SOCKET _s)
	{
		s = _s;
	}
	~UDPSocket()
	{
		//closesocket(s);
	}
	inline int SendTo(const char FAR* buf, int len, int flags, const struct sockaddr FAR* to, int tolen)
	{
		std::lock_guard<std::mutex> lk(lock);
		return sendto(s, buf, len, flags, to, tolen);
	}
};