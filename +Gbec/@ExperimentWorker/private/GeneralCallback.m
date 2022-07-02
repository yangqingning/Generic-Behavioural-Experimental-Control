function GeneralCallback(EW,~,~)
import Gbec.UIDs
persistent Request HttpOptions
if isempty(Request)
	Request=matlab.net.http.RequestMessage;
	HttpOptions=matlab.net.http.HTTPOptions("ConnectTimeout",Inf);
end
if EW.Serial.NumBytesAvailable>0
	switch EW.Serial.read(1,"uint8")
		case UIDs.Signal_SessionFinished
			if EW.SaveFile
				EW.SaveInformation;
			else
				disp("数据未保存");
			end
			if ~isempty(EW.VideoInput)
				stop(EW.VideoInput);
			end
			disp("会话结束");
			EW.WatchDog.start;
			if EW.EndMiaoCode~=""
				for a=0:EW.HttpRetryTimes
					try
						Request.send("http://miaotixing.com/trigger?id="+EW.EndMiaoCode,HttpOptions);
					catch
						continue;
					end
					break;
				end
			end
		case UIDs.Signal_TrialStarted
			if ~isempty(EW.VideoInput)
				if EW.VideoInput.Logging=="off"
					trigger(EW.VideoInput);
				end
			end
			fprintf("回合%u",EW.Serial.read(1,"uint16"));
			EW.CurrentTrial=UIDs(EW.Serial.read(1,"uint8"));
			fprintf("-%s：",EW.CurrentTrial);
		case UIDs.Signal_Detected
			EW.HandleEvent(string(UIDs(EW.Serial.read(1,"uint8")))+"-Detected");
		case UIDs.Signal_TimeUp
			EW.HandleEvent(string(UIDs(EW.Serial.read(1,"uint8")))+"-TimeUp");
		case UIDs.Signal_Running
			EW.SessionRunning=true;
			disp("会话运行中");
		case UIDs.Signal_NoCurrentSession
			EW.SessionRunning=false;
			disp("会话未运行");
		case UIDs.Signal_SessionAborted
			if EW.SaveFile
				if input("实验已放弃，是否保存现有数据？y/n","s")~="n"
					EW.SaveInformation;
				else
					delete(EW.SavePath);
				end
			else
				disp("数据未保存");
			end
			%放弃实验主动函数里已经停止了相机设备，此处无需再次停止
			EW.WatchDog.start;
		case UIDs.Signal_Ready
			disp("串口已就绪");
		case UIDs.Signal_Paused
			EW.SessionRunning=false;
			disp("会话暂停");
		case UIDs.Signal_NoCurrentSession
			disp("没有运行中的会话");
		case UIDs.Signal_StimulatorUp
			EW.HandleEvent(string(UIDs(EW.Serial.read(1,"uint8")))+"-Up");
		case UIDs.Signal_StimulatorDown
			EW.HandleEvent(string(UIDs(EW.Serial.read(1,"uint8")))+"-Down");
		case UIDs.Signal_TrialFinished
			EW.TrialRecorder.Append(string(EW.CurrentTrial(7:end)));
			fprintf(newline);
	end
end
end