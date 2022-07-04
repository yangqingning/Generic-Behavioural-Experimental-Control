#pragma once
#include "MonitorBase.h"
#include "IInterruptable.h"
#include "SerialInterrupt.h"
//冷静阶段。该阶段会从指定范围中随机抽取一个毫秒数，然后让监视器持续监视该毫秒数。一旦检测到信号，重新开始检测；否则时间到，本阶段结束。
template <uint16_t MinMilliseconds, uint16_t MaxMilliseconds, UIDs UID>
class CalmdownPhase : public IInterruptable
{
public:
	const MonitorBase *const Monitor;
	CalmdownPhase(const MonitorBase *Monitor) : Monitor(Monitor) {}
	bool TryRun() const override
	{
		uint16_t Milliseconds = random(MinMilliseconds, MaxMilliseconds + 1);
		while (true)
		{
			MonitorResult MR = Monitor->StartMonitor(Milliseconds, CheckInterrupt);
			switch (MR)
			{
			case Detected:
				continue;
			case TimeUp:
				return false;
			case Cancelled:
				return true;
			}
		}
	}
	void WriteInformation() const override
	{
		WriteProperty(UID);
		WriteComponent(Monitor);
		WriteProperty(MinMilliseconds);
		WriteProperty(MaxMilliseconds);
	}
};