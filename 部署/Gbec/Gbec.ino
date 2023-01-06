#include <ArduinoSTL.h>
#include "ExperimentDesign.h"
UID State = State_SessionInvalid;
void SessionFinish() {
  SerialWrite(State = State_SessionFinished);
}
void setup() {
  Serial.setTimeout(-1);
  Serial.begin(9600);
  // PC端无法确认何时初始化完毕，不能提前发送信号，必须等待Arduino端宣布初始化完毕
  pinMode(pCapacitorVdd, OUTPUT);
  DigitalWrite<pCapacitorOut,HIGH>();
  ISession::FinishCallback = SessionFinish;
  SerialWrite(Signal_SerialReady);
}
const ISession *CurrentSession;
void Start() {
  const UID SessionUID = SerialRead<UID>();
  if (State == State_SessionRunning || State == State_SessionPaused) {
    SerialWrite(State);
    return;
  }
  const auto Query = SessionMap.find(SessionUID);
  if (Query == SessionMap.end()) {
    SerialWrite(Signal_NoSuchSession);
    return;
  }
  CurrentSession = (*Query).second;
  State = State_SessionRunning;
  SerialWrite(Signal_SessionStarted);
  CurrentSession->Start();
}
void Pause() {
  if (State == State_SessionRunning) {
    State = State_SessionPaused;
    CurrentSession->Pause();
  }
  SerialWrite(State);
}
void Continue() {
  if (State == State_SessionPaused) {
    SerialWrite(Signal_SessionContinue);
    State = State_SessionRunning;
    CurrentSession->Continue();
  } else
    SerialWrite(State);
}
void Abort() {
  if (State == State_SessionPaused || State == State_SessionRunning) {
    CurrentSession->Abort();
    State = State_SessionAborted;
  }
  SerialWrite(State);
}
void Peek() {
  SerialWrite(State);
}
void GetInfo() {
  const UID SessionUID = SerialRead<UID>();
  if (State == State_SessionRunning)
    SerialWrite(State);
  else if (SessionUID == Session_Current) {
    if (State == State_SessionInvalid)
      SerialWrite(State);
    else {
      SerialWrite(Signal_InfoStart);
      CurrentSession->WriteInfo();
    }
  } else {
    const auto Query = SessionMap.find(SessionUID);
    if (Query == SessionMap.end())
      SerialWrite(Signal_NoSuchSession);
    else {
      SerialWrite(Signal_InfoStart);
      (*Query).second->WriteInfo();
    }
  }
}
void Restore() {
#pragma pack(push, 1)
  struct RestoreCommand {
    UID SessionUID;
    uint8_t NumDistinctTrials;
  };
  const RestoreCommand RC = SerialRead<RestoreCommand>();
  const std::unique_ptr<RestoreInfo[]> RIs = std::make_unique<RestoreInfo[]>(RC.NumDistinctTrials);
  SerialRead(RIs.get(), RC.NumDistinctTrials);
  if (State == State_SessionRunning || State == State_SessionPaused)
    return State;
  const auto Query = SessionMap.find(RC.SessionUID);
  if (Query == SessionMap.end())
    return Signal_NoSuchSession;
  CurrentSession = (*Query).second;
  State = State_SessionRunning;
  SerialWrite(Signal_SessionRestored);
  CurrentSession->Restore(RC.NumDistinctTrials, RIs.get());
}
const ITest *CurrentTest = nullptr;
void TestStart() {
  struct TestInfo {
    UID TestUID;
    uint16_t TestTimes;
  };
#pragma pack(pop)
  const TestInfo TI = SerialRead<TestInfo>();
  if (State == State_SessionRunning) {
    SerialWrite(State_SessionRunning);
    return;
  }
  const auto Query = TestMap.find(TI.TestUID);
  if (Query == TestMap.end()) {
    SerialWrite(Signal_NoSuchTest);
    return;
  }
  CurrentTest = (*Query).second;
  if (CurrentTest->Start(TI.TestTimes)) {
    CurrentTest = nullptr;
    SerialWrite(Signal_TestStartedAutoStop);
  } else
    SerialWrite(Signal_TestStartedManualStop);
}
void TestStop() {
  const UID TestUID = SerialRead<UID>();
  if (TestUID == Test_Last)
    if (CurrentTest) {
      CurrentTest->Stop();
      CurrentTest = nullptr;
      SerialWrite(State == State_SessionRunning ? State_SessionRunning : Signal_TestStopped);
    } else
      SerialWrite(Signal_NoLastTest);
  else {
    const auto Query = TestMap.find(TestUID);
    if (Query == TestMap.end())
      SerialWrite(Signal_NoSuchTest);
    else {
      const ITest *const TestToStop = (*Query).second;
      TestToStop->Stop();
      if (TestToStop == CurrentTest)
        CurrentTest = nullptr;
      SerialWrite(State == State_SessionRunning ? State_SessionRunning : Signal_TestStopped);
    }
  }
}
void IsReady() {
  SerialWrite(Signal_SerialReady);
}
//从串口获取随机种子，因为Arduino设备没有可靠的获取真随机数的方法，测试发现读空模拟引脚也常常读到恒定值
void RandomSeed()
{
  std::ArduinoUrng::seed(SerialRead<uint32_t>());
}
void loop() {
  constexpr void (*APIs[])() = {
    Start,
    Pause,
    Continue,
    Abort,
    Peek,
    GetInfo,
    Restore,
    TestStart,
    TestStop,
    IsReady,
    RandomSeed,
  };
  const uint8_t API = SerialRead<uint8_t>();
  if (API < std::extent_v<decltype(APIs)>) {
    SerialWrite(Signal_ApiFound);
    APIs[API]();
  } else
    SerialWrite(Signal_ApiInvalid);
}