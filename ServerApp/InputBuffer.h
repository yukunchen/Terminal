#pragma once
#include "..\ServerBaseApi\IConsoleObjects.h"
class InputBuffer : public IConsoleInputObject
{
public:
	InputBuffer();
	~InputBuffer();

	long WriteInputEvent(const INPUT_RECORD* const pInputRecord, _In_ long cInputRecords);

private:

    INPUT_RECORD* _pInputRecords;
    long _cInputRecords;
    long _InputMode;
    INPUT_RECORD* _pFirst; // ptr to base of circular buffer
    INPUT_RECORD* _pIn; // ptr to next free event
    INPUT_RECORD* _pOut; // ptr to next available event
    INPUT_RECORD* _pLast; // ptr to end + 1 of buffer
};

