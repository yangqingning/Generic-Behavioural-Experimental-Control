function RunningCallback(EW,~,~)
import Gbec.UID
while EW.Serial.NumBytesAvailable
	EventUID=EW.Serial.read(1,'uint8');
	switch EventUID
		case UID.Signal_TrialStart
			TrialIndex=EW.Serial.read(1,'uint16');
			TrialUID=string(Gbec.LogTranslate(EW.Serial.read(1,'uint8')));
			EW.TrialRecorder.LogEvent(TrialUID);
			if ~isempty(EW.VideoInput)
				if EW.VideoInput.Logging=="off"
					trigger(EW.VideoInput);
				end
			end
			fprintf('\n回合%u-%s：',TrialIndex,Gbec.LogTranslate(TrialUID));
		case UID.SessionFinished
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
			if ~isempty(EW.VideoInput)
				stop(EW.VideoInput);
			end
			fprintf('\n会话完成');
			if EW.SaveFile
				EW.SaveInformation;
			else
				fprintf(" 数据未保存");
			end
			fprintf('\n');
			EW.Serial.configureCallback('off');
			EW.WatchDog.start;
			EW.State=UID.State_SessionFinished;
		otherwise
			EW.EventRecorder.LogEvent(EventUID);
			fprintf(' %s',Gbec.LogTranslate(EventUID));
	end
end
end