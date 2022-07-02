#pragma once
#include "IInterruptablePublicUid.h"
//一个回合本质上是多个阶段的集合，依次运行。
template <uint8_t NoPhases, UIDs UID>
class Trial : public IInterruptablePublicUid
{
public:
	const IInterruptable **Phases;
	Trial(const IInterruptable *Phases[]) : Phases(Phases) {}
	bool TryRun() const override
	{
		SerialWrite(UID);
		for (uint8_t a = 0; a < NoPhases; ++a)
			if (Phases[a]->TryRun())
				return true;
		SerialWrite(Signal_TrialFinished);
		return false;
	}
	void WriteInformation() const override
	{
		WriteProperty(UID);
		SerialWrite(Property_Phases);
		SerialWrite(ArrayStart);
		SerialWrite(Type_uint8_t);
		SerialWrite(NoPhases);
		//用StructStart表示这是一个结构体数组，然后每个结构体的结束就用单一个StructEnd标志，不要使用重复的StructStart。
		SerialWrite(StructStart);
		for (uint8_t a = 0; a < NoPhases; ++a)
		{
			Phases[a]->WriteInformation();
			SerialWrite(StructEnd);
		}
	}
	UIDs GetUID() const override
	{
		return UID;
	}
};