function RunningHandler(obj,Signal)
import Gbec.UID
switch Signal
	case UID.Signal_TrialStart
		TrialIndex=obj.Serial.read(1,'uint16')+1;
		TrialUID=UID(obj.Serial.read(1,'uint8'));
		%这里必须记录UID而不是字符串，因为还要用于断线重连
		obj.TrialRecorder.LogEvent(TrialUID);
		obj.EventRecorder.LogEvent(UID.Signal_TrialStart)
		TrialMod=mod(TrialIndex,obj.CheckCycle);
		if obj.MiaoCode~=""
			if TrialIndex==obj.DesignedNumTrials
				SendMiao('实验结束',obj.HttpRetryTimes,obj.MiaoCode);
			elseif TrialMod==0
				SendMiao(sprintf('已到%u回合，请检查实验状态',TrialIndex),obj.HttpRetryTimes,obj.MiaoCode);
			end
		end
		if TrialMod==1&&TrialIndex>1
			cprintf([1,0,1],'\n已过%u回合，请检查实验状态',TrialIndex-1);
		end
		FprintfInCommandWindow('\n回合%u-%s：',TrialIndex,TrialUID);
	case UID.State_SessionFinished
		if ~isempty(obj.VideoInput)
			stop(obj.VideoInput);
		end
		FprintfInCommandWindow('\n会话完成\n');

		obj.Serial.configureCallback('off');
		warning off MATLAB:callback:DynamicPropertyEventError
		%先关中断，以免与信息冲突。关中断可能会让已经产生的中断出错，这是MATLAB自身设计逻辑缺陷，不应当由用户负责处理，且不影响程序逻辑，可以直接忽略
		if obj.SaveFile
			obj.SaveInformation;
		else
			FprintfInCommandWindow(" 数据未保存");
		end

		obj.State=UID.State_SessionFinished;
		obj.WatchDog.start;
	case UID.Signal_StartRecord
		if isempty(obj.VideoInput)
			Gbec.Exceptions.Cannot_record_without_VideoInput.Throw;
		else
			if obj.VideoInput.Logging=="off"
				trigger(obj.VideoInput);
			end
		end
	case UID.Signal_HostAction
		if isempty(obj.HostAction)
			Gbec.Exceptions.Arduino_requires_undefined_HostAction.Throw;
		else
			obj.HostAction.Run(obj.Serial,obj.EventRecorder);
		end
	case UID.Signal_PreciseLog
		Event=UID(obj.Serial.read(1,'uint8'));
		Time=milliseconds(obj.Serial.read(1,'uint32'));
		if isempty(obj.TIC)
			obj.TIC=tic;
		end
		obj.TimeOffset=Time-seconds(toc(obj.TIC));
		obj.PreciseRecorder.PushBack(struct(Time=Time,Event=Event));
		FprintfInCommandWindow('%s（%s）',Gbec.LogTranslate(Event),Time);
	otherwise
		%为了与TrialUID保持一致，这里也记录UID而不是字符串
		obj.EventRecorder.LogEvent(UID(Signal));
		FprintfInCommandWindow(' %s',Gbec.LogTranslate(Signal));
end
end
function SendMiao(Note,HttpRetryTimes,MiaoCode)
for a=0:HttpRetryTimes
	try
		%HttpRequest不支持中文，必须用webread
		webread(sprintf('http://miaotixing.com/trigger?id=%s&text=%s',MiaoCode,Note));
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
function FprintfInCommandWindow(varargin)
if feature('SuppressCommandLineOutput')
	timer(StartDelay=0.1,TimerFcn=@(~,~)fprintf(varargin{:})).start;
else
	fprintf(varargin{:});
end
end