function StartSession(obj)
%开始会话
import Gbec.UID
import Gbec.GbecException
if obj.State==UID.State_SessionRestored
	GbecException.There_is_already_a_session_being_paused.Throw;
end
if input("将保存为："+replace(obj.SavePath,"\","\\")+"，确认？y/n","s")=="y"
	if isfile(obj.SavePath)&&input("文件已存在，是否覆盖？y/n","s")~="y"
		disp("将不会自动写入文件");
		obj.SaveFile=false;
	else
		Fid=fopen(obj.SavePath,'w');
		if Fid==-1
			mkdir(fileparts(obj.SavePath));
			Fid=fopen(obj.SavePath,'w');
		end
		fclose(Fid);
		obj.SaveFile=true;
	end
else
	disp("取消会话");
	return;
end
obj.ApiCall(UID.API_Start);
if ~isempty(obj.VideoInput)
	%这一步很慢，必须优先执行完再启动Arduino。
	preview(obj.VideoInput);
	start(obj.VideoInput);
	waitfor(obj.VideoInput,'Running','on');
end
obj.Serial.write(obj.SessionUID,"uint8");
while true
	Signal=obj.WaitForSignal;
	switch Signal
		case UID.State_SessionRunning
			GbecException.There_is_already_a_session_running.Throw;
		case UID.State_SessionPaused
			GbecException.There_is_already_a_session_being_paused.Throw;
		case UID.Signal_NoSuchSession
			GbecException.Session_not_found_on_Arduino.Throw;
		case UID.Signal_SessionStarted
			obj.WatchDog.stop;
			obj.EventRecorder.Reset;
			obj.TrialRecorder.Reset;
			obj.SignalHandler=@obj.RunningHandler;
			obj.Serial.configureCallback("byte",1,@obj.SerialCallback);
			obj.State=UID.State_SessionRunning;
			disp('会话开始');
			break;
		otherwise
			obj.HandleSignal(Signal);
	end
end
end