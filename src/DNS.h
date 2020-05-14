#pragma once
#include <WinSock2.h>
#include <string>

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
	ULONG address;
} DNSA;

#pragma pack(pop)

namespace Transcription {
	bool Interpret(const char* rawMsg, char** out);
	//번역 내부 함수 (인라인)
}