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
	ULONG address[] = { inet_addr("172.30.1.16") };
	db->DNSUpdate("a.a", address, 1);
	db->DNSUpdate("b.a", address, 1);
	db->DNSUpdate("www.naver.com", address, 1);
	db->DNSUpdate("naver.com", address, 1);
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
