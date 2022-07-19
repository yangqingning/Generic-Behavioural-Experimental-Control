通用行为实验控制，MATLAB与Arduino串口连接，提供高度可扩展可配置的模块化一站式服务
# 项目背景
实验室的自动化行为实验控制大多用Arduino开发板实现。Arduino开发板是一种嵌入式计算机，可以通过杜邦线引脚连接到硬件设备模组，实现对小鼠的感官刺激和行为记录。为了对实验过程编程，需要编写代码。Arduino官方提供IDE，可以将C++代码编译上传到开发板执行。此外，MATLAB也提供Arduino开发工具箱，允许使用MATLAB语言对Arduino开发板进行编程。基于这两套技术栈，我们实验室流传下来大量经典的行为实验控制代码。

但是，这整个生态系统都存在根本性的严重问题：
- 从 Arduino IDE 上传的C++代码只能在Arduino中执行，无法将收集到的行为数据传回主机进行处理和存储。主机端必须另外配置数据收集解决方案。
- MATLAB工具箱并不能将MATLAB代码编译成Arduino二进制代码，而是将预编译的服务器上传到Arduino，然后主机端MATLAB通过连接Arduino的USB串口一条一条发送指令，交给Arduino执行。这除了导致只能使用MATLAB提供的指令、无法进行多线程编程以外，指令通过串口的传输也存在延迟，对时间精度要求较高的刺激（例如光遗传激光波形、短暂的水泵给水）无法正确完成。
- 祖传代码大多为了特定的实验临时编写，代码设计相对固定，可扩展、可配置性极差，缺乏良好的设计思路。

为了解决这些问题，必须重新编写一个横跨Arduino与主机MATLAB两端的桥梁，并采用高度模块化的代码结构设计，并确保将时间精度要求高的过程由Arduino端独立决策完成。于是有此项目。

本实验控制系统分为Arduino服务端和MATLAB客户端两个部分。使用本系统的优越性：
- 连接PC端和Arduino只为传简单信号，Arduino端独立决策，比用MATLAB端步步操纵Arduino具有更高的时间精度；
- 高度可自定义性，清晰的模块化组件化搭配，可重用性高；
- 随时暂停、放弃实验，不需要拔插头；
- 实验过程中意外掉线自动重连，不会丢失进度
# 配置环境
安装本工具箱之前需要先安装[Arduino IDE 2.0.0-rc8 以上版本](https://github.com/arduino/arduino-ide/releases)，以及 Arduino Library Manager 中的TimersOneForAll库。

此外，本实验系统还依赖[埃博拉酱的MATLAB扩展](https://ww2.mathworks.cn/matlabcentral/fileexchange/96344-matlab-extension)。正常情况下，此工具箱在安装本工具箱时会被自动安装。如果安装失败，可以在MATLAB附加功能管理器中安装。

工具箱安装成功后，请务必查看快速入门指南（GettingStarted.mlx），执行进一步安装配置。

本项目是行为实验通用控制程序，因此针对不同的实验设计，你需要自行编写一些自定义Arduino代码。最常用的就是ExperimentDesign.cpp，各种实验参数都需要在这里设置好然后上传到Arduino。对于较大的改动，如硬件、回合、实验方案等的增删，可能还需要修改UIDs.h。

ExperimentDesign.cpp的具体配置语法，在文件中有详细的注释说明。但在那之前，建议先浏览一遍本自述文件。
# Arduino C++ 代码结构
使用本项目之前，你需要理解本项目的概念层级。

从顶级到低级，一次实验包含会话、回合、时相、设备四个层级。我们描述这些层级的常用说法包括：

- 会话：今天做了一个蓝光喷气闭眼实验，喷气100次，随机穿插了20个不喷气的Trial
- 回合：这个Trial没给喷气刺激
- 时相：实验一开始是一个Pretrial等待阶段，需要连续6秒不舔水才继续
- 设备：蓝光1s、喷气30㎳、给水20㎳、检测舔水的电容

因此我们得到四个概念的定义：
### 设备
定义了连接到Arduino开发板的设备的引脚号和典型的使用方法。如果一个设备在不同会话、回合、时相中采用不同的使用方法，则可以定义为具有相同引脚的多个设备，尽管它们在物理上是同一个硬件。本项目提供了3种设备模板：
- 监视器，能够在一段时间内监视某个信号，一旦信号出现，返回给程序。可用于电容等传感器。
- 刺激器，能够通过一定的电平模式控制其工作一段时间。工作时间是与主时相流程并行的，延长工作时间不会导致时相延长。包含下面3个子类：
	- 一闪刺激器，能够给予一段时间的高电平使其工作，然后低电平使其停止。可用于施加单一刺激。
	- 方波刺激器，能够输出方波信号的刺激器，例如光遗传，亮多少毫秒暗多少毫秒
	- 声音刺激器，能够输出方波声音的设备，可以指定频率和时长等。
- 打标器，类似于一闪刺激器，但不用于给动物施加刺激，而是给记录设备打标用。
### 时相
定义了一段具有特殊性质的时间控制流程。本项目提供了3种时相模板：
- 等待相。该时相不会运行任何设备，单纯等待一段时间。这个时间可以设置为在一个范围内抽取随机数。
- 冷静相。该时相需要一个监视器，设定监视器必须连续一段时间没有收到任何信号才能结束该时相。这个时间段的长度可以设置为一定范围内的随机数。如果在时间段结束之前收到了信号，那么将重置计时器，重新开始该时间段。该时相不会给动物任何刺激。
- 响应相。该时相需要一个监视器和一个刺激器。时相开始后，传感器持续监视，直到收到信号，运行刺激器，然后时相结束。如果一直收不到信号，也可以设置一个最大时间，超时则时相结束。超时后是否仍然运行刺激器，可以自行配置。
- 刺激相。该时相需要一个刺激器。时相开始后立刻运行刺激器，然后可选地等待一段固定时间。如果需要等待随机时间，请使用0等待的刺激相后接一个等待相。
### 回合
定义了几个时相的顺序组合，并可以在会话当中以固定或随机的顺序重复交替运行。
### 会话
一次实验的最顶级控制单元。一个会话可以包含多个不同的回合，并且可以设置这些回合各自的重复次数，以及这些回合是按固定顺序还是随机顺序交替运行。
## UID与Arduino-PC通信
系统使用UID作为“密码表”实现Arduino与PC的通信。PC向Arduino发送UID指示要运行哪个会话；Arduino向PC发送UID指示当前运行到哪个回合，等等。所有的设备、时相、回合、会话都具有各自的UID，需要在定义时指定，并将该UID注册在UIDs.h中。PC端也需要同样的UID密码表，但可以在安装后用Gbec.GenerateMatlabUIDs自动生成，无需手动操作。

UID具有命名规范，不遵守命名规范可能造成意外错误。除了一些系统保留的内置UID以外，新增的UID必须命名为`类型_实例`结构。常用的类型包括：
- Device。如果你要添加未列出的全新设备，需要添加Device类的UID。例如`Device_BlueLed`表示一个蓝色LED灯。注意，Device类UID不一定要和设备一一对应。只要在实验设计的概念层次上不会造成混淆，就可以给多个设备对象共享UID。但是，在设备测试（见SelfCheck_Client.mlx）阶段，UID是和设备对象一一对应的，因此需要参与测试的设备必须具有不同的UID。
- Phase。如果你设计了一种未列出的新时相，需要指定Phase类的UID。例如`Phase_Calmdown`表示一个配置了冷静相。一般来说，只要保证同一个回合内的时相具有不同的UID即可，不会出现在同一个回合内的时相可以共享相同的UID。
- Trial。每当你设计了一种未预设的新回合，需要指定Trial类的UID。例如`Trial_BlueAir`表示一个简单的蓝光喷气偶联刺激回合。一个会话内的回合一般应当具有不同的UID，不可能在同一个会话内出现的回合可以共享相同的UID。
- Session。每当你设计了一种未预设的新会话，需要指定Session类的UID。例如`Session_BlueAir`表示一个由多次蓝光喷气偶联回合串联成的会话。每一个会话都必须具有独特的UID，不允许任何两个会话拥有相同的UID，因为PC端需要通过UID唯一指定一个具体的会话要求Arduino运行。

还有一些其它类型的UID用法，参见[高级](#高级)部分。
## 设备测试
在实验开始之前通常要进行设备测试。设备测试需要考虑以下问题：
- 不同的设备可能具有不同的用法
- 同一个设备也可能有不同的用法
- 有些设备可能无法或不需要测试

本项目维护一个“可测试设备列表”来解决上述问题。用户定义设备对象以后，如果该设备需要参与测试，则将其添加到列表中（该列表在ExperimentDesign.cpp中定义）。测试时，PC端将发送UID来指示Arduino要测试哪个设备，因此参与测试的所有设备必须具有各不相同的UID。因为一种设备可以对应多个设备对象，只要这些对象具有不同的UID，你就可以用多种方法对设备进行测试。
## 计时器设置
对于具有时间控制的对象，必须指定一个物理计时器。Arduino Mega 2560开发板具有0、1、2、3、4、5共6个计时器。其中0精度最低，1、3、4、5精度最高，2居中。一个计时器只能同时执行一个计时任务，即凡是有可能同时运行的两个对象，都不能指定同一个计时器。所有设备都需要计时器，刺激相和等待相也需要计时器。

本项目提供了一种优化的分配方案，可以比较好地充分利用各个计时器而不发生冲突：
- 0号计时器用于触发器。因为触发器只需要有一个足够长的高电平即可，对时间精度要求低。
- 1、3计时器分别用于两个物理打标器设备，因为打标器通常要求较高的时间精度，且有可能同时运行。
- 2号计时器用于US类的刺激器，如给水、喷气等。因为这类刺激通常对时间精度要求低，且一般不会需要同时给两个US。
- 4号计时器用于CS类的刺激器，如蓝光、声音、光遗传等。这类刺激通常也要求较高的时间精度，但通常不需要同时给两个CS。
- 5号计时器用于监视器和时相控制。需要使用计时器的刺激相和等待相都不需要监视器，因此不会冲突。冷静相虽然需要计时，但其计时功能是调用监视器实现的，因此本身并不需要占用一个计时器。

当然上述方案只是参考建议。如果你有特殊需求（比如同时给两个CS），可能需要自定义分配方案。
## 高级
如果上述配置无论怎样组合都无法满足你的需求，你将需要一些更复杂的编码工作。本文假定你对C++语言熟练掌握或具有足够强的检索能力，不再对一些术语进行解释。下面列出一些常见高级更改的指南：
### 新增一种全新的设备模板
如果项目中预设的设备模板都不符合需求，或者你的设备具有特殊的复杂控制方式，你将需要自己写控制代码来控制该设备。

对于监视器，建议继承自MonitorBase抽象类。要实例化该类，你需要实现以下方法：
```C++
MonitorResult StartMonitor(uint16_t Milliseconds, InterruptResult (*CancelCheck)()) const override;
/*当开始监视时，上级时相对象会调用监视器的StartMonitor方法，并提供Milliseconds参数，提示最长监视多少毫秒；以及一个CancelCheck函数指针，以支持实验暂停、中断等操作。一般来说，你需要在指定的毫秒数内进行持续的监视，并不断调用CancelCheck以检查是否应当中断执行，因为PC端随时会发送指令要求实验中断。MonitorResult和InterruptResult都是枚举类型，可以在定义文件中找到每个枚举值的实际含义。
调用方不提供计时器。你需要在类内自行维护计时器。建议参考本项目已提供的Monitor类的做法，引用TimersOneForAll库的计时器操纵方法。*/

void Test(uint16_t Times) const override;
//该方法用于实现设备测试功能。Times不是时间，而是要检测的信号个数。每次检测到一个信号，应当在串口中写入Signal_Detected，直到检测到信号数达到Times的要求则返回。Times可能为0，此时表示无限循环检测（而不是检测0次立即返回）。无论次数是否有限，都应当持续检查串口是否有PC端发来的信号；一旦收到Command_CheckOver信号，就应当立即终止测试，并在返回之前向串口发送Signal_TestFinished。

void WriteInformation() const override;
/*
为了向PC端发送实验设定参数，该方法要求你将设备的各种参数依次写入串口。
如果要写的参数是单个值，可以使用WriteProperty宏，但要确保你的属性名称和数据类型已经在UIDs.h中注册。例如UID属性注册为Property_UID，该属性是UIDs类型，注册为Type_UIDs。
如果要写的参数是个类对象，确保那个类对象实现了IInformative接口，这样可以直接使用WriteComponent宏。同样属性名需要在UIDs.h中注册。如果没有实现接口，则你需要参照WriteComponent宏的定义，将那个类对象的各种属性包装在UIDs.StructStart和UIDs.StructEnd对组里。
如果要写的参数是个数组，则你需要按照以下顺序写串口来定义这个数组：
属性UID，为Property_前缀
UIDs.ArrayStart
数组元素个数的数据类型，如Type_uint8_t
数组元素个数
循环，依次写入各个数组元素。特别地，如果每个元素都是一个结构体/类对象，则只有第一个元素需要包裹在StructStart和StructEnd对组里，后续元素的StructStart必须省略，但StructEnd不能省略。
可以参见Trial.h的WriteInformation方法，学习数组类属性的写法。
*/

UIDs GetUID() const override;
//返回设备的UID。顾名思义，无需多言。
```
可以参见[Monitor.h](Monitor.h)的示例写法。

对于刺激器，建议继承自Stimulator抽象类。要实例化该类，你需要实现以下方法：
```C++
void Test(uint16_t Times) const override;
//Times表示需要连续给多少个刺激。不同于监视器，此处为0表示不给刺激直接返回，而不是无限给刺激。

void Action() const override;
//运行刺激器，给一个刺激。请使用计时器并行调度整个刺激行为，不要在主线程中执行等待，以免影响时间控制流。该函数应当仅仅是布置一个刺激任务然后立即返回，而不应当等待刺激完成再返回。此外，刺激完成后还应当调用基类的OnStimulatorDown方法，提示刺激完成，以便向PC端发送信号。

void WriteInformation() const override;
```
可以参见[SquareWaveStimulator.h](SquareWaveStimulator.h)的示例写法。

对于打标器，至少应当继承IRunnable接口；如果希望支持测试，还应当继承ITestable接口，实现以下方法：
```C++
void Test(uint16_t Times) const override;

void Run() const override;
//运行打标器，同样需注意不要占用主线程的时间，布置完任务立刻返回。但无需像刺激器那样在打标完成后调用什么别的方法。

void WriteInformation() const override;
UIDs GetUID() const override;
```
可以参见[Tagger.h](Tagger.h)的示例写法。

如果你的设备不在上述三大类之中，则你需要考虑是否实现一些接口：
- IRunnable。可以在时相中独立运行的设备，必须实现IRunnable接口。
- ITestable。可以在测试阶段测试的设备，必须实现ITestable接口。
### 新增一种全新的时相模板
为了能够装载到现有的回合模板中，你的新时相需要继承IInterruptable接口，实现以下方法：
```C++
bool TryRun() const override;
//轮到你的时相运行时，上级的回合对象会调用此函数。一个时相有可能被PC端指令中断，如果发生了中断，则立即返回true，提示上级回合对象应当中断；否则返回false，表示可以继续运行。

void WriteInformation() const override;
//应当在该函数中写出你的时相涉及的所有设备信息。这些设备最好是实现了IInformative接口的，这样你可以直接使用宏WriteComponent(设备对象)来写出该设备。
```
可以参见[CalmdownPhase.h](CalmdownPhase.h)的示例写法。
### 新增一种全新的回合模板
为了能够装载到现有的会话中，你的新回合需要继承IInterruptablePublicUid接口，实现以下方法：
```C++
bool TryRun() const override;
void WriteInformation() const override;
UIDs GetUID() const override;
```
可以参见[Trial.h](Trial.h)的示例写法。
### 新增一种全新的会话模板
会话必须实现ISession接口才能装载到会话列表中，实现以下方法：
```C++
bool Continue(uint8_t NoDistinctUIDs, const UIDs *FinishedUID, const uint16_t *FinishedNumbers) const override;
/*
为了实现自动断线重连，你的会话应当支持从中断处继续。系统将会给你的会话提供三个参数，这三个参数提供了足够的信息让你能够恢复中断的会话：
NoDistinctUIDs，指示中断之前有几个不同的回合已经运行过了。该数值实际上是下面FinishedUID数组的长度。
FinishedUID，该数组指示有哪些不同的回合已经运行过了。注意，这个数组不一定包含你的会话所能够支持的所有不同的回合。如果某些回合尚未运行就中断了，这个列表将不包含那些回合。
FinishedNumbers，该数组和FinishedUID一一对应，指示每个回合各自已经运行了几次。
会话随时可能中断，因此一旦发生了中断，应当返回true表示会话被中断了；否则返回false表示会话是正常结束的。
*/
bool TryRun() const override;
void WriteInformation() const override;
UIDs GetUID() const override;
```
# MATLAB代码结构
使用前需导入包：
```MATLAB
import Gbec.*
```
安装阶段生成的Development_Client, Experiment_Client和SelfCheck_Client是你的控制面板。一般先根据脚本中已有的提示信息进行Arduino端配置、设备检查（SelfCheck_Client），然后开始实验（Experiment_Client）

每个类/函数的详细文档可用doc命令查看。以下列出公开接口：

类
```MATLAB
classdef ExperimentWorker<handle
    %实验主控制器
    %一般不直接使用该类，而是通过*_Client.mlx实时脚本作为用户界面来操纵实验。当然您也可以根据我们提供的Client脚本学习本类的使用方法。
	properties
		%实验记录保存路径
		SavePath(1,1)string
		%断线重连尝试间隔秒数
		RetryInterval(1,1)double=2
		%断线重连尝试次数
		MaxRetryTimes(1,1)uint8=3
		%实验结束后是否保存记录
		SaveFile(1,1)logical
		%喵提醒码
		EndMiaoCode(1,1)string=""
		%喵提醒重试次数
		HttpRetryTimes(1,1)uint8=3
		%会话结束后是否自动关闭串口
		ShutDownSerialAfterSession(1,1)logical
		%当前运行会话
		Session(1,1)Gbec.UIDs
		%视频输入设备
		VideoInput
		%日期时间
		DateTime
		%鼠名
		Mouse string
	end
	properties(Dependent)
		%如果启用会话结束后自动关闭串口功能，该属性设置关闭串口的延迟时间
		SerialFreeTime(1,1)double
	end
	methods
		function obj=ExperimentWorker
		end
		function PauseSession(EW)
			%暂停会话
		end
		function ContinueSession(EW)
			%继续会话
		end
		function AbortSession(EW)
			%放弃会话
		end
		function CloseSerial(EW,~,~)
			%关闭串口
		end
		function StartCheckMonitor(EW,DeviceUID)
			%开始检查监视器
		end
		function StopCheckMonitor(EW)
			%停止检查监视器
		end
		function OneEnterOneCheck(EW,DeviceUID,EnterPrompt)
			%检查刺激器，按一次回车给一个刺激，输入任意字符停止检查
		end
		function CheckManyTimes(EW,DeviceUID,CheckTimes)
			%多次检查刺激器
		end
		function Information = GetInformation(EW)
			%获取会话信息
		end
		function SaveInformation(EW)
			%获取并保存会话信息
		end
		function SerialInitialize(EW,SerialPort)
			%初始化串口
		end
		function StartSession(EW)
			%开始会话
		end
	end
end
classdef UIDs<uint8
	%该枚举类中记录了和Arduino端完全一致的UID密码表，供MATLAB端参考，识别从串口发来的字节编码。如果要修改UID密码表，应当在Arduino端修改UIDs.h，然后运行GenerateMatlabUIDs.mlx，自动生成UIDs.m，不要手动修改该文件。
end
```
函数
```MATLAB
%本函数尝试修复 Arduino C++ 标准配置错误的问题。Arduino默认使用C++11标准编译，但本工具箱的Arduino代码使用了C++17新特性，因此必须改用C++17标准编译。
function ArduinoCppStandard
%根据Arduino端的UIDs.h，生成MATLAB端的UIDs.m
function GenerateMatlabUIDs
%安装通用行为控制工具箱的向导
function Setup
%该函数用于将输出的英文日志翻译成中文。如果你发现实验中输出的日志信息含有英文，而你希望将它翻译成方便阅读的中文，可以模仿已有条目增加新的翻译项。也可以对已有项进行删改。
function LogTranslate
```
脚本
```MATLAB
%此脚本用于Arduino端相关的配置，如代码上传、错误排除等
Development_Client
%实验主控台脚本。在这里完成相关配置，并开始实验。通常你需要设定好串口号，选择要运行的会话，设置保存路径，是否自动关闭串口，设置喵提醒码（如果要用），视频拍摄设置。实验进程中，如需暂停、放弃、继续、返回信息、断开串口等操作，则运行脚本中相应节。
Experiment_Client
%实验开始前，应当检查各项设备是否运行正常。该脚本中提供了多种常用的设备检查命令。你也可以照例增添新设备的检查命令。
SelfCheck_Client
```