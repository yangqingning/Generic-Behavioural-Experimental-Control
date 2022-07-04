#pragma once
#include "ISession.h"
template <uint8_t NoDistinctTrials, bool SequenceFixed, UIDs UID>
class Session : public ISession
{
public:
	const IInterruptablePublicUid *const *const DistinctTrials;
	const uint16_t *const NoEachTrials;
	Session(const IInterruptablePublicUid *DistinctTrials[], const uint16_t NoEachTrials[]) : DistinctTrials(DistinctTrials), NoEachTrials(NoEachTrials) {}
	bool Continue(uint8_t NoDistinctUIDs, const UIDs *FinishedUID, const uint16_t *FinishedNumbers) const override
	{
		//不用反馈UID，因为UID是由PC端直接发来的指令，理应知道将要运行什么UID
		uint16_t NoTrialsLeft[NoDistinctTrials];
		memcpy(NoTrialsLeft, NoEachTrials, NoDistinctTrials * sizeof(uint16_t));
		uint16_t TrialToRun = 0;
		for (uint8_t a = 0; a < NoDistinctUIDs; ++a)
			for (uint8_t b = 0; b < NoDistinctTrials; ++b)
				if (DistinctTrials[b]->GetUID() == FinishedUID[a])
				{
					TrialToRun += FinishedNumbers[a];
					NoTrialsLeft[b] -= FinishedNumbers[a];
					break;
				}
		if (SequenceFixed)
		{
			for (uint8_t a = 0; a < NoDistinctTrials; ++a)
				for (uint8_t b = 0; b < NoTrialsLeft[a]; ++b)
				{
					SerialWrite(Signal_TrialStarted);
					SerialWrite(++TrialToRun);
					if (DistinctTrials[a]->TryRun())
					{
						SerialWrite(Signal_SessionAborted);
						return true;
					}
				}
		}
		else
		{
			uint8_t TrialIndex;
			uint16_t SumTrialsLeft = 0;
			for (TrialIndex = 0; TrialIndex < NoDistinctTrials; ++TrialIndex)
				SumTrialsLeft += NoTrialsLeft[TrialIndex];
			uint16_t Selection;
			for (; SumTrialsLeft > 0; SumTrialsLeft--)
			{
				Selection = random(SumTrialsLeft);
				for (TrialIndex = 0; TrialIndex < NoDistinctTrials; ++TrialIndex)
					if (Selection < NoTrialsLeft[TrialIndex])
						break;
					else
						Selection -= NoTrialsLeft[TrialIndex];
				SerialWrite(Signal_TrialStarted);
				SerialWrite(++TrialToRun);
				if (DistinctTrials[TrialIndex]->TryRun())
				{
					SerialWrite(Signal_SessionAborted);
					return true;
				}
				else
					NoTrialsLeft[TrialIndex]--;
			}
		}
		SerialWrite(Signal_SessionFinished);
		return false;
	}
	bool TryRun() const override
	{
		return Continue(0, nullptr, nullptr);
	}
	void WriteInformation() const override
	{
		WriteProperty(UID);
		WriteProperty(SequenceFixed);
		SerialWrite(Property_NoEachTrials);
		SerialWrite(ArrayStart);
		SerialWrite(Type_uint8_t);
		SerialWrite(NoDistinctTrials);
		SerialWrite(Type_uint16_t);
		Serial.write((uint8_t *)NoEachTrials, NoDistinctTrials * sizeof(uint16_t));
		SerialWrite(Property_DistinctTrials);
		SerialWrite(ArrayStart);
		SerialWrite(Type_uint8_t);
		SerialWrite(NoDistinctTrials);
		//结构体数组只需要一个StructStart
		SerialWrite(StructStart);
		for (uint8_t a = 0; a < NoDistinctTrials; ++a)
		{
			DistinctTrials[a]->WriteInformation();
			SerialWrite(StructEnd);
		}
	}
	UIDs GetUID() const override
	{
		return UID;
	}
};