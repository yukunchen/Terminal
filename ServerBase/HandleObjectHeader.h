#pragma once
class HandleObjectHeader
{
public:
	HandleObjectHeader();
	~HandleObjectHeader();

	ULONG OpenCount;
	ULONG ReaderCount;
	ULONG WriterCount;
	ULONG ReadShareCount;
	ULONG WriteShareCount;
};

