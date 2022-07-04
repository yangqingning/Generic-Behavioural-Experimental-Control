#pragma once
#include "IPublicUid.h"
class ITestable : public IPublicUid
{
public:
	virtual void Test(uint16_t) const = 0;
	static void TestStatic(const ITestable *IT, uint16_t Times)
	{
		if (IT)
			IT->Test(Times);
	}
};