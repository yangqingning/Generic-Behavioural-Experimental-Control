function InterruptRetry(EW,~)
import Gbec.UIDs
EW.Recorder.Hit("断线");
Suffix="/"+string(EW.MaxRetryTimes)+"次";
SerialPort=EW.Serial.Port;
fprintf("串口连接中断，");
for a=1:EW.MaxRetryTimes
	disp("正尝试恢复连接第"+string(a)+Suffix);
	pause(EW.RetryInterval);
	try
		EW.SerialInitialize(SerialPort);
	catch
		fprintf("串口同步失败，");
		continue;
	end
	Serial=EW.Serial;
	disp("重新连接成功");
% 	WS=warning("query");
% 	EW.WarningState=WS(1).state;
% 	warning off;
	EW.Recorder.Hit("重连");
	if EW.SessionRunning
		Serial.write(UIDs.Command_Restore,"uint8");
		Serial.write(EW.Session,"uint8");
		if Serial.read(1,"uint8")==UIDs.Signal_SessionFound
			TrialsDone=EW.TrialRecorder.Harvest;
			DistinctTrials=unique(TrialsDone);
			Serial.write(numel(DistinctTrials),"uint8");
			Serial.write(DistinctTrials,"uint8");
			Serial.write(countcats(categorical(TrialsDone)),"uint16");
			disp("实验继续");
			Serial.configureCallback("byte",1,@EW.GeneralCallback);
		else
			error("会话UID未找到");
		end
% 		if EW.CheckNumber~=Serial.read(1,"uint16")
% 			%检验数校验不通过，说明断线重连太紧凑，Arduino陷入了异常的工作状态，必须重新连接
% 			fprintf("Arduino状态校验失败，");
% 			delete(EW.Serial);
% 			continue;
% 		end
	end
	return;
end
disp("达到最大重试次数，连接失败");
end