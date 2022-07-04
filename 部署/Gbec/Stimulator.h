#pragma once
#include "IRunnable.h"
#include "ITestable.h"
#include <LowLevelQuickDigitalIO.h>
template <UIDs UID, uint8_t PinCode, bool DownEvent>
class Stimulator : public IRunnable, public ITestable
{
protected:
	//子类应当在刺激结束后调用该函数
	static void OnStimulatorDown()
	{
		if (DownEvent)
		{
			SerialWrite(Signal_StimulatorDown);
			SerialWrite(UID);
		}
	}
	virtual void Action() const = 0;

public:
	const IRunnable *const Notifier;
	Stimulator(const IRunnable *Notifier) : Notifier(Notifier)
	{
		pinMode(PinCode,OUTPUT);
	}
	void Run() const override
	{
		Action();
		RunStatic(Notifier);
		SerialWrite(Signal_StimulatorUp);
		SerialWrite(UID);
	}
	virtual void WriteInformation() const override
	{
		WriteProperty(UID);
		WriteComponent(Notifier);
	}
	UIDs GetUID() const override
	{
		return UID;
	}
};