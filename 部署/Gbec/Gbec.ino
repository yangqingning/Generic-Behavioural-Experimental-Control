#include "UserComponents.h"
#include "SerialInterrupt.h"
void setup()
{
	randomSeed(analogRead(0));
	Serial.setTimeout(-1);
	Serial.begin(9600);
	//PC端无法确认何时初始化完毕，不能提前发送信号，必须等待Arduino端宣布初始化完毕
	SerialWrite(Signal_Ready);
}
const ISession *AskForCurrentSession()
{
	UIDs CurrentUID = SerialRead<UIDs>();
	for (uint8_t a = 0; a < NoSessions; ++a)
		if (SessionList[a]->GetUID() == CurrentUID)
			return SessionList[a];
	return nullptr;
}
void loop()
{
	UIDs Instruction = SerialRead<UIDs>();
	//等待状态
	switch (Instruction)
	{
	case Command_Restore:
		if (CurrentSession = AskForCurrentSession())
		{
			//PC端需要确认找到会话才发送已完成回合数的数据
			SerialWrite(Signal_SessionFound);
			uint8_t NoDistinctUIDs = SerialRead<uint8_t>();
			UIDs DistinctUIDs[NoDistinctUIDs];
			uint16_t NoEachTrials[NoDistinctUIDs];
			SerialRead(DistinctUIDs, NoDistinctUIDs);
			SerialRead(NoEachTrials, NoDistinctUIDs);
			CurrentSession->Continue(NoDistinctUIDs, DistinctUIDs, NoEachTrials);
			//会话是完成还是放弃，由会话内部负责反馈，外部只管运行
			/*
			不应使用如下代码，因为参数的计算顺序是未定义的：
			//CurrentSession->Continue(NoDistinctUIDs, SerialRead(DistinctUIDs, NoDistinctUIDs), SerialRead(NoEachTrials, NoDistinctUIDs));
			可能会导致NoEachTrials被先读取，数据错位。
			*/
		}
		else
			SerialWrite(Signal_NoSuchSession);
		break;
	case Command_Start:
		if (CurrentSession = AskForCurrentSession())
		{
			//为了和Restore命令保持一致性
			SerialWrite(Signal_SessionFound);
			CurrentSession->TryRun();
		}
		else
			SerialWrite(Signal_NoSuchSession);
		break;
	default:
		CheckerInstructions(Instruction, Signal_NoCurrentSession);
		break;
	}
}
