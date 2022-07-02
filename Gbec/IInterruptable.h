#pragma once
#include "IInformative.h"
class IInterruptable : public IInformative
{
public:
	//返回true表示应当中断，返回false表示应当继续
	virtual bool TryRun() const = 0;
	static bool TryRunStatic(const IInterruptable *II)
	{
		if (II)
			II->TryRun();
	}
};