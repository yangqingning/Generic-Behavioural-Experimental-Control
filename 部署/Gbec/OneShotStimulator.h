#pragma once
#include "Stimulator.h"
#include <TimersOneForAll.h>
using namespace TimersOneForAll;
template <uint8_t PinCode, uint8_t TimerCode, uint16_t Milliseconds, UIDs UID, bool DownEvent = true>
class OneShotStimulator : public Stimulator<UID, PinCode, DownEvent>
{
private:
	static void DownAndCallback()
	{
		DigitalWrite<PinCode, LOW>();
		OnStimulatorDown();
	}

public:
	OneShotStimulator(const IRunnable *Notifier = nullptr) : Stimulator<UID, PinCode, DownEvent>(Notifier) {}
	void Test(uint16_t Times) const override
	{
		DigitalWrite<PinCode, HIGH>();
		DoAfter<TimerCode, DigitalWrite<PinCode, LOW>>(Milliseconds * Times);
	}
	void Action() const override
	{
		DigitalWrite<PinCode, HIGH>();
		DoAfter<TimerCode, Milliseconds, DownAndCallback>();
	}
	void WriteInformation() const override
	{
		Stimulator<UID, PinCode, DownEvent>::WriteInformation();
		WriteProperty(Milliseconds);
	}
};