#pragma once
#include <TimersOneForAll.h>
#include "MonitorBase.h"
using namespace TimersOneForAll;
template <uint8_t SwitchPin, uint8_t MonitorPin, uint8_t TimerCode, UIDs UID>
class Monitor : public MonitorBase
{
	void Reset()
	{
		DigitalWrite<SwitchPin, LOW>();
		DoAfter<TimerCode, 1, DigitalWrite<SwitchPin, HIGH>>();
	}

public:
	Monitor(const IRunnable *Notifier = nullptr) : MonitorBase(Notifier)
	{
		pinMode(SwitchPin, OUTPUT);
		pinMode(MonitorPin, INPUT);
		Reset();
	}
	//可选返回MillisecondsElapsed，返回实际经过的毫秒数。但如果监视超时，则不写该值。
	MonitorResult StartMonitor(uint16_t Milliseconds, InterruptResult (*CancelCheck)(), uint32_t *ElapsedMilliseconds = nullptr) const override
	{
		//这里不能用DoAfter，因为只有StartTimg能根据暂停动态延长监视时间
		StartTiming<TimerCode>();
		uint32_t TimeBefore;
		while (MillisecondsElapsed<TimerCode> < Milliseconds)
		{
			if (digitalRead(MonitorPin))
			{
				IRunnable::RunStatic(Notifier);
				//监视器不直接写串口，以免冷静阶段输出过多信息
				ShutDown<TimerCode>();
				if (ElapsedMilliseconds)
					*ElapsedMilliseconds = MillisecondsElapsed<TimerCode>;
				return Detected;
			}
			TimeBefore = MillisecondsElapsed<TimerCode>;
			switch (CancelCheck())
			{
			case Abort:
				ShutDown<TimerCode>();
				if (ElapsedMilliseconds)
					*ElapsedMilliseconds = MillisecondsElapsed<TimerCode>;
				return Cancelled;
			case Paused:
				MillisecondsElapsed<TimerCode> = TimeBefore;
			}
		}
		//监视器不直接写串口，以免冷静阶段输出过多信息
		ShutDown<TimerCode>();
		return TimeUp;
	}
	void Test(uint16_t Times) const override
	{
		if (Times > 0)
		{
			while (Times--)
			{
				if (digitalRead(MonitorPin))
				{
					SerialWrite(Signal_Detected);
					Reset();
				}
				if (Serial.available())
				{
					SerialRead<uint8_t>();
					break;
				}
			}
		}
		else
		{
			while (!Serial.available())
				if (digitalRead(MonitorPin))
				{
					SerialWrite(Signal_Detected);
					Reset();
				}
			SerialRead<uint8_t>();
		}
		SerialWrite(Signal_TestFinished);
		//检测到字节以后必须读出，否则会导致接下来的指令错位
	}
	void WriteInformation() const override
	{
		WriteProperty(UID);
	}
	UIDs GetUID() const override
	{
		return UID;
	}
};