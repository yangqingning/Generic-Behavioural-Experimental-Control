#pragma once
#include "IStepTrial.h"
#include <vector>
struct RestoreInfo
{
	UID TrialUID;
	uint16_t NumDone;
};
struct ISession
{
	UID MyUID;
	virtual void Start() const = 0;
	void Pause() const { TrialQueue[TrialsDone - 1]->Pause(); }
	void Continue() const { TrialQueue[TrialsDone - 1]->Continue(); }
	void Abort() const { TrialQueue[TrialsDone - 1]->Abort(); }
	virtual uint8_t GetInfo(const void *&Info) const = 0;
	virtual void Restore(uint8_t NumDistinctTrials, const RestoreInfo *RI) const = 0;
	static void Setup(void (*FC)())
	{
		FinishCallback = FC;
	}

  protected:
	static std::vector<const ITrial *> TrialQueue;
	static uint16_t TrialsDone;
	static uint16_t TrialsRestored;
	static void (*FinishCallback)();
	static void RunAsync();
	constexpr ISession(UID MyUID) : MyUID(MyUID) {}
};