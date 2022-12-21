function InterruptRetry(EW,~)
import Gbec.UID
import Gbec.GbecException
RunningOrPaused=EW.State==UID.SessionRunning||EW.State==UID.SessionPaused;
if RunningOrPaused
	EW.EventRecorder.LogEvent(UID.Event_SerialInterrupt);
end
Suffix="/"+string(EW.MaxRetryTimes)+"次";
SerialPort=EW.Serial.Port;
fprintf("串口连接中断");
ReconnectFail=true;
for a=1:EW.MaxRetryTimes
	disp("，正尝试恢复连接第"+string(a)+Suffix);
	pause(EW.RetryInterval);
	try
		EW.SerialInitialize(SerialPort);
		ReconnectFail=false;
		break;
	catch
		fprintf("串口同步失败");
	end
end
if ReconnectFail
	GbecException.Disconnection_reconnection_failed.Throw;
end
disp("重新连接成功");
if RunningOrPaused
	EW.EventRecorder.LogEvent(UID.Event_SerialReconnect);
	if EW.State==UID.State_SessionRunning
		EW.RestoreSession;
	else
		EW.State=UID.State_SessionRestored;
	end
end
end