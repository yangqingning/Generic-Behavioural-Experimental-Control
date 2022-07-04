#include "EDPrelimit.h"
/*
绝大多数实验设计只需修改这个文件即可完成。涉及增删设备、属性、数据类型时，还需要修改UIDs.h。
本文件中可以包含多种会话（Session），每种会话包含多个回合（Trial），每个回合包含多个时相（Phase），每个时相有各自的工作硬件和时间设定。将这些信息全部上传到Arduino开发板后，可以在PC端通过串口控制，随时触发任何一种实验的启动，并接收实时回传数据。
*/
#pragma region 硬件设置
/*
程序总是要从最基础的底层开始搭建的，也就是硬件。硬件定义语句的通用语法如下：
const 硬件类型<引脚号,计时器号,时间参数,UID[,其它可选参数]> 设备名称[(&绑定打标器)];
[]中为可选项。
硬件类型：硬件类型分为打标器Tagger、刺激器Stimulator、监视器Monitor三大类。
引脚号：对于Arduino Mega 2560，可用的引脚号包括3~11、22~53。使用其它引脚将造成意外结果。
计时器号：绝大多数硬件都需要使用计时器。在Arduino Mega开发板上有6个计时器，分别编号0、1、2、3、4、5。其中1、3、4、5精度最高，2其次，0最低。应尽可能使用高精度计时器。必须使用低精度计时器的情况下，尽量给单次工作时间较长的设备分配高精度计时器。对于不可能同时工作的硬件，可以共享计时器；对于有可能需要同时工作的硬件，必须使用不同的计时器，否则会发生冲突。
时间参数：是硬件类型特定的，将在下面具体说明。除非特殊说明，硬件工作时间是与主程序并行的，不会造成后续步骤的延迟。
UID：是每个硬件设备的唯一标识符，是开发板通过串口向PC端报告设备工作状况、PC向开发板要求测试某个设备时的密码表。UID需要在UIDs.h中进行注册。添加新设备、新属性、新数据类型时，也都要添加新的UID。所有参与测试的设备之间必须具有不同的UID，不参与测试的设备可以跟参与测试的设备共享UID。另外，同一个设备可能会有多种不同的配置参数，它们也可以共享UID，但其中只有一个能参与测试。
设备名称：可以任意设置，主要供人类阅读。
绑定打标器：刺激器和监视器都可以绑定打标器。刺激器的打标器将与刺激器同时运行。监视器的打标器将在监测到信号时运行。对于纯行为实验，也可以不使用打标器。
但这个语法只是一个框架，对于具体的硬件类型，必须遵守特定的设置语法，将在下面具体说明。
为了在不同的实验套件之间复用代码，我们需要一些硬件设定组。譬如BOX1一套硬件，BOX2则用另一套硬件；它们可能具有不同的引脚号。这里用宏定义的方式确认究竟使用那一套硬件。
*/
//首先定义当前要上传的套件名称
#define TwoPhoton
//用#ifdef块定义该套件中要使用的硬件
#ifdef TwoPhoton
/*
打标器。设置语法：
const Tagger<引脚号,计时器号,毫秒数,UID> 设备名称;
打标器运行时，将在指定引脚给一个高电平，持续指定毫秒数后返回低电平。
*/
const Tagger<9, 1, 1000, Device_CD1> CD11s;
//此设备跟CD1实际上是同一个设备的不同用法，共享UID。
const Tagger<9, 1, 250, Device_CD1> CD1250ms;
const Tagger<23, 3, 20, Device_CD2> CD220ms;
const Tagger<23, 3, 200, Device_CD2> CD2200ms;
/*
刺激器。设置语法：
const 刺激器类型<引脚号,计时器号,时间参数,UID,是否结束提示> 设备名称[(&绑定打标器设备名称)];
[]中为可选。
刺激器类型：包括一闪刺激器OneShotStimulator、方波刺激器SquareWaveStimulator和声音刺激器ToneStimulator三种。
时间参数：取决于具体的刺激器类型。
刺激器的刺激有开始和结束之分。开始时会向串口发送信号。结束时是否发送可以配置，默认发送。
*/
//一闪刺激器：运行时给指定引脚一个高电平，经过指定毫秒数后，再变成低电平，刺激结束。有1个时间参数：毫秒数。
const OneShotStimulator<3, 4, 1000, Device_BlueLed> Blue1s(&CD11s);
//此设备跟BlueLed实际上是同一个设备的不同用法，共享UID。
const OneShotStimulator<3, 4, 250, Device_BlueLed> Blue250ms(&CD1250ms);
const OneShotStimulator<4, 4, 1000, Device_YellowLed> YellowLed(&CD11s);
const OneShotStimulator<6, 4, 1000, Device_ActiveBuzzer> ActiveBuzzer(&CD11s);
const OneShotStimulator<8, 2, 20, Device_WaterPump, false> Water20ms(&CD220ms);
const OneShotStimulator<8, 2, 200, Device_WaterPump, false> Water200ms(&CD2200ms);
const OneShotStimulator<7, 2, 20, Device_AirPuff, false> AirPuff(&CD220ms);
const OneShotStimulator<10, 0, 20, Device_Trigger1, false> Trigger1;
const OneShotStimulator<10, 0, 20, Device_Trigger2, false> Trigger2;
//方波刺激器：运行时给指定引脚一个高电平，持续指定毫秒数后变成低电平，持续指定毫秒数后再变成高电平，如此高低周期重复指定次数。有3个时间参数：高电平毫秒数、低电平毫秒数、重复高低周期数
const SquareWaveStimulator<11, 4, 20, 10, 100, Device_Optogenetic> Optogenetic(&CD11s);
//声音刺激器：运行时给指定引脚一个指定频率的方波，持续指定毫秒数。有2个时间参数：频率、毫秒数
const ToneStimulator<25, 4, 1000, 1000, Device_LowTone> LowTone(&CD11s);
const ToneStimulator<24, 4, 4000, 1000, Device_HighTone> HighTone(&CD11s);
/*
监视器。设置语法：
const Monitor<开关引脚,监视引脚,计时器号,UID> 设备名称[(&绑定打标器设备名称)];
开关引脚用于监视器的调零等操作。监视引脚返回是否检测到信号的信息。监测开始后，检测到信号就会进行打标。
*/
const Monitor<22, 26, 5, Device_LickCapacitor> LickCapacitor;
//为了在实验前对设备进行测试，将要测试的设备加入下述列表中，并正确填写设备总数；不希望被测试的设备不要加入此列表：
constexpr uint8_t NoCheckableDevices = 13;
const ITestable *const CheckableDevices[NoCheckableDevices] = {
	&CD11s,
	&CD220ms,
	&Blue1s,
	&YellowLed,
	&ActiveBuzzer,
	&Water20ms,
	&AirPuff,
	&Trigger1,
	&Trigger2,
	&Optogenetic,
	&LowTone,
	&HighTone,
	&LickCapacitor,
};
#endif
//如需再其它设备套件中使用，可以用另一套ifdef块，重新定义硬件。
#ifdef BOX1
//在此处定义用于BOX1的硬件配置
#endif
#pragma endregion
#pragma region 时相设置
/*
硬件配置完毕，下面定义时相。时相是构成回合的基块，运行一个回合就是依次运行多个时相，每个时相执行不同的硬件和等待任务。时相定义的基本语法结构：
const 时相模板<时相参数,UID> 时相名称[(&硬件名称)]
类似于硬件定义语法。但是，因为时相的进行不需要向串口汇报，也不需要测试，仅在实验结束后反馈信息时才会用到，因此，不在同一回合出现的时相可以共用UID。例如，同一回合一般不会既给高音又给低音，因此HighTonePhase和LowTonePhase可以共用同一个UID。
以下提供了4种时相模板，可以覆盖大部分需求：
*/
/*
冷静阶段。设置语法：
const CalmdownPhase<最小毫秒数,最大毫秒数,UID> 时相名称(&监视器名称);
该时相的功能是在最小和最大毫秒数之间随机抽取一个作为等待时间，然后在这个等待时间里运行监视器。一旦监视器检测到信号，就重新开始计时，直到在一个完整的等待时间里都没有监测到信号时，本阶段才结束。
*/
const CalmdownPhase<6000, 12000, Phase_Calmdown> CalmdownP(&LickCapacitor);
/*
刺激阶段。设置语法：
const StimulatePhase<计时器号,刺激后延时,UID> 时相名称(&刺激器名称);
该时相运行时，先立即运行刺激器，然后延迟指定的毫秒数。刺激器运行的过程与延迟是并行的，因此本阶段的最终时长等于刺激后延时，与刺激器运行时长无关。
*/
const StimulatePhase<5, 0, Phase_Trigger> Trigger10P(&Trigger1);
const StimulatePhase<5, 2000, Phase_Trigger> Trigger12sP(&Trigger1);
const StimulatePhase<5, 0, Phase_Trigger> Trigger20P(&Trigger2);
const StimulatePhase<5, 0, Phase_Stimulate> Blue0P(&Blue1s);
const StimulatePhase<5, 250, Phase_Stimulate> Blue250msP(&Blue250ms);
const StimulatePhase<5, 1000, Phase_Stimulate> Blue1sP(&Blue1s);
const StimulatePhase<5, 0, Phase_Stimulate> Audio0P(&ActiveBuzzer);
const StimulatePhase<5, 250, Phase_Stimulate> Audio250msP(&ActiveBuzzer);
const StimulatePhase<5, 1000, Phase_Stimulate> Audio1sP(&ActiveBuzzer);
const StimulatePhase<5, 0, Phase_Stimulate> Water20msP(&Water20ms);
const StimulatePhase<5, 0, Phase_Stimulate> Water200msP(&Water200ms);
const StimulatePhase<5, 0, Phase_Stimulate> AirP(&AirPuff);
/*
响应阶段。设置语法：
const ResponsePhase<毫秒数,超时是否响应,UID> 时相名称(&监测器名称,&响应器名称);
该时相运行后，将持续检测信号，一旦出现信号，运行响应器。如果达到指定的毫秒数仍未检测到信号，则根据超时是否响应的设定决定是否仍然给响应。
*/
const ResponsePhase<1000, true, Phase_Response> WaterR(&LickCapacitor, &Water20ms);
const ResponsePhase<1000, true, Phase_Response> AirR(&LickCapacitor, &AirPuff);
/*
等待阶段。设置语法：
const AwaitPhase<计时器号,最小毫秒数,最大毫秒数,UID> 时相名称;
该时相运行时，将在最小和最大毫秒数之间随机抽一个，等待，然后结束。
*/
const AwaitPhase<5, 2000, 2000, Phase_Await> BaseP;
const AwaitPhase<5, 8000, 18000, Phase_Await> ITI;
/*
监视阶段
*/
const MonitorPhase<5,1000,true,Phase_Monitor> ResponseWindow(&LickCapacitor);
#pragma endregion
#pragma region 回合设置
/*
时相定义完毕，下面要用时相构建回合。定义语法：
DefineTrial(时相数, UID, 回合名, &时相1, &时相2, …);
时相数目必须和时相列表一致。同一个会话中的不同回合必须有不同的UID，以便向串口返回信息；不可能在同一个会话中发生的回合则可以共享UID。回合运行时，将按照时相列表的顺序依次运行每个时相。
*/
DefineTrial(3, Trial_TriggerBlue, TriggerBlueT, &CalmdownP, &Trigger12sP, &Blue1sP);
DefineTrial(3, Trial_TriggerAudio, TriggerAudioT, &CalmdownP, &Trigger12sP, &Audio1sP);
DefineTrial(3, Trial_TriggerWater, TriggerWaterT, &CalmdownP, &Trigger12sP, &Water200msP);
DefineTrial(3, Trial_TriggerAir, TriggerAirT, &CalmdownP, &Trigger12sP, &AirP);
DefineTrial(4, Trial_TriggerBlueLickWater, TriggerBlueLickWaterT, &CalmdownP, &Trigger12sP, &Blue0P, &WaterR);
DefineTrial(4, Trial_TriggerAudioLickWater, TriggerAudioLickWaterT, &CalmdownP, &Trigger12sP, &Audio0P, &WaterR);
DefineTrial(3, Trial_BlueLickWater, BlueLickWaterT, &CalmdownP, &Blue0P, &WaterR);
DefineTrial(3, Trial_AudioLickWater, AudioLickWaterT, &CalmdownP, &Audio0P, &WaterR);
DefineTrial(2, Trial_BlueOnly, BlueOnlyT, &CalmdownP, &Blue0P);
DefineTrial(2, Trial_AudioOnly, AudioOnlyT, &CalmdownP, &Audio0P);
DefineTrial(2, Trial_WaterOnly, WaterOnlyT, &CalmdownP, &Water20msP);
DefineTrial(2, Trial_AirOnly, AirOnlyT, &ITI, &AirP);
DefineTrial(3, Trial_TriggerBase, TriggerBaseT, &CalmdownP, &Trigger12sP, &BaseP);
//所有喷气训练，一般前置2s基线，后置ITI，以便MATLAB端在回合开始时定时拍摄，减小视频文件体积
DefineTrial(4, Trial_TriggerBlueAir, TriggerBlueAirT, &Trigger12sP, &Blue0P, &AirR, &ITI);
DefineTrial(4, Trial_TriggerAudioAir, TriggerAudioAirT, &Trigger12sP, &Audio0P, &AirR, &ITI);
DefineTrial(4, Trial_BlueAir, BlueAirT, &BaseP, &Blue0P, &AirR, &ITI);
DefineTrial(4, Trial_AudioAir, AudioAirT, &BaseP, &Audio0P, &AirR, &ITI);
#pragma endregion
#pragma region 会话设置
//最后定义会话。会话是一系列回合的重复。因为需要接受PC端指令启动会话，必须将所有会话包装在一个数组中，并且明确指定会话的个数。
//常用的数量分配方案
constexpr uint16_t PureStimuScan[3] = {5, 5, 5};
constexpr uint16_t TriggerStimuScan[4] = {5, 5, 5, 5};
constexpr uint16_t TriggerWaterTrain[3] = {20, 30, 5};
constexpr uint16_t TriggerAirTrain[3] = {20, 80, 5};
constexpr uint16_t WaterTrain[1] = {50};
constexpr uint16_t AirTrain[1] = {100};
constexpr uint8_t NoSessions = 12;
const ISession *const SessionList[NoSessions] =
	{
		/*
		会话定义语法：
		new Session<不同回合数, 是否固定顺序, UID>(TrialList(不同回合数, &回合1, &回合2, …), NumberList(不同回合数, 回合1重复数, 回合2重复数, …)),
		一个会话中可以有多种不同的回合，每个回合可以重复多次。这些回合重复可以按照固定顺序排列，也可以随机排列。如果使用随机排列，仍然需要设定每个回合重复数，类似于无放回采样。
		*/
		new Session<3, false, Session_TriggerBlueLickWater>(TrialList(3, &TriggerBlueLickWaterT, &BlueLickWaterT, &TriggerBaseT), TriggerWaterTrain),
		new Session<3, false, Session_TriggerAudioLickWater>(TrialList(3, &TriggerAudioLickWaterT, &AudioLickWaterT, &TriggerBaseT), TriggerWaterTrain),
		new Session<3, true, Session_TriggerBlueAir>(TrialList(3, &TriggerBlueAirT, &BlueAirT, &TriggerBaseT), TriggerAirTrain),
		new Session<3, true, Session_TriggerAudioAir>(TrialList(3, &TriggerAudioAirT, &AudioAirT, &TriggerBaseT), TriggerAirTrain),
		new Session<1, true, Session_BlueLickWater>(TrialList(1, &BlueLickWaterT), WaterTrain),
		new Session<1, true, Session_AudioLickWater>(TrialList(1, &AudioLickWaterT), WaterTrain),
		new Session<1, true, Session_BlueAir>(TrialList(1, &BlueAirT), AirTrain),
		new Session<1, true, Session_AudioAir>(TrialList(1, &AudioAirT), AirTrain),
		new Session<3, false, Session_BlueAudioWater>(TrialList(3, &BlueOnlyT, &AudioOnlyT, &WaterOnlyT), PureStimuScan),
		new Session<3, false, Session_BlueWaterAir>(TrialList(3, &BlueOnlyT, &WaterOnlyT, &AirOnlyT), PureStimuScan),
		new Session<4, false, Session_TriggerBaseBlueAudioWater>(TrialList(4, &TriggerBaseT, &TriggerBlueT, &TriggerAudioT, &TriggerWaterT), TriggerStimuScan),
		new Session<4, false, Session_TriggerBaseBlueWaterAir>(TrialList(4, &TriggerBaseT, &TriggerBlueT, &TriggerWaterT, &TriggerAirT), TriggerStimuScan)};
#pragma endregion