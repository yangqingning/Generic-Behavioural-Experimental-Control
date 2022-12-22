function InterruptRetry(obj,~,~)
import Gbec.UID
import Gbec.GbecException
RunningOrPaused=obj.State==UID.State_SessionRunning||obj.State==UID.State_SessionPaused;
if RunningOrPaused
	obj.EventRecorder.LogEvent(UID.Event_SerialInterrupt);
end
Suffix="/"+string(obj.MaxRetryTimes)+"次";
SerialPort=obj.Serial.Port;
fprintf("串口连接中断");
ReconnectFail=true;
for a=1:obj.MaxRetryTimes
	disp("，正尝试恢复连接第"+string(a)+Suffix);
	pause(obj.RetryInterval);
	try
		obj.SerialInitialize(SerialPort);
		ReconnectFail=false;
		break;
	catch
		fprintf("串口同步失败");
	end
end
if ReconnectFail
	%这里如果报错，会显示大量堆栈信息，所以不报错了
	fprintf('\n重新连接失败');
end
disp("重新连接成功");
if RunningOrPaused
	obj.EventRecorder.LogEvent(UID.Event_SerialReconnect);
	if obj.State==UID.State_SessionRunning
		obj.RestoreSession;
	else
		obj.State=UID.State_SessionRestored;
	end
end
end