#pragma once
#include "IInterruptable.h"
#include "SerialInterrupt.h"
#include "MonitorBase.h"
#include "IRunnable.h"
template <uint16_t Milliseconds, bool ResponseOnTimeUp, UIDs UID>
class ResponsePhase : public IInterruptable
{
public:
	const MonitorBase *const Monitor;
	const IRunnable *const Responsor;
	ResponsePhase(const MonitorBase *Monitor, const IRunnable *Responsor) : Monitor(Monitor), Responsor(Responsor) {}
	bool TryRun() const override
	{
		switch (Monitor->StartMonitor(Milliseconds, CheckInterrupt))
		{
		case Detected:
			SerialWrite(Signal_Detected);
			SerialWrite(Monitor->GetUID());
			Responsor->Run();
			return false;
		case TimeUp:
			SerialWrite(Signal_TimeUp);
			SerialWrite(Monitor->GetUID());
			if (ResponseOnTimeUp)
				Responsor->Run();
			return false;
		default:
			return true;
		}
	}
	void WriteInformation() const override
	{
		WriteProperty(UID);
		WriteComponent(Monitor);
		WriteComponent(Responsor);
		WriteProperty(Milliseconds);
		WriteProperty(ResponseOnTimeUp);
	}
};