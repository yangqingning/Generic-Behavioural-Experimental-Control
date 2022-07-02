#include "SerialInterrupt.h"
#include "UserComponents.h"
const ISession *CurrentSession = nullptr;
void CheckerInstructions(UIDs Instruction, UIDs Default)
{
	static UIDs DeviceID;
	static const ITestable *DeviceToCheck;
	switch (Instruction)
	{
	case Command_IsReady:
		SerialWrite(Signal_Ready);
		break;
	case Command_Information:
		if (CurrentSession)
		{
			SerialWrite(Signal_Information);
			SerialWrite(StructStart);
			CurrentSession->WriteInformation();
			SerialWrite(StructEnd);
		}
		else
			SerialWrite(Signal_NoCurrentSession);
		break;
	case Command_CheckDevice:
		SerialRead(DeviceID);
		for (uint8_t DeviceIndex = 0; DeviceIndex < NoCheckableDevices; ++DeviceIndex)
		{
			if ((DeviceToCheck = CheckableDevices[DeviceIndex])->GetUID() == DeviceID)
			{
				SerialWrite(Signal_DeviceFound);
				DeviceToCheck->Test(SerialRead<uint8_t>());
				return;
			}
		}
		SerialWrite(Signal_DeviceNotFound);
		break;
	case Command_SignalwiseCheck:
		SerialRead(DeviceID);
		for (uint8_t DeviceIndex = 0; DeviceIndex < NoCheckableDevices; ++DeviceIndex)
		{
			if ((DeviceToCheck = CheckableDevices[DeviceIndex])->GetUID() == DeviceID)
			{
				SerialWrite(Signal_DeviceFound);
				while (SerialRead<UIDs>() == Command_CheckOnce)
					DeviceToCheck->Test(1);
				return;
			}
		}
		SerialWrite(Signal_DeviceNotFound);
		break;
	default:
		SerialWrite(Default);
	}
}
InterruptResult CheckInterrupt()
{
	bool ShouldPause;
	UIDs Instruction;
	InterruptResult Result = NotPaused;
	while (Serial.available())
	{
		switch (SerialRead(Instruction))
		{
		case Command_Abort:
			return Abort;
		case Command_Pause:
			SerialWrite(Signal_Paused);
			ShouldPause = true;
			Result = Paused;
			while (ShouldPause)
			{
				switch (SerialRead(Instruction))
				{
				case Command_Abort:
					return Abort;
				case Command_Continue:
					ShouldPause = false;
					break;
				default:
					CheckerInstructions(Instruction, Signal_Paused);
				}
			}
			break;
		default:
			CheckerInstructions(Instruction, Signal_Running);
		}
	}
	return Result;
}