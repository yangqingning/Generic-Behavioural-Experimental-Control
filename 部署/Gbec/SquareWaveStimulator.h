#pragma once
#include "Stimulator.h"
#include <TimersOneForAll.h>
using namespace TimersOneForAll;
template <uint8_t PinCode, uint8_t TimerCode, uint16_t HighMilliseconds, uint16_t LowMilliseconds, uint16_t RepeatTimes, UIDs UID,bool DownEvent=true>
class SquareWaveStimulator : public Stimulator<UID, PinCode,DownEvent>
{
public:
	SquareWaveStimulator(const IRunnable *Notifier = nullptr) : Stimulator<UID, PinCode,DownEvent>(Notifier) {}
	void Test(uint16_t Times) const override
	{
		SquareWave<TimerCode, PinCode, HighMilliseconds, LowMilliseconds>(RepeatTimes * Times);
	}
	void Action() const override
	{
		SquareWave<TimerCode, PinCode, HighMilliseconds, LowMilliseconds, RepeatTimes, Stimulator<UID, PinCode,DownEvent>::OnStimulatorDown>();
	}
	void WriteInformation() const override
	{
		Stimulator<UID, PinCode,DownEvent>::WriteInformation();
		WriteProperty(HighMilliseconds);
		WriteProperty(LowMilliseconds);
		WriteProperty(RepeatTimes);
	}
};