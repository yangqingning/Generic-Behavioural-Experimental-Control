#pragma once
#include "IRunnable.h"
#include "ITestable.h"
#include <TimersOneForAll.h>
using namespace TimersOneForAll;
template <uint8_t PinCode, uint8_t TimerCode, uint16_t Milliseconds, UIDs UID>
class Tagger : public IRunnable, public ITestable
{
public:
	Tagger() : IRunnable(), ITestable()
	{
		pinMode(PinCode, OUTPUT);
	}
	void Test(uint16_t Times) const override
	{
		DigitalWrite<PinCode, HIGH>();
		DoAfter<TimerCode, DigitalWrite<PinCode, LOW>>(Milliseconds * Times);
	}
	void Run() const override
	{
		DigitalWrite<PinCode, HIGH>();
		DoAfter<TimerCode, Milliseconds, DigitalWrite<PinCode, LOW>>();
	}
	void WriteInformation() const override
	{
		WriteProperty(UID);
		WriteProperty(Milliseconds);
	}
	UIDs GetUID() const override
	{
		return UID;
	}
};