#include "DNS.h"
#include "DNSDatabase.h"
#include "MessageBuffer.h"
#include "UDPListener.h"
#include <stdio.h>
#include "Logger.h"

std::unique_ptr<DNSDatabase> db(new DNSDatabase);
std::unique_ptr<Logger> logger(new Logger("testlog.txt"));
UDPSocket udpSender;

int main() {
	ULONG address[] = { htonl(0xAC1E0110), htonl(0xAC1E0104) };
	db->DNSUpdate("a.a", address, 2);
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
	ServerConfig config;
	config.address = INADDR_ANY;
	config.port = 53;
	config.minRecvTerm = 100;
	try {
		auto msgBuffer = std::make_unique<MessageBuffer>();
		auto listner = std::make_unique<UDPListener>(config, msgBuffer.get());
		while (getchar() != ' ');
	}
	catch (std::exception e)
	{
		logger->Log(e.what());
	}
	WSACleanup();
	return 0;
}