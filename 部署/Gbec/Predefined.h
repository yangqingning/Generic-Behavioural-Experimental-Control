#pragma once
#include "ISession.h"
#include "SerialIO.h"
#include <algorithm>
#include <map>
#include <numeric>
#include <set>
#include <type_traits>
#include <TimersOneForAll.h>
template<uint8_t Pin>
bool NeedSetup = true;
template<typename T>
bool _NeedSetup = true;
constexpr bool PinInterruptable(uint8_t Pin) {
  constexpr uint8_t Interruptables[] = { 2, 3, 18, 19, 20, 21 };
  bool Found = false;
  for (uint8_t P : Interruptables)
    if (P == Pin) {
      Found = true;
      break;
    }
  return Found;
}
template<uint8_t Pin>
std::set<void (*)()> CallbackList;
template<uint8_t Pin>
void TraverseCallback() {
  for (void (*const C)() : CallbackList<Pin>)
    C();
}
template<uint8_t Pin>
constexpr uint8_t Interrupt = digitalPinToInterrupt(Pin);
// 如果试图添加重复的中断回调，则不会添加，也不会出错
template<uint8_t Pin>
void RisingInterrupt(void (*Callback)()) {
  static_assert(PinInterruptable(Pin));
  if (CallbackList<Pin>.empty())
    attachInterrupt(Interrupt<Pin>, TraverseCallback<Pin>, RISING);
  CallbackList<Pin>.insert(Callback);
}
template<uint8_t Pin>
void DetachInterrupt(void (*Callback)()) {
  CallbackList<Pin>.erase(Callback);
  if (CallbackList<Pin>.empty())
    detachInterrupt(Interrupt<Pin>);
}
struct ITest {
  UID MyUID;
  // 测试开始时将调用此方法。测试分为自动结束型和手动结束型。对于自动结束型，应当根据TestTimes参数将测试重复指定的次数，并返回true表示该测试将自动结束。对于手动结束型，一般应当忽略TestTimes参数，返回false，持续测试直到Stop被调用。
  virtual bool Start(uint16_t TestTimes) const = 0;
  //测试被用户手动结束时将调用此方法。自动结束型测试无需实现此方法，手动结束型则必须实现。
  virtual void Stop() const {}
  constexpr ITest(UID MyUID)
    : MyUID(MyUID) {}
};
// 给引脚一段时间的高或低电平，然后反转
template<UID TMyUID, uint8_t Pin, uint8_t TimerCode, uint16_t Milliseconds, bool HighOrLow = HIGH>
struct PinFlashTest : public ITest {
  constexpr PinFlashTest()
    : ITest(TMyUID) {}
  bool Start(uint16_t TestTimes) const override {
    if (NeedSetup<Pin>) {
      pinMode(Pin, OUTPUT);
      NeedSetup<Pin> = false;
    }
    DigitalWrite<Pin, HighOrLow>();
    TimersOneForAll::DoAfter<TimerCode>(Milliseconds * TestTimes, DigitalWrite<Pin, !HighOrLow>);
    return true;
  }
};
// 监视引脚，每次高电平发送串口报告。此测试需要调用Stop才能终止，且无视TestTimes参数
template<UID TMyUID, uint8_t ReadPin>
class MonitorTest : public ITest {
  static void ReportHit() {
    SerialWrite(Signal_MonitorHit);
  }

public:
  constexpr MonitorTest()
    : ITest(TMyUID) {}
  bool Start(uint16_t) const override {
    RisingInterrupt<ReadPin>(ReportHit);
    return false;
  }
  void Stop() const override {
    DetachInterrupt<ReadPin>(ReportHit);
  }
};
template<typename T>
constexpr T Instance;
template<typename... Ts>
const std::map<UID, const ITest *> TestMap_t{ { Instance<Ts>.MyUID, &Instance<Ts> }... };
template<typename ToAdd, typename Container>
struct _AddToArray;
template<typename ToAdd, template<typename...> typename Container, typename... AlreadyIn>
struct _AddToArray<ToAdd, Container<AlreadyIn...>> {
  using Result = Container<ToAdd, AlreadyIn...>;
};
template<typename ToAdd, typename Container>
using AddToArray = typename _AddToArray<ToAdd, Container>::Result;
template<typename T>
constexpr UID TypeToUID = T::MyUID;
template<>
constexpr UID TypeToUID<bool> = Type_Bool;
template<>
constexpr UID TypeToUID<uint8_t> = Type_UInt8;
template<>
constexpr UID TypeToUID<uint16_t> = Type_UInt16;
template<>
constexpr UID TypeToUID<UID> = Type_UID;
#pragma pack(push, 1)
template<typename T, typename... Ts>
struct InfoArray {
  constexpr static UID MyUID = Type_Array;
  uint8_t Number = sizeof...(Ts) + 1;
  UID Type = TypeToUID<typename T::value_type>;
  typename T::value_type Array[sizeof...(Ts) + 1] = { T::value, Ts::value... };
};
template<typename T, typename... Ts>
struct CellArray {
  UID Type = TypeToUID<T>;
  T Value;
  CellArray<Ts...> Values;
  constexpr CellArray(T Value, Ts... Values)
    : Value(Value), Values(CellArray<Ts...>(Values...)) {}
};
template<typename T>
struct CellArray<T> {
  UID Type = TypeToUID<T>;
  T Value;
  constexpr CellArray(T Value)
    : Value(Value) {}
};
template<typename... T>
struct InfoCell {
  constexpr static UID MyUID = Type_Cell;
  uint8_t NumCells = 0;
};
template<typename T, typename... Ts>
struct InfoCell<T, Ts...> {
  constexpr static UID MyUID = Type_Cell;
  uint8_t NumCells = sizeof...(Ts) + 1;
  CellArray<T, Ts...> Values;
  constexpr InfoCell(T Value, Ts... Values)
    : Values(CellArray<T, Ts...>(Value, Values...)) {}
};
// 只有主模板能推断，特化模板必须加推断向导
template<typename T, typename... Ts>
InfoCell(T, Ts...) -> InfoCell<T, Ts...>;
template<typename TName, typename TValue, typename... Ts>
struct InfoFields {
  TName Name;
  UID Type = TypeToUID<TValue>;
  TValue Value;
  InfoFields<Ts...> FollowingFields;
  constexpr InfoFields(TName Name, TValue Value, Ts... NameValues)
    : Name(Name), Value(Value), FollowingFields(InfoFields<Ts...>(NameValues...)) {}
};
template<typename TName, typename TValue>
struct InfoFields<TName, TValue> {
  TName Name;
  UID Type = TypeToUID<TValue>;
  TValue Value;
  constexpr InfoFields(TName Name, TValue Value)
    : Name(Name), Value(Value) {}
};
template<typename... Ts>
struct InfoStruct {
  constexpr static UID MyUID = Type_Struct;
  uint8_t NumFields = 0;
};
template<typename T, typename... Ts>
struct InfoStruct<UID, T, Ts...> {
  constexpr static UID MyUID = Type_Struct;
  uint8_t NumFields = sizeof...(Ts) / 2 + 1;
  InfoFields<UID, T, Ts...> Fields;
  constexpr InfoStruct(UID Name, T Value, Ts... NameValues)
    : Fields(InfoFields<UID, T, Ts...>(Name, Value, NameValues...)) {}
};
// 只有主模板能推断，特化模板必须加推断向导
template<typename... Ts>
InfoStruct(UID, Ts...) -> InfoStruct<UID, Ts...>;
#pragma pack(pop)
using NullStep = IStep;
// 持续等待，直到指定引脚在指定连续毫秒数内都保持静默才结束
template<uint8_t Pin, uint8_t TimerCode, uint16_t MinMilliseconds, uint16_t MaxMilliseconds = MinMilliseconds, UID MyUID = Step_Calmdown>
class CalmdownStep : public IStep {
  static uint16_t Milliseconds;
  static void (*FinishCallback)();
  static constexpr bool RandomTime = MinMilliseconds < MaxMilliseconds;
  static void Reset() {
    if (RandomTime)
      TimersOneForAll::DoAfter<TimerCode>(Milliseconds, TimeUp);
    else
      TimersOneForAll::DoAfter<TimerCode, MinMilliseconds>(TimeUp);
  }
  static void TimeUp() {
    DetachInterrupt<Pin>(Reset);
    FinishCallback();
  }

public:
  bool Start(void (*FC)()) const override {
    FinishCallback = FC;
    if (RandomTime)
      TimersOneForAll::DoAfter<TimerCode>(Milliseconds = random(MinMilliseconds, MaxMilliseconds + 1), TimeUp);
    else
      TimersOneForAll::DoAfter<TimerCode, MinMilliseconds>(TimeUp);
    RisingInterrupt<Pin>(Reset);
    return true;
  }
  void Pause() const override {
    DetachInterrupt<Pin>(Reset);
    TimersOneForAll::Pause<TimerCode>();
  }
  void Continue() const override {
    TimersOneForAll::Continue<TimerCode>();
    RisingInterrupt<Pin>(Reset);
  }
  void Abort() const override {
    DetachInterrupt<Pin>(Reset);
    TimersOneForAll::ShutDown<TimerCode>();
  }
  void Setup() const override {
    if (NeedSetup<Pin>) {
      static_assert(PinInterruptable(Pin), "试图将不支持中断的引脚用于CalmdownStep");
      pinMode(Pin, INPUT);
      NeedSetup<Pin> = false;
    }
  }
  static constexpr auto Info = InfoStruct(Info_UID, MyUID, Info_Pin, Pin, Info_MinMilliseconds, MinMilliseconds, Info_MaxMilliseconds, MaxMilliseconds);
};
template<uint8_t Pin, uint8_t TimerCode, uint16_t MinMilliseconds, uint16_t MaxMilliseconds, UID MyUID>
uint16_t CalmdownStep<Pin, TimerCode, MinMilliseconds, MaxMilliseconds, MyUID>::Milliseconds;
template<uint8_t Pin, uint8_t TimerCode, uint16_t MinMilliseconds, uint16_t MaxMilliseconds, UID MyUID>
void (*CalmdownStep<Pin, TimerCode, MinMilliseconds, MaxMilliseconds, MyUID>::FinishCallback)();
template<typename Reporter>
inline void Report() {
  Instance<Reporter>.Start([]() {});
}
template<typename T>
constexpr bool IsNS = std::is_same_v<T, NullStep>;
// 让引脚高电平一段时间再回到低电平。异步执行，不阻塞时相
template<uint8_t Pin, uint8_t TimerCode, uint16_t Milliseconds, typename UpReporter = NullStep, typename DownReporter = NullStep, UID MyUID = Step_PinFlash>
class PinFlashStep : public IStep {
  static void DownReport() {
    DigitalWrite<Pin, LOW>();
    if (!IsNS<DownReporter>)
      Report<DownReporter>();
  }
public:
  bool Start(void (*FC)()) const override {
    if (!IsNS<UpReporter>)
      Report<UpReporter>();
    DigitalWrite<Pin, HIGH>();
    TimersOneForAll::DoAfter<TimerCode, Milliseconds>(DownReport);
    return false;
  }
  void Setup() const override {
    if (NeedSetup<Pin>) {
      pinMode(Pin, OUTPUT);
      NeedSetup<Pin> = false;
    }
  }
  static constexpr auto Info = InfoStruct(Info_UID, MyUID, Info_Pin, Pin, Info_Milliseconds, Milliseconds);
};
// 在一段时间内同步监视引脚，发现高电平立即汇报。HitReporter和MissReporter都是IStep类型，一律异步执行，不等待
template<uint8_t Pin, uint8_t TimerCode, uint16_t Milliseconds, uint8_t Flags, typename HitReporter, typename MissReporter = NullStep, UID MyUID = Step_Monitor>
class MonitorStep : public IStep {
  static void (*FinishCallback)();
  static bool NoHits;
  static constexpr bool ReportOnce = Flags & Monitor_ReportOnce;
  static constexpr bool ReportMiss = !IsNS<MissReporter>;
  static constexpr bool HitAndFinish = Flags & Monitor_HitAndFinish;
  static void HitReport() {
    Report<HitReporter>();
    if (HitAndFinish) {
      DetachInterrupt<Pin>(HitReport);
      TimersOneForAll::ShutDown<TimerCode>();
      FinishCallback();
    } else {
      if (ReportOnce)
        DetachInterrupt<Pin>(HitReport);
      if (ReportMiss)
        NoHits = false;
    }
  }
  static void TimeUp() {
    DetachInterrupt<Pin>(HitReport);
    if (ReportMiss && NoHits)
      Report<MissReporter>();
    FinishCallback();
  }

public:
  bool Start(void (*FC)()) const override {
    FinishCallback = FC;
    if (ReportMiss)
      NoHits = true;
    RisingInterrupt<Pin>(HitReport);
    TimersOneForAll::DoAfter<TimerCode, Milliseconds>(TimeUp);
    return true;
  }
  void Pause() const override {
    DetachInterrupt<Pin>(HitReport);
    TimersOneForAll::Pause<TimerCode>();
  }
  void Continue() const override {
    TimersOneForAll::Continue<TimerCode>();
    RisingInterrupt<Pin>(HitReport);
  }
  void Abort() const override {
    DetachInterrupt<Pin>(HitReport);
    TimersOneForAll::ShutDown<TimerCode>();
  }
  void Setup() const override {
    if (NeedSetup<Pin>) {
      static_assert(PinInterruptable(Pin), "试图将不支持中断的引脚用于MonitorStep");
      pinMode(Pin, INPUT);
      NeedSetup<Pin> = false;
    }
  }
  static constexpr auto Info = InfoStruct(
    Info_UID, MyUID,
    Info_Pin, Pin,
    Info_Milliseconds, Milliseconds,
    Info_ReportOnce, ReportOnce,
    Info_ReportMiss, ReportMiss,
    Info_HitAndFinish, HitAndFinish,
    Info_HitReporter, HitReporter::Info,
    Info_MissReporter, MissReporter::Info);
};
template<uint8_t Pin, uint8_t TimerCode, uint16_t Milliseconds, uint8_t Flags, typename HitReporter, typename MissReporter, UID MyUID>
void (*MonitorStep<Pin, TimerCode, Milliseconds, Flags, HitReporter, MissReporter, MyUID>::FinishCallback)();
template<uint8_t Pin, uint8_t TimerCode, uint16_t Milliseconds, uint8_t Flags, typename HitReporter, typename MissReporter, UID MyUID>
bool MonitorStep<Pin, TimerCode, Milliseconds, Flags, HitReporter, MissReporter, MyUID>::NoHits;
// 不做任何事，同步等待一段时间后再进入下一步。
template<uint8_t TimerCode, uint16_t MinMilliseconds, uint16_t MaxMilliseconds = MinMilliseconds, UID MyUID = Step_Wait>
struct WaitStep : public IStep {
  bool Start(void (*FinishCallback)()) const override {
    if (MinMilliseconds < MaxMilliseconds)
      TimersOneForAll::DoAfter<TimerCode>(random(MinMilliseconds, MaxMilliseconds + 1), FinishCallback);
    else
      TimersOneForAll::DoAfter<TimerCode, MinMilliseconds>(FinishCallback);
    return true;
  }
  void Pause() const override {
    TimersOneForAll::Pause<TimerCode>();
  }
  void Continue() const override {
    TimersOneForAll::Continue<TimerCode>();
  }
  void Abort() const override {
    TimersOneForAll::ShutDown<TimerCode>();
  }
  static constexpr auto Info = InfoStruct(Info_UID, MyUID, Info_MinMilliseconds, MinMilliseconds, Info_MaxMilliseconds, MaxMilliseconds);
};
// 开始监视指定引脚，发现高电平立即汇报。异步执行，步骤本身立即结束，但会在后台持续监视，直到StopMonitorStep才会结束监视。
template<uint8_t Pin, typename Reporter, UID MyUID = Step_StartMonitor>
struct StartMonitorStep : public IStep {
  bool Start(void (*)()) const override {
    RisingInterrupt<Pin>(Report<Reporter>);
    return false;
  }
  void Setup() const override {
    if (NeedSetup<Pin>) {
      static_assert(PinInterruptable(Pin), "试图将不支持中断的引脚用于StartMonitorStep");
      pinMode(Pin, INPUT);
      NeedSetup<Pin> = false;
    }
  }
  static constexpr auto Info = InfoStruct(Info_UID, MyUID, Info_Pin, Pin, Info_Reporter, Reporter::Info);
};
// 停止监视指定引脚。如果一个引脚被多个Reporter监视，此步骤只会终止指定Reporter的监视，其它Reporter照常工作。
template<uint8_t Pin, typename Reporter, UID MyUID = Step_StopMonitor>
struct StopMonitorStep : public IStep {
  bool Start(void (*)()) const override {
    DetachInterrupt<Pin>(Report<Reporter>);
    return false;
  }
  static constexpr auto Info = InfoStruct(Info_UID, MyUID, Info_Pin, Pin, Info_Reporter, Reporter::Info);
};
// 向串口写出一个字节
template<UID ToWrite, UID MyUID = Step_SerialWrite>
struct SerialWriteStep : public IStep {
  bool Start(void (*)()) const override {
    SerialWrite(ToWrite);
    return false;
  }
  static constexpr auto Info = InfoStruct(Info_UID, MyUID, Info_ToWrite, ToWrite);
};
// 播放具有指定频率㎐和指定毫秒数的声音。异步执行，步骤不阻塞时相。
template<uint8_t Pin, uint8_t TimerCode, uint16_t FrequencyHz, uint16_t Milliseconds, typename UpReporter = NullStep, typename DownReporter = NullStep, UID MyUID = Step_Audio>
struct ToneStep : public IStep {
  bool Start(void (*)()) const override {
    if (!IsNS<UpReporter>)
      Report<UpReporter>();
    TimersOneForAll::PlayTone<TimerCode, Pin, FrequencyHz, Milliseconds, IsNS<DownReporter> ? nullptr : Report<DownReporter>>();
  }
  void Setup() const override {
    if (NeedSetup<Pin>) {
      pinMode(Pin, OUTPUT);
      NeedSetup<Pin> = false;
    }
  }
  static constexpr auto Info = InfoStruct(Info_UID, MyUID, Info_Pin, Pin, Info_FrequencyHz, FrequencyHz, Info_Milliseconds, Milliseconds);
};
template<UID TUID, typename... TSteps>
class Trial : public ITrial {
  static bool &NeedSetup;
  static const IStep *Steps[sizeof...(TSteps)];
  static void NextStep() {
    while (StepsDone < sizeof...(TSteps))
      if (Steps[StepsDone++]->Start(NextStep))
        return;
    FinishCallback();
  }

public:
  static constexpr auto Info = InfoStruct(Info_UID, TUID, Info_Steps, InfoCell(TSteps::Info...));
  constexpr Trial()
    : ITrial(TUID) {}
  void Setup() const override {
    if (NeedSetup) {
      for (const IStep *S : Steps)
        S->Setup();
      NeedSetup = false;
    }
  }
  bool Start(void (*FC)()) const override {
    FinishCallback = FC;
    StepsDone = 0;
    while (StepsDone < sizeof...(TSteps))
      if (Steps[StepsDone++]->Start(NextStep))
        return true;
    return false;
  }
  void Pause() const override {
    Steps[StepsDone - 1]->Pause();
  }
  void Continue() const override {
    Steps[StepsDone - 1]->Continue();
  }
  void Abort() const override {
    Steps[StepsDone - 1]->Abort();
  }
};
template<UID TUID, typename... TSteps>
bool &Trial<TUID, TSteps...>::NeedSetup = _NeedSetup<Trial<TUID, TSteps...>>;
template<UID TUID, typename... TSteps>
const IStep *Trial<TUID, TSteps...>::Steps[sizeof...(TSteps)] = { &Instance<TSteps>... };
template<uint16_t Value>
using N = std::integral_constant<uint16_t, Value>;
template<typename... Trials>
struct TrialArray {
  static const ITrial *Interfaces[sizeof...(Trials)];
  static constexpr auto Info = InfoCell(Trials::Info...);
};
// 静态成员必须类外定义，即使模板也一样
template<typename... Trials>
const ITrial *TrialArray<Trials...>::Interfaces[sizeof...(Trials)] = { &Instance<Trials>... };
template<typename TTrial, typename TNumber, typename... TrialThenNumber>
struct TrialNumberSplit {
  using Numbers_t = AddToArray<TNumber, typename TrialNumberSplit<TrialThenNumber...>::Numbers_t>;
  using Trials_t = AddToArray<TTrial, typename TrialNumberSplit<TrialThenNumber...>::Trials_t>;
  constexpr static Numbers_t Numbers = Numbers_t();
};
template<typename TTrial, typename TNumber>
struct TrialNumberSplit<TTrial, TNumber> {
  using Numbers_t = InfoArray<TNumber>;
  using Trials_t = TrialArray<TTrial>;
  constexpr static Numbers_t Numbers = Numbers_t();  // 必须显式初始化不然不过编译
};
template<UID TUID, bool TRandom, typename... TrialThenNumber>
struct Session : public ISession {
  using TNS = TrialNumberSplit<TrialThenNumber...>;
  static bool &NeedSetup;
  constexpr static uint8_t NumDistinctTrials = std::extent_v<decltype(TNS::Numbers.Array)>;
  static void ArrangeTrials(const uint16_t *TrialsLeft) {
    TrialQueue.resize(std::accumulate(TrialsLeft, TrialsLeft + NumDistinctTrials, uint16_t(0)));
    const ITrial **TQPointer = TrialQueue.data();
    for (uint8_t T = 0; T < NumDistinctTrials; ++T) {
      if (NeedSetup)
        TNS::Trials_t::Interfaces[T]->Setup();
      std::fill_n(TQPointer, TrialsLeft[T], TNS::Trials_t::Interfaces[T]);
      TQPointer += TrialsLeft[T];
    }
    NeedSetup = false;
    if (TRandom)
      std::shuffle(TrialQueue.data(), TQPointer, Urng);
    TrialsDone = 0;
  }

public:
  constexpr Session()
    : ISession(TUID) {}
  void WriteInfo() const override {
    constexpr auto Info = InfoStruct(Info_UID, TUID, Info_Random, TRandom, Info_DistinctTrials, TNS::Trials_t::Info, Info_NumTrials, TNS::Numbers);
    SerialWrite(Info);
  }
  void Start() const override {
    ArrangeTrials(TNS::Numbers.Array);
    SerialWrite<uint16_t>(TrialQueue.size());    
    RunAsync();
  }
  void Restore(uint8_t NDT, const RestoreInfo *RIs) const override {
    const std::unique_ptr<uint16_t[]> NumTrialsLeft = std::make_unique<uint16_t[]>(NumDistinctTrials);
    const RestoreInfo *const RIEnd = RIs + NDT;
    TrialsRestored = 0;
    for (uint8_t T = 0; T < NumDistinctTrials; ++T) {
      uint16_t NT = TNS::Numbers.Array[T];
      for (const RestoreInfo *RIBegin = RIs; RIBegin < RIEnd; ++RIBegin)
        if (TNS::Trials_t::Interfaces[T]->MyUID == RIBegin->TrialUID) {
          NT -= RIBegin->NumDone;
          TrialsRestored += RIBegin->NumDone;
          break;
        }
      NumTrialsLeft[T] = NT;
    }
    ArrangeTrials(NumTrialsLeft.get());
    RunAsync();
  }
};
template<UID TUID, bool TRandom, typename... TrialThenNumber>
bool &Session<TUID, TRandom, TrialThenNumber...>::NeedSetup = _NeedSetup<Session<TUID, TRandom, TrialThenNumber...>>;
template<typename... Ts>
const std::map<UID, const ISession *> SessionMap_t{ { Instance<Ts>.MyUID, &Instance<Ts> }... };