#pragma once
#include <WinSock2.h>
#include <string>
#include <vector>
#include <unordered_map>

#pragma pack(push, 1)

typedef struct {
	USHORT id;
	USHORT qr		: 1;
	USHORT opcode	: 4;
	USHORT flags	: 7;
	USHORT rcode	: 4;
	USHORT questions;
	USHORT answers;
	USHORT tarr, tarr2; //Not used
} DNSHEADER;

typedef struct {
	USHORT name;
	USHORT type;
	USHORT cls;
	ULONG ttl;
	USHORT length;
	ULONG address;
};

typedef struct {
	ULONG address;
} DNSA;

#pragma pack(pop)

namespace Transcription {
	typedef struct {
		int count;
		ULONG* arr;
	} DMAP;
	bool Interpret(const char* rawMsg, std::vector<std::string>* out, DNSHEADER* hout);
	//번역 내부 함수 (인라인)
	inline const char* ParseName(const char* pointer, std::string& out);
	bool ParseTo(char* buffer, std::unordered_map<std::string, DMAP> dmap, USHORT id, int* length);
	inline char* WriteName(char* buffer, const std::string& name);
}