#pragma once
#include "IStepTrial.h"
#include <vector>
#include <random>
#pragma pack(push,1)
struct RestoreInfo {
  UID TrialUID;
  uint16_t NumDone;
};
#pragma pack(pop)
struct ISession {
  UID MyUID;
  static std::mt19937 Urng;
  virtual void Start() const = 0;
  void Pause() const {
    TrialQueue[TrialsDone - 1]->Pause();
  }
  void Continue() const {
    TrialQueue[TrialsDone - 1]->Continue();
  }
  void Abort() const {
    TrialQueue[TrialsDone - 1]->Abort();
  }
  virtual void WriteInfo() const = 0;
  virtual void Restore(uint8_t NumDistinctTrials, const RestoreInfo *RI) const = 0;
  static void Setup(void (*FC)(), uint32_t RandomSeed) {
    FinishCallback = FC;
    Urng = std::mt19937(RandomSeed);
  }

protected:
  static std::vector<const ITrial *> TrialQueue;
  static uint16_t TrialsDone;
  static uint16_t TrialsRestored;
  static void (*FinishCallback)();
  static void RunAsync();
  constexpr ISession(UID MyUID)
    : MyUID(MyUID) {}
};