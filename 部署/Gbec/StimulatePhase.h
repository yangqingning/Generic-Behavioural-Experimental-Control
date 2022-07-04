#pragma once
#include "IInterruptable.h"
#include "IRunnable.h"
#include "SerialInterrupt.h"
#include <TimersOneForAll.h>
using namespace TimersOneForAll;
template <uint8_t TimerCode, uint16_t Milliseconds, UIDs UID>
class StimulatePhase : public IInterruptable
{
public:
	const IRunnable *const Stimulator;
	StimulatePhase(const IRunnable *Stimulator) : Stimulator(Stimulator) {}
	bool TryRun() const override
	{
		Stimulator->Run();
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
		WriteComponent(Stimulator);
		WriteProperty(Milliseconds);
	}
};