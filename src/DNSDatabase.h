#pragma once
#include "DNS.h"
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

constexpr int batchSize = 20;

class DNSDatabase
{
	struct Node {
		DNSA a;
		Node* next=nullptr;
	};

	struct Head {
		Node* begin=nullptr;
		int cnt=0;
	};

	void AddRecord(Head& dest, DNSA rec);
	void DeleteRecord(Head& dest, ULONG address);
	void DeleteAll(Head& begin);

	mutable std::shared_mutex dnsTableLock, authTableLock;
	std::unordered_map<std::string, Head> dnsTable;
	std::unordered_set<ULONG> authBlacklist;
public:
	bool AuthQuery(ULONG host) const;
	bool AuthInsert(ULONG host);
	bool AuthRemove(ULONG host);

	bool DNSQuery(const std::string& domain, ULONG** out, int* size) const;
	bool DNSUpdate(const std::string& domain, const ULONG* addresses, int size);
	bool DNSRemove(const std::string& domain, const ULONG* addresses, int size);
	//bool DNSUpdateBatch(FILE* file);
};