#pragma once
#include "DNS.h"
#include "MessageBuffer.h"
#include "DNSDatabase.h"
#include <thread>
#include <future>
#include <WinSock2.h>
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
	ServerConfig& Config();
	void Stop();
};

