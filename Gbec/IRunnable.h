#pragma once
#include "IInformative.h"
class IRunnable:public IInformative
{
public:
	virtual void Run() const = 0;
	static void RunStatic(const IRunnable *IR)
	{
		if (IR)
			IR->Run();
	}
};