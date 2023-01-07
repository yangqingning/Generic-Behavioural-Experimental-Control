#pragma once
#include "Predefined.h"
#define Pin constexpr uint8_t

// 引脚设置。建议遵守命名规范：p开头表示名称指向一个引脚号（Pin）

Pin pCD1 = 9;
Pin pBlueLed = 3;
Pin pActiveBuzzer = 6;
Pin pWaterPump = 8;
Pin pAirPuff = 7;
Pin pCapacitorVdd = 22;
Pin pCapacitorOut = 18;
Pin pPassiveBuzzer = 25;
//使用一个模拟输入引脚获取随机种子，这个引脚应该是空闲状态
Pin pRandomPin = 1;

// 关于计时器参数TimerCode。Arduino Mega 有0~5共6个计时器，其中1、3、4、5精度最高，2其次，0最低。应尽可能使用高精度计时器。必须使用低精度计时器的情况下，尽量给单次工作时间较长的设备分配高精度计时器。对于不可能同时工作的硬件，可以共享计时器；对于有可能需要同时工作的硬件，必须使用不同的计时器，否则会发生冲突。

const auto& TestMap = TestMap_t<
  /*设备测试，在下方列出要运行的测试，逗号分隔。

	测试有两种类型：PinFlashTest和MonitorTest。

	PinFlashTest<MyUID,Pin,TimerCode,Milliseconds,HighOrLow = HIGH>
	此测试运行时，将把指定引脚设为高或低电平，在指定毫秒数后反转。
	参数：
	MyUID，唯一标识此测试的UID
	Pin，要测试的设备引脚
	TimerCode，要使用的计时器。请勿在上一测试未结束时，运行与其共享同一个计时器的下一个测试。
	Milliseconds，单次测试中，电平维持的毫秒数
	HighOrLow，要设为高电平还是低电平，可选HIGH或LOW

	MonitorTest<MyUID,Pin>
	此测试运行时，每次指定引脚从低电平变成高电平，将向串口发出Signal_MonitorHit信号。
	参数：
	MyUID，唯一标识此测试的UID
	Pin，要监视的引脚。注意，只能监视支持中断的引脚，包括2, 3, 18, 19, 20, 21
	*/
  PinFlashTest<Test_CD1, pCD1, 0, 200>,
  PinFlashTest<Test_BlueLed, pBlueLed, 3, 200>,
  PinFlashTest<Test_ActiveBuzzer, pActiveBuzzer, 4, 200>,
  PinFlashTest<Test_Water, pWaterPump, 5, 150>,
  PinFlashTest<Test_Air, pAirPuff, 2, 150>,
  PinFlashTest<Test_CapacitorReset, pCapacitorVdd, 1, 100, LOW>,
  MonitorTest<Test_CapacitorMonitor, pCapacitorOut, pCapacitorVdd, 1>>;

// 步骤设计。建议StepName遵守命名规范：s开头表示名称指向一个步骤（Step）

/*写串口步骤

此类步骤通常用作其它步骤的汇报器。方便起见，这里用S作为SerialWriteStep的别名。可用S<Signal>语法表示向串口发送Signal，其中Signal是Signal_前缀的UID。
*/

template<UID Signal>
using S = SerialWriteStep<Signal>;

/*冷静类步骤

在指定范围内随机抽取一个毫秒数，然后在该毫秒数的时间内监视指定引脚。如果时间内出现高电平，将重新开始计时，直到在某次重置计时时间内未出现任何一次高电平，才能结束该步骤，进入下一步。语法：
using StepName=CalmdownStep<Pin,TimerCode,MinMilliseconds,MaxMilliseconds=MinMilliseconds,MyUID=Step_Calmdown>;

参数：
Pin，要监视的引脚。注意，只能监视支持中断的引脚，包括2, 3, 18, 19, 20, 21
TimerCode，要使用的计时器
MinMilliseconds，最小随机毫秒数
MaxMilliseconds，最大随机毫秒数
MyUID，标识该步骤的UID，在返回信息时供人类识别
*/

using sCalmDown = CalmdownStep<pCapacitorOut, 1, 5000, 10000>;

/*引脚闪烁类步骤

让引脚高电平指定毫秒数，再回到低电平。此类步骤异步执行，一开始就立即结束进入下一步，而不会等待引脚回到低电平，不占用主时间轴。语法：
using StepName=PinFlashStep<Pin,TimerCode,Milliseconds,UpReporter=NullStep,DownReporter=NullStep,MyUID=Step_PinFlash>;

参数：
Pin，要闪烁的引脚
TimerCode，要使用的计时器
Milliseconds，维持高电平的毫秒数
UpReporter,DownReporter，提醒闪烁开始和结束的汇报器，可以使用使用S<Signal>语法写串口，或者NullStep表示无需汇报。
MyUID，标识该步骤的UID，在返回信息时供人类识别
*/

using sLight = PinFlashStep<pBlueLed, 3, 200, S<Signal_LightUp>, S<Signal_LightDown>, Step_Light>;
using sAudio = PinFlashStep<pActiveBuzzer, 3, 200, S<Signal_AudioUp>, S<Signal_AudioDown>, Step_Audio>;
using sWater = PinFlashStep<pWaterPump, 3, 150, S<Signal_WaterOffered>, NullStep, Step_Water>;
using sAir = PinFlashStep<pAirPuff, 3, 150, S<Signal_AirPuff>, NullStep, Step_Air>;
using sTag = PinFlashStep<pCD1, 4, 200, NullStep, NullStep, Step_Tag>;

/*监视类步骤

在指定毫秒数内监视指定引脚。如果发现高电平，汇报命中。还可用扩展旗帜指定增强功能。语法：
using StepName=MonitorStep<Pin,TimerCode,Milliseconds,Flags,HitReporter,MissReporter=NullStep,MyUID=Step_Monitor>;

参数：
Pin，要监视的引脚。注意，只能监视支持中断的引脚，包括2, 3, 18, 19, 20, 21
TimerCode，要使用的计时器
Milliseconds，要监视的毫秒数

Flags，用按位或（|）符号组合多个可选功能旗帜。可选以下功能：
- Monitor_ReportOnce，只汇报第1次命中，以后再命中就不再汇报。如不指定此旗帜，将在步骤结束前重复汇报每次命中。
- Monitor_HitAndFinish，只要发现1次命中，汇报后立即结束步骤，不等待时间结束。如不指定此旗帜，将总是等待指定毫秒数结束，无论命中情况如何。
- 0，不指定任何旗帜

HitReporter，用于汇报命中的步骤。可以使用使用S<Signal>语法写串口，或者NullStep表示无需汇报。
MissReporter，用于汇报错失的步骤。必须指定Monitor_ReportMiss旗帜，此参数才有效。可以使用使用S<Signal>语法写串口，或者NullStep表示无需汇报。
MyUID，标识该步骤的UID，在返回信息时供人类识别
*/

using sMonitorLick = MonitorStep<pCapacitorOut, 5, 1000, Monitor_ReportOnce, S<Signal_MonitorHit>, S<Signal_MonitorMiss>>;
using sResponseWindow = MonitorStep<pCapacitorOut, 5, 2000, Monitor_ReportOnce, sWater, S<Signal_MonitorMiss>>;

/*等待类步骤

在指定范围内随机抽取一个毫秒数，静默等待那个毫秒数的时间，然后结束步骤。语法：
using StepName=WaitStep<TimerCode,MinMilliseconds,MaxMilliseconds=MinMilliseconds,MyUID=Step_Wait>;

参数：
TimerCode，要使用的计时器
MinMilliseconds，最少等待的毫秒数
MaxMilliseconds，最大等待的毫秒数。可以设置为与MinMilliseconds相等，这样等待时间将是固定的。
MyUID，标识该步骤的UID，在返回信息时供人类识别
*/

using sFixedITI = WaitStep<2, 20000>;
using sRandomITI = WaitStep<2, 10000, 20000>;
using sFixedPrepare = WaitStep<2, 2000>;
using sDelay = WaitStep<2, 1000>;

/*后台监视类步骤

类似于监视类步骤，但不占用主时间轴，而是在后台持续监视，不断汇报每次命中。此类包含一个开始步骤和一个终止步骤，都是立即结束，不占用主时间轴。语法：
using StepName=StartMonitorStep<Pin,Reporter,MyUID=Step_StartMonitor>;
using StepName=StopMonitorStep<Pin,Reporter,MyUID=Step_StopMonitor>;
开始和终止步骤的Pin和Reporter参数必须相同，配对出现。开始步骤后即开始在后台持续监视，后续其它步骤继续正常执行，直到配对的终止步骤后才结束监视。

参数：
Pin，要监视的引脚。注意，只能监视支持中断的引脚，包括2, 3, 18, 19, 20, 21
Reporter，汇报命中的步骤。此步骤总是异步执行，不等待立刻结束，无论步骤类本身是否宣称其为异步。
MyUID，标识该步骤的UID，在返回信息时供人类识别
*/

using sStartMonitor = StartMonitorStep<pCapacitorOut, S<Signal_HitCount>>;
using sStopMonitor = StopMonitorStep<pCapacitorOut, S<Signal_HitCount>>;

/*音调类步骤

此步骤在指定引脚上播放指定频率的音调。此类步骤异步执行，一开始就立即结束进入下一步，而不会等待播放结束，不占用主时间轴。语法：
using StepName=ToneStep<Pin,TimerCode,FrequencyHz,Milliseconds,UpReporter=NullStep,DownReporter=NullStep,MyUID=Step_Audio>;

参数：
Pin，要播放的引脚
TimerCode，要使用的计时器
FrequencyHz，音调频率㎐
Milliseconds，播放毫秒数
MyUID，标识该步骤的UID，在返回信息时供人类识别
*/

using sLowTone = ToneStep<pPassiveBuzzer, 3, 500, 200, S<Signal_LowUp>, S<Signal_LowDown>, Step_LowTone>;
using sHighTone = ToneStep<pPassiveBuzzer, 3, 5000, 200, S<Signal_HighUp>, S<Signal_LowDown>, Step_HighTone>;

/*回合设计。
一个回合由多个步骤串联而成。语法：
using TrialName=Trial<MyUID,Step1,Step2,…>;

参数：
TrialName，回合名称，建议遵守命名规范：t开头表示名称指向一个回合（Trial）
MyUID，标识该回合的UID，在返回信息时供人类识别
Step1,Step2,…，依次排列要在该回合内执行的步骤
*/

using tLightOnly = Trial<Trial_LightOnly, sCalmDown, sLight, sTag, sMonitorLick, sFixedITI>;
using tAudioOnly = Trial<Trial_AudioOnly, sCalmDown, sAudio, sTag, sMonitorLick, sFixedITI>;
using tWaterOnly = Trial<Trial_WaterOnly, sCalmDown, sWater, sTag, sMonitorLick, sFixedITI>;
using tLightWater = Trial<Trial_LightWater, sCalmDown, sLight, sTag, sMonitorLick, sWater, sFixedITI>;
using tAudioWater = Trial<Trial_AudioWater, sCalmDown, sAudio, sTag, sMonitorLick, sWater, sFixedITI>;
using tLightAir = Trial<Trial_LightAir, S<Signal_StartRecord>, sFixedPrepare, sLight, sDelay, sAir, sRandomITI>;
using tLightDelayWater = Trial<Trial_LightDelayWater, sCalmDown, sLight, sDelay, sResponseWindow>;

const auto& SessionMap = SessionMap_t<
  /*会话设计
	一个实验会话定义了要做哪几种回合，每种回合重复多少次，以及是固定顺序还是随机穿插。在下方列出会话设计项，语法：
	Session<MyUID,Random,Trial1,N<Numer1>,Trial2,N<Number2>,…>;

	参数：
	MyUID，标识该会话的UID，在返回信息时供人类识别，以及电脑给Arduion指示该运行哪个会话
	Random，若true则每个回合将打乱顺序随机穿插，若false则按照定义的顺序逐个运行
	Trial1,Trial2,…，要运行的回合
	Number1,Number2,…，每个回合的重复次数
	*/
  Session<Session_LAWLw, true, tLightOnly, N<20>, tAudioOnly, N<20>, tWaterOnly, N<20>, tLightWater, N<20>>,
  Session<Session_LAWLwAw, true, tLightOnly, N<20>, tAudioOnly, N<20>, tWaterOnly, N<20>, tLightWater, N<20>, tAudioWater, N<20>>,
  Session<Session_LightWater, false, tLightWater, N<30>>,
  Session<Session_AudioWater, false, tAudioWater, N<30>>,
  Session<Session_LightAir, false, tLightAir, N<30>>,
  Session<Session_SurveillanceThroughout, false, Trial<Trial_StartMonitor, sStartMonitor>, N<1>, tWaterOnly, N<5>, tLightDelayWater, N<10>, Trial<Trial_StopMonitor, sStopMonitor>, N<1>> >;