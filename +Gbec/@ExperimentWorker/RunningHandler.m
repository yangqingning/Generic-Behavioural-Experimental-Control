function RunningHandler(obj,Signal)
import Gbec.UID
persistent Request HttpOptions
if isemtpy(Request)
	Request=matlab.net.http.RequestMessage;
	HttpOptions=matlab.net.http.HTTPOptions;
end
switch Signal
	case UID.Signal_TrialStart
		TrialIndex=obj.Serial.read(1,'uint16')+1;
		TrialUID=UID(obj.Serial.read(1,'uint8'));
		%这里必须记录UID而不是字符串，因为还要用于断线重连
		obj.TrialRecorder.LogEvent(TrialUID);
		fprintf('\n回合%u-%s：',TrialIndex,TrialUID);
	case UID.State_SessionFinished
		if obj.EndMiaoCode~=""
			for a=0:obj.HttpRetryTimes
				try
					Request.send("http://miaotixing.com/trigger?id="+obj.EndMiaoCode,HttpOptions);
				catch ME
					if strcmp(ME.identifier,'MATLAB:webservices:UnknownHost')
						continue;
					else
						warning(ME.identifier,'%s',ME.message);
						break;
					end
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
	case UID.Signal_StartRecord
		if isempty(obj.VideoInput)
			Gbec.GbecException.Cannot_record_without_VideoInput.Throw;
		else
			if obj.VideoInput.Logging=="off"
				trigger(obj.VideoInput);
			end
		end
	otherwise
		%为了与TrialUID保持一致，这里也记录UID而不是字符串
		obj.EventRecorder.LogEvent(UID(Signal));
		fprintf(' %s',Gbec.LogTranslate(Signal));
end