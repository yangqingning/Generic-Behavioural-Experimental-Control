#pragma once
#include "ITestable.h"
#include "IRunnable.h"
#include "InterruptResult.h"
enum MonitorResult : uint8_t
{
	//返回该值，通知调用方本次监视期间检测到了信号
	Detected,
	//返回该值，表示监视期间未检测到信号，超时返回
	TimeUp,
	//返回该值，表示监视期间收到指令要求中断监视，传感器未收到信号
	Cancelled,
};
class MonitorBase : public IInformative, public ITestable
{
public:
	const IRunnable *const Notifier;
	MonitorBase(const IRunnable *Notifier) : Notifier(Notifier) {}
	//virtual void Reset() const = 0;
	virtual MonitorResult StartMonitor(uint16_t Milliseconds, InterruptResult (*CancelCheck)(), uint32_t *MillisecondsElapsed = nullptr) const = 0;
};	