#pragma once
#include "UID.h"
// 具体Step的设计应当具有概念性，优先考虑其输出信息易于理解
struct IStep {
  virtual void Setup() const {}
  // 返回值指示该步骤是否有持续时间，必须等待FinishCallback。如果返回false，表示步骤已立即完成，可以直接开始下一步。非持续性等待步骤通常无需重写Pause和Continue
  virtual bool Start(void (*FinishCallback)()) const {};
  virtual void Pause() const {}
  virtual void Continue() const {}
  // 默认和暂停相同，如果没有特殊的Abort实现就不需要重写
  virtual void Abort() const {
    Pause();
  }
  static constexpr UID Info = Step_Null;
};
struct ITrial : public IStep {
  UID MyUID;
  virtual void WriteInfo()const=0;

protected:
  static uint8_t StepsDone;
  constexpr ITrial(UID MyUID)
    : MyUID(MyUID) {}
  static void (*FinishCallback)();
};