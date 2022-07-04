#pragma once
#include "IInterruptable.h"
#include "SerialInterrupt.h"
#include <TimersOneForAll.h>
using namespace TimersOneForAll;
template <uint8_t TimerCode, uint16_t MinMilliseconds, uint16_t MaxMilliseconds,UIDs UID>
class AwaitPhase : public IInterruptable
{
public:
	bool TryRun() const override
	{
		uint16_t Milliseconds = random(MinMilliseconds, MaxMilliseconds + 1);
		StartTiming<TimerCode>();
		uint16_t TimeBefore;
		while (MillisecondsElapsed<TimerCode> < Milliseconds)
		{
			TimeBefore = MillisecondsElapsed<TimerCode>;
			switch (CheckInterrupt())
			{
			case Abort:
				return true;
			case Paused:
				MillisecondsElapsed<TimerCode> = TimeBefore;
			}
		}
		return false;
	}
	void WriteInformation() const override
	{
		WriteProperty(UID);
		WriteProperty(MinMilliseconds);
		WriteProperty(MaxMilliseconds);
	}
};