#include "DNSDatabase.h"
#include "Logger.h"

extern std::unique_ptr<Logger> logger;

using namespace std;
void DNSDatabase::AddRecord(Head& dest, DNSA rec)
{
	if (dest.begin == nullptr) {
		dest.begin = new Node{ rec, nullptr };
		dest.cnt++;
		return;
	}

	Node* newNode = new Node;

	newNode->a = rec;
	newNode->next = dest.begin->next;
	dest.begin->next = newNode;
	dest.cnt++;
}

void DNSDatabase::DeleteRecord(Head& dest, ULONG address)
{
	if (dest.begin == nullptr)
		return;

	Node* node = dest.begin;
	while (node->next != nullptr) {
		auto d = node->next;
		if (d->a.address == address) {
			node->next = d->next;
			delete d;
			dest.cnt--;
		}
		node = node->next;
	}

	if (dest.begin->a.address == address)
	{
		node = dest.begin;
		dest.begin = node->next;
		delete node;
		dest.cnt--;
	}
}

void DNSDatabase::DeleteAll(Head& begin)
{
	Node* node = begin.begin;
	while (node != nullptr) {
		auto t = node->next;
		delete node;
		node = t;
	}
	begin.begin = nullptr;
	begin.cnt = 0;
}

bool DNSDatabase::AuthQuery(ULONG host) const
{
	shared_lock<shared_mutex> lk{ authTableLock };
	return authBlacklist.find(host) == authBlacklist.end();
}

bool DNSDatabase::AuthInsert(ULONG host)
{
	unique_lock<shared_mutex> lk(dnsTableLock, std::adopt_lock);
	authBlacklist.insert(host);
	return true;
}

bool DNSDatabase::AuthRemove(ULONG host)
{
	unique_lock<shared_mutex> lk(dnsTableLock, std::adopt_lock);
	authBlacklist.erase(host);
	return true;
}

bool DNSDatabase::DNSQuery(const std::string& domain, ULONG** out, int* size) const
{
	shared_lock<shared_mutex> lk{ dnsTableLock };
	const auto result = dnsTable.find(domain);
	if (result == dnsTable.end())
		return false;
	
	ULONG* lst = nullptr;
	try {
		int count = result->second.cnt;
		auto ptr = result->second.begin;
		lst = new ULONG[count];
		for (int i = 0; i < count; i++) {
			lst[i] = ptr->a.address;
			ptr = ptr->next;
		}

		*out = lst;
		*size = count;
		return true;
	}
	catch (std::bad_alloc e) {
		logger->Log(e.what());
		return false;
	}
	catch (exception e) {
		if (lst != nullptr)
			delete[] lst;
		logger->Log(e.what());
		return false;
	}
}

bool DNSDatabase::DNSUpdate(const string& domain, const ULONG* addresses, int size)
{
	unique_lock<shared_mutex> lk(dnsTableLock, std::adopt_lock);
	auto it = dnsTable.find(domain);
	if (it == dnsTable.end())
	{
		it = dnsTable.insert(make_pair(domain, Head())).first;
	}
	Head& head = it->second;
	for (int i = 0; i < size; ++i) {
		AddRecord(head, DNSA{addresses[i]});
	}
	lk.unlock();
	return true;
}

bool DNSDatabase::DNSRemove(const std::string& domain, const ULONG* addresses, int size)
{
	unique_lock<shared_mutex> lk(dnsTableLock, std::adopt_lock);
	auto it = dnsTable.find(domain);
	if (it == dnsTable.end())
		return false;
	
	Head& head = it->second;
	for (int i = 0; i < size; ++i)
		DeleteRecord(head, addresses[i]);
	if (head.cnt == 0)
		dnsTable.erase(it);
	lk.unlock();
	return true;
}

//bool DNSDatabase::DNSUpdateBatch(FILE* file)
//{
//	int batchCounter = 0;
//	unique_lock<shared_mutex> lk(dnsTableLock, std::defer_lock);
//	
//	while (size > 0)
//	{
//		lk.lock();
//		for (; batchCounter < batchSize && size > 0; ++batchCounter) {
//			auto it = dnsTable.find(domain);
//			if (it == dnsTable.end()) {
//				dnsTable.insert()
//			}
//			--size;
//		}
//		batchCounter = 0;
//		lk.unlock();
//	}
//
//
//	return true;
//}
