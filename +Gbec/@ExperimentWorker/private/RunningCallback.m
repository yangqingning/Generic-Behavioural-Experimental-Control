function RunningCallback(obj,~,~)
import Gbec.UID
while obj.Serial.NumBytesAvailable
	EventUID=obj.Serial.read(1,'uint8');
	switch EventUID
		case UID.Signal_TrialStart
			TrialIndex=obj.Serial.read(1,'uint16')+1;
			TrialUID=char(UID(obj.Serial.read(1,'uint8')));
			TrialUID=string(TrialUID(7:end));
			obj.TrialRecorder.LogEvent(TrialUID);
			if ~isempty(obj.VideoInput)
				if obj.VideoInput.Logging=="off"
					trigger(obj.VideoInput);
				end
			end
			fprintf('\n回合%u-%s：',TrialIndex,TrialUID);
		case UID.State_SessionFinished
			if obj.EndMiaoCode~=""
				for a=0:obj.HttpRetryTimes
					try
						Request.send("http://miaotixing.com/trigger?id="+obj.EndMiaoCode,HttpOptions);
					catch
						continue;
					end
					break;
				end
			end
			if ~isempty(obj.VideoInput)
				stop(obj.VideoInput);
			end
			fprintf('\n会话完成\n');
			if obj.SaveFile
				obj.SaveInformation;
			else
				fprintf(" 数据未保存");
			end
			fprintf('\n');
			obj.Serial.configureCallback('off');
			if obj.ShutDownSerialAfterSession
				obj.WatchDog.start;
			end
			obj.State=UID.State_SessionFinished;
		otherwise
			EventUID=Gbec.LogTranslate(UID(EventUID));
			obj.EventRecorder.LogEvent(EventUID);
			fprintf(' %s',EventUID);
	end
end
end