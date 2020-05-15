#include "DNS.h"

bool Transcription::Interpret(const char* rawMsg, std::vector<std::string>* out, DNSHEADER* hout)
{
	memcpy(hout, rawMsg, 12);
	rawMsg += 12;

	const char* names = rawMsg;
	hout->questions = ntohs(hout->questions);
	for (int i = 0; i < hout->questions; ++i)
	{
		std::string name;
		names = ParseName(names, name);
		if (names == nullptr)
			return false;
		out->push_back(name);
	}

	return true;
}

const char* Transcription::ParseName(const char* pointer, std::string& out)
{
	while (*pointer != '\0')
	{
		int len = pointer[0];
		pointer++;
		for (int i = 0; i < len; ++i)
		{
			if (*pointer == '\0')
				break;
			
			out.push_back(*(pointer++));
		}
		out.push_back('.');
	}
	out.pop_back();
	if (out.empty())
		return nullptr;
	return pointer+1;
}

bool Transcription::ParseTo(char* buffer, std::unordered_map<std::string, DMAP> dmap, USHORT id, int *length)
{
	std::vector<int> nameOffsets;
	DNSHEADER header;
	memset(&header, 0, sizeof(DNSHEADER));

	auto begin = buffer + 12;
	for (auto name : dmap) {
		header.answers += name.second.count;
		nameOffsets.push_back(begin - buffer);
		begin = WriteName(begin, name.first);
		begin[1] = 1;
		begin[3] = 1;
		begin += 4;
	}

	int k = 0;
	for (auto name : dmap) {
		for (int i = 0; i < name.second.count; i++)
		{
			USHORT head = htons(0b1100000000000000 | nameOffsets[k]);
			*(USHORT*)begin = head;
			begin[3] = 1;
			begin[5] = 1;
			*(ULONG*)(begin + 6) = htonl(10);
			begin[11] = 4;
			*(ULONG*)(begin + 12) = name.second.arr[i];
			begin += 16;
		}
		k++;
	}

	header.questions = htons(nameOffsets.size());
	header.answers = htons(header.answers);
	header.qr = 1;
	header.id = id;
	memcpy(buffer, &header, sizeof(DNSHEADER));
	*length = begin - buffer;
	return true;
}

char* Transcription::WriteName(char* buffer, const std::string& name)
{
	std::string tb = " " + name;
	int bc = 0;
	const int len = tb.size();
	for (int i = 0; i < len; ++i) {
		if (tb[i] == '.') {
			tb[bc] = i - bc-1;
			bc = i;
		}
	}
	tb[bc] = len - bc-1;
	memcpy(buffer, tb.c_str(), len);
	buffer[len] = 0;
	return buffer+len+1;
}

