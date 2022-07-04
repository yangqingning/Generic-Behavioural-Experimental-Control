#pragma once
#include "Stimulator.h"
#include <TimersOneForAll.h>
using namespace TimersOneForAll;
template <uint8_t PinCode, uint8_t TimerCode, uint16_t FrequencyHz, uint16_t Milliseconds, UIDs UID, bool DownEvent = true>
class ToneStimulator : public Stimulator<UID, PinCode, DownEvent>
{
public:
	ToneStimulator(const IRunnable *Notifier = nullptr) : Stimulator<UID, PinCode, DownEvent>(Notifier) {}
	void Test(uint16_t Times) const override
	{
		PlayTone<TimerCode, PinCode, FrequencyHz>(Milliseconds * Times);
	}
	void Action() const override
	{
		PlayTone<TimerCode, PinCode, FrequencyHz, Milliseconds, Stimulator<UID, PinCode,DownEvent>::OnStimulatorDown>();
	}
	void WriteInformation() const override
	{
		Stimulator<UID, PinCode, DownEvent>::WriteInformation();
		WriteProperty(FrequencyHz);
		WriteProperty(Milliseconds);
	}
};